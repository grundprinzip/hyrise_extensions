#include "ShortestPath.h"

#include <access/system/PlanOperation.h>
#include <access/system/QueryParser.h>

#include <io/StorageManager.h>

#include <storage/InvertedIndex.h>
#include <storage/PointerCalculator.h>
#include <storage/OrderPreservingDictionary.h>

#include "log4cxx/logger.h"

#include <queue>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <iostream>

namespace hyrise { namespace access {

namespace detail {

struct QElement {
  hyrise_int_t value;
  std::vector<size_t> path;

  QElement() : value(0) {}

  explicit QElement(hyrise_int_t v) : value(v) {}

  QElement(hyrise_int_t v, const std::vector<size_t> &p) : value(v), path(p) {}

  bool valid() { return path.size() > 0; }

};

}

  
namespace { 
auto _ = QueryParser::registerPlanOperation<SingleShortestPathIdx>("SingleShortestPathIdx");
}

namespace { auto logger = log4cxx::Logger::getLogger("hyrise.access.plan.SingleShortestPathIdx"); }


pos_list_t retrace(hyrise_int_t start, hyrise_int_t end, const std::vector<pos_t>& parent, storage::c_atable_ptr_t tab) {
  pos_list_t result({parent[end]});
  while(tab->getValue<hyrise_int_t>(0, result.back()) != start) {
    auto val = tab->getValue<hyrise_int_t>(0, result.back());
    result.push_back(parent[val]);
  }
  return result;
}

std::vector<size_t> retrace_two(hyrise_int_t start, hyrise_int_t end, hyrise_int_t l, hyrise_int_t r, const std::vector<size_t>& parent, storage::c_atable_ptr_t tab) {

  // Since we only have the values and not the positions, lookup the position from the value and index

  std::vector<hyrise_int_t> l_path({l});
  
  while( l_path.back() != start) {
    auto val = l_path.back();
    l_path.push_back(parent[val]);
  }

  std::vector<hyrise_int_t> r_path({r});
  while( r_path.back() != end) {
    auto val = r_path.back();
    r_path.push_back(parent[val]);
  }

  std::reverse(l_path.begin(), l_path.end());
  l_path.insert(l_path.end(), r_path.begin(), r_path.end());


  // Use the index
  pos_list_t result;
  auto sm = io::StorageManager::getInstance();
  auto idx = std::dynamic_pointer_cast<storage::InvertedIndex<hyrise_int_t>>(sm->getInvertedIndex("friendships_idx"));

  for(size_t i=0; i < l_path.size() - 1; ++i) {
    const auto& pos_list = idx->getPositionsForKeyRef(l_path[i]);
    auto next = l_path[i+1];
    auto ftor = std::find_if(pos_list.begin(), pos_list.end(), [tab,next](pos_t p){ 
	return tab->getValue<hyrise_int_t>(1,p) == next;});
    result.push_back(*ftor);
  }

  return result;
}



  void SingleShortestPathIdx::executePlanOperation() {

    // Get the index of the graph
    auto sm = io::StorageManager::getInstance();
    auto idx = std::dynamic_pointer_cast<storage::InvertedIndex<hyrise_int_t>>(sm->getInvertedIndex(_indexName));

    // Get the table we use to access the data
    auto tab = getInputTable();


    // Queue to store the current progress
    std::queue<value_id_t> toProcessLeft;
    std::queue<value_id_t> toProcessRight;

    // Mark the positions as visited
    _visited = std::vector<char>(tab->size(), false);

    // Mark parents
    _parent = std::vector<pos_t>(tab->size());


    // State variables
    size_t level = 0;
    bool found = false;
    size_t counter = 0;

    _vid_source = tab->getValueIdForValue<hyrise_int_t>(1, _source).valueId;
    _vid_dest = tab->getValueIdForValue<hyrise_int_t>(1, _dest).valueId;



    // Get the AV and dictionary
    auto av = std::dynamic_pointer_cast<storage::FixedLengthVector<value_id_t>>(tab->getAttributeVectors(1).front().attribute_vector);
    auto dict = std::dynamic_pointer_cast<storage::OrderPreservingDictionary<hyrise_int_t>>(tab->dictionaryAt(1));

    // Use the VID for the lookup
    toProcessLeft.emplace(_source);
    toProcessRight.emplace(_dest);

    while(!toProcessLeft.empty() && !toProcessRight.empty()) {
      counter++;

      auto eL = toProcessLeft.front();
      toProcessLeft.pop();
      
      // We found a path
      if (eL == _vid_dest) {
	_resultPath = retrace(_source, _dest, _parent, getInputTable());
	found = true;
	break;
      } else {
	// Append all children of element to the queue
	if ( idx->exists(eL) ) {
	  const auto& pos_list = idx->getPositionsForKeyRef(eL);

	  // Append each value in the queue. Since the inedx is position based,
	  // append the value
	  for(const auto& v : pos_list) {
	    auto newValID = av->get(0, v);
	    auto newVal = dict->getValueForValueId(newValID);

	    // Check if we passed by before
	    if (_visited[newVal] == 0) {
	      _parent[newVal] = eL;
	      toProcessLeft.push(newVal);
	      _visited[newVal] = LEFT;
	    } else if (_visited[newVal] == RIGHT) {
	      _resultPath = retrace_two(_source, _dest, eL, newVal, _parent, getInputTable());
	      found = true;
	      break;
	    }
	  }
	}
      }
      if (found) break;

      auto eR = toProcessRight.front();
      toProcessRight.pop();
      
      // We found a path
      if (eR == _vid_source) {
	_resultPath = retrace(_source, _dest, _parent, getInputTable());
	found = true;
	break;
      } else {
	// Append all children of element to the queue
	if ( idx->exists(eR) ) {
	  const auto& pos_list = idx->getPositionsForKeyRef(eR);

	  // Append each value in the queue. Since the inedx is position based,
	  // append the value
	  for(const auto& v : pos_list) {
	    auto newValID = av->get(0, v);
	    auto newVal = dict->getValueForValueId(newValID);

	    // Check if we passed by before
	    if (_visited[newVal] == 0) {
	      _parent[newVal] = eR;
	      toProcessRight.push(newVal);
	      _visited[newVal] = RIGHT;
	    } else if (_visited[newVal] == LEFT) {
	      _resultPath = retrace_two(_source, _dest, newVal, eR, _parent, getInputTable());
	      found = true;
	      break;
	    }
	  }
	}
      }
      if (found) break;
    }

    LOG4CXX_DEBUG(logger, "Processed: " << counter);

    // If we found something build result table
    addResult(storage::PointerCalculator::create(input.getTable(0), std::move(_resultPath)));
  }

  void SingleShortestPathIdx::setIndexName(std::string n) {
    _indexName = n;
  }

  void SingleShortestPathIdx::setSearchPath(hyrise_int_t s, hyrise_int_t d) {
    _source = s;
    _dest = d;
  }

  std::shared_ptr<PlanOperation> SingleShortestPathIdx::parse(const Json::Value &data) {
    auto res = BasicParser<SingleShortestPathIdx>::parse(data);
    res->setIndexName(data["index"].asString());
    res->setSearchPath(data["source"].asInt(), data["dest"].asInt());
    if (data.isMember("max")) {
      res->_levelStop = data["max"].asUInt();
    }
    return res;
  }

}}
