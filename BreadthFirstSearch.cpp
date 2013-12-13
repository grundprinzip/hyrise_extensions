#include "BreadthFirstSearch.h"

#include <access/system/PlanOperation.h>
#include <access/system/QueryParser.h>

namespace hyrise { namespace access {

  
  namespace { 
    auto _ = QueryParser::registerPlanOperation<BreadthFirstSearchIdx>("BFS");
  }

  void BreadthFirstSearchIdx::executePlanOperation() {

  }

  void BreadthFirstSearchIdx::setIndexName(std::string n) {
    _indexName = n;
  }

  std::shared_ptr<PlanOperation> BreadthFirstSearchIdx::parse(const Json::Value &data) {
    auto res = BasicParser<BreadthFirstSearchIdx>::parse(data);
    res->setIndexName(data["index"].asString());
    return res;
  }

}}