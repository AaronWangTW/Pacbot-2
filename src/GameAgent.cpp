#include "GameAgent.hpp"
#include "DecisionModule.hpp"

#include <memory>
#include <vector>
#include <thread>
#include <mutex>

#include "GameState.hpp"
#include "Location.hpp"

void GameAgent::step(int numTicks, Directions pacmanDirection) {
  // Store the index of the last version
  versions.push(deltas.size());

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
          perform(ghostAgent->move(gameState, ghost));
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

void GameAgent::undo() {
  // Check to see if there was a previous version
  if (not versions.empty()) {
    int lastVersion = versions.top();

    // While not at the last version
    while (deltas.size() > lastVersion) {
      // Undoes the last delta
      IDelta* lastDelta = deltas.top();
      lastDelta->undo();
      deltas.pop();
    }
    versions.pop();
  }
}

void GameAgent::perform(IDelta* delta) {
  if (delta) {
    delta->perform();
    deltas.push(std::move(delta));
  }
}

void GameAgent::update(const GameState &gameState) {
  this->gameState = gameState;
}

GameAgent &GameAgent::operator=(const GameAgent &other) {
  // Guard self assignment
  if (this == &other) return *this;

  this->gameState = other.gameState;
  std::stack<IDelta*> d_stack = other.deltas;
  std::vector<IDelta*> new_deltas;

  while(!d_stack.empty()) {
    IDelta* old_d = d_stack.top();
    // std::unique_ptr<IDelta> new_d = std::make_unique<IDelta>(*(old_d.get()));
    d_stack.pop();
    new_deltas.push_back(old_d->clone());
    // new_deltas.push_back(new_d);
  }
  while(!(this->deltas.empty())) {
    delete this->deltas.top();
    this->deltas.pop();
  }
  for(unsigned int i = new_deltas.size() - 1; i >= 0; i--) {
    this->deltas.push(new_deltas[i]);
  }
  for(unsigned int i = 0; i < 4; i++) {
    delete this->ghostAgents[i];
    this->ghostAgents[i] = other.ghostAgents[i]->clone();
  }
  return *this;
}