#include "GameAgent.hpp"
#include "DecisionModule.hpp"

#include <memory>
#include <vector>
#include <thread>
#include <mutex>

#include "GameState.hpp"
#include "Location.hpp"

void GameAgent::step(int numTicks, Directions pacmanDirection) {

  // for (int tick = 1; tick <= numTicks; tick++) {
  //   if ((gameState.currTicks + tick) % gameState.updatePeriod) {
  //     continue;
  //   }

  //   // Generate the deltas associated with actions
  //   for (int i = 0; i < ghostAgents.size(); i++) {
  //     std::unique_ptr<IGhostAgent> &ghostAgent = ghostAgents[i];
  //     Ghost &ghost = gameState.ghosts[i];
  //     perform(ghostAgent->move(gameState, ghost));
  //   }
  // }

  int searchDepth = 3;

  auto processTick = [&](int startTick, int endTick) {
    for (int tick = startTick; tick <= endTick; tick++) {
      std::lock_guard<std::mutex> lock(_mutex);
      if ((gameState.currTicks + tick) % gameState.updatePeriod) {
        continue;
      }
      if (tick % searchDepth == 0) {
        // update game state
        update(gameState);
      } else {
        // Generate the deltas associated with actions
        for (int i = 0; i < ghostAgents.size(); i++) {
          IGhostAgent *ghostAgent = ghostAgents[i];
          Ghost &ghost = gameState.ghosts[i];
        }
      }
    }
  };

  for (int i = 0; i < numTicks; i += searchDepth) {
    int startTick = i + 1;
    int endTick = std::min(i + searchDepth, numTicks);
    _threads.emplace_back(processTick, startTick, endTick);
  }

  for (auto &thread : _threads) {
    thread.join();
  }
  
}

void GameAgent::update(const GameState &gameState) {
  this->gameState = gameState;
}

GameAgent::GameAgent(const GameAgent &other) {
  this->gameState = other.gameState;
  for(unsigned int i = 0; i < 4; i++) {
    this->ghostAgents[i] = other.ghostAgents[i]->clone();
  }
}

GameAgent &GameAgent::operator=(const GameAgent &other) {
  // Guard self assignment
  if (this == &other) return *this;

  this->gameState = other.gameState;
  for(unsigned int i = 0; i < 4; i++) {
    delete this->ghostAgents[i];
    this->ghostAgents[i] = other.ghostAgents[i]->clone();
  }
  return *this;
}