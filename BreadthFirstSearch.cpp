#include "BreadthFirstSearch.h"

#include "access/system/QueryParser.h"

namespace hyrise { namespace access {
  namespace { 
    auto _ = QueryParser::registerPlanOperation<BreadthFirstSearch>("BFS");
  }


  void BreadthFirstSearch::executePlanOperation() {

  }

  std::shared_ptr<PlanOperation> BreadthFirstSearch::parse(const Json::Value &data) {
    auto res = BasicParser<IndexJoin>::parse(data);
    return res;
  }

}}