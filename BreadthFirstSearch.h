// Copyright (c) 2013 Hasso-Plattner-Institut fuer Softwaresystemtechnik GmbH. All rights reserved.
#pragma once

#include <access/system/PlanOperation.h>

namespace hyrise { namespace access {

/**
*  Allows to perform a breadth first search through a graph using an index and
*  the indexed field. The operation assumes that for each node the outlinks
*  can be obtained by looking up the values from the index.
*
*  Currently we assume that IDs are Integer values
*/
class BreadthFirstSearchIdx : public PlanOperation {

public:

  // Default value for break at level x
  static const size_t MAX_LEVEL = 5;

protected:

  // The index representing the graph
  std::string _indexName;

  size_t _levelStop = MAX_LEVEL;


public:

  static std::shared_ptr<PlanOperation> parse(const Json::Value &data);

  void executePlanOperation();

  void setIndexName(std::string idx);

};

}}