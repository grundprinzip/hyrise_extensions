#include "ShortestPath.h"

#include <access/system/PlanOperation.h>
#include <access/system/QueryParser.h>

#include <io/StorageManager.h>

#include <storage/InvertedIndex.h>
#include <storage/PointerCalculator.h>

#include "log4cxx/logger.h"

#include <queue>
#include <unordered_set>
#include <set>

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

  void SingleShortestPathIdx::executePlanOperation() {

    // Get the index of the graph
    auto sm = io::StorageManager::getInstance();
    auto idx = std::dynamic_pointer_cast<storage::InvertedIndex<hyrise_int_t>>(sm->getInvertedIndex(_indexName));

    // Get the table we use to access the data
    auto tab = getInputTable();


    std::queue<detail::QElement> toProcess;

    // Mark the positions as visited
    std::vector<bool> visited(10000000, false);

    toProcess.emplace(source);

    size_t level = 0;
    detail::QElement result;

    size_t counter = 0;

    while(!toProcess.empty() && level < _levelStop) {
      counter++;
      auto element = toProcess.front();
      toProcess.pop();

      // Insert ourselves
      visited[element.value] = true; 

      // We found a path
      if (element.value == dest) {
        result = element;
        break;
      } else {
        // Append all children of element to the queue
        const auto& pos_list = idx->getPositionsForKeyRef(element.value);

        // Increase the level size, once we reach a new depth
        if (element.path.size() > level) {
          level++;
        }

        // Append each value in the queue. Since the inedx is position based,
        // append the value
        for(const auto& v : pos_list) {
	  auto newVal = tab->getValue<hyrise_int_t>(_field_definition[0], v);

	  //if (visited.insert(newVal).second) {
	  if (!visited[newVal]) {
            detail::QElement q(newVal, element.path);
            q.path.push_back(v);
            toProcess.push(std::move(q));
	    visited[newVal] = true;
          }
        }
      }
    }

    LOG4CXX_DEBUG(logger, "Visited: " << toProcess.size());
    LOG4CXX_DEBUG(logger, "Processed: " << counter);

    // If we found something build result table
    if (level < _levelStop && result.valid()) {
      addResult(storage::PointerCalculator::create(input.getTable(0), std::move(result.path)));
    }
  }

  void SingleShortestPathIdx::setIndexName(std::string n) {
    _indexName = n;
  }

  void SingleShortestPathIdx::setSearchPath(hyrise_int_t s, hyrise_int_t d) {
    source = s;
    dest = d;
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
