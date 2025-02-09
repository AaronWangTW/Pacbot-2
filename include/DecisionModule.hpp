#pragma once
#include "GameAgent.hpp"
#include "Location.hpp"
#include <climits>

class DecisionModule {
 public:
  DecisionModule(GameAgent agent, int depthLimit);
  DecisionModule();
  Directions decide();
  void overrideAgent(GameAgent agent);

 private:
  GameAgent _agent;
  int _depthLimit;
  int evaluateState(GameAgent& currAgent);
  int deepSearch(int depth, GameAgent& currAgent);
  std::queue<GameAgent> bfsSearch(int bfsDepth, GameAgent& agent);
};