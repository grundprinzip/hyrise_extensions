// Copyright (c) 2013 Hasso-Plattner-Institut fuer Softwaresystemtechnik GmbH. All rights reserved.
#pragma once

namespace hyrise { namespace access {

/**
*
*/
class BreadthFirstSearchIdx : public PlanOperation {



public:

  static std::shared_ptr<PlanOperation> parse(const Json::Value &data);

  void executePlanOperation();


};

}