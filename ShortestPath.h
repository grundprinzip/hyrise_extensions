// Copyright (c) 2013 Hasso-Plattner-Institut fuer Softwaresystemtechnik GmbH. All rights reserved.
#pragma once

#include <access/system/PlanOperation.h>

#include <vector>
#include <queue>
#include <memory>



namespace hyrise { namespace access {

/**
*  Allows to perform a breadth first search through a graph using an index and
*  the indexed field. The operation assumes that for each node the outlinks
*  can be obtained by looking up the values from the index.
*
*  Currently we assume that IDs are Integer values
*/
class SingleShortestPathIdx : public PlanOperation {

public:

  // Default value for break at level x
  static const size_t MAX_LEVEL = 5;

protected:

  // The index representing the graph
  std::string _indexName;

  size_t _levelStop = MAX_LEVEL;

  hyrise_int_t _source;
  hyrise_int_t _dest;

  const char NONE = 0;
  const char LEFT = 1;
  const char RIGHT = 2;

  value_id_t _vid_source;
  value_id_t _vid_dest;

  std::vector<pos_t> _parent;
  std::vector<char> _visited;
  std::vector<pos_t> _resultPath;

public:

  static std::shared_ptr<PlanOperation> parse(const Json::Value &data);

  void executePlanOperation();

  void setIndexName(std::string idx);

  void setSearchPath(hyrise_int_t s, hyrise_int_t d);

};

}}
