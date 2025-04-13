#include "GameAgent.hpp"
#include "DecisionModule.hpp"

#include <memory>
#include <vector>
#include <thread>
#include <mutex>

#include "GameState.hpp"
#include "Location.hpp"

int noOfSetBits(uint32_t i) {
  // https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
  i = i - ((i >> 1) & 0x55555555);                 // add pairs of bits
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
  i = (i + (i >> 4)) & 0x0F0F0F0F;                 // groups of 8
  i *= 0x01010101;                                 // horizontal sum of bytes
  return i >> 24;  // return just that top byte (after truncating to 32-bit even
                   // when int is wider than uint32_t)
}

int noPellets(std::array<uint32_t, 31> pelletArr) {
  int sum = 0;
  for (unsigned int i = 0; i < 31; i++) {
    sum += noOfSetBits(pelletArr[i]);
  }
  return sum;
}

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

  this->_depthInterval = 3;
  int searchInterval = this->_depthInterval;

  auto processTick = [&](int startTick, int endTick) {
    for (int tick = startTick; tick <= endTick; tick++) {
      std::lock_guard<std::mutex> lock(_mutex);
      if ((gameState.currTicks + tick) % gameState.updatePeriod) {
        continue;
      }
      if (tick % searchInterval == 0) {
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

  for (int i = 0; i < numTicks; i += searchInterval) {
    int startTick = i + 1;
    int endTick = std::min(i + searchInterval, numTicks);
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

int GameAgent::simulateAction(int numTicks, Directions pacmanDirection) {
  // update Gamestate
  int searchDepth = this->_depthInterval;
  GameState* gameState = &(this->gameState);
  for (int tick = 0; tick <= numTicks; tick++) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (((gameState->currTicks + tick) % gameState->updatePeriod) != 0) {
      continue;
    }
    if (tick % searchDepth == 0) {
      // update game state
      update(*gameState);
    } else {
      // Generate the deltas associated with actions
      for (int i = 0; i < ghostAgents.size(); i++) {
        IGhostAgent *ghostAgent = ghostAgents[i];
        Ghost &ghost = gameState->ghosts[i];
        // update ghosts
        perform(ghostAgent->move(*(gameState), ghost));
      }

      if (!safetyCheck()){
        return 0;
      }

      if (gameState->modeSteps > 0){
        gameState->modeSteps--;
      } else if (gameState->modeSteps == 0) {
        // Scatter -> Chase
        if (gameState->gameMode == GameModes::SCATTER) {
          gameState->gameMode = GameModes::CHASE;
          gameState->modeSteps = 180;
          gameState->modeDuration = 180;
        // Chase -> Scatter
        } else if (gameState->gameMode == GameModes::CHASE) {
          gameState->gameMode = GameModes::SCATTER;
          gameState->modeSteps = 60;
          gameState->modeDuration = 60;
        }

        // reverse planned ghost directions
        for (int i = 0; i < ghostAgents.size(); i++) {
          ghostAgents[i]->plannedDirection = REVERSE_DIRECTIONS.find(
            ghostAgents[i]->plannedDirection)->second;
        }
      }
    }
  }

  if (pacmanDirection == Directions::NONE) {
    return 1;
  }
  
  gameState->pacmanLoc.setRowDir(D_ROW[pacmanDirection]);
  gameState->pacmanLoc.setColDir(D_COL[pacmanDirection]);
  gameState->pacmanLoc.setRow(gameState->pacmanLoc.getRow() + gameState->pacmanLoc.getRowDir());
  gameState->pacmanLoc.setCol(gameState->pacmanLoc.getCol() + gameState->pacmanLoc.getColDir());

  collectFruit(gameState->pacmanLoc.getRow(), gameState->pacmanLoc.getCol());
  collectPellet(gameState->pacmanLoc.getRow(), gameState->pacmanLoc.getCol());

  if (noPellets(gameState->pelletArr) == 0){
    return 1;
  }

  if (!safetyCheck()){
    return 0;
  }

  return 1;


}

void GameAgent::collectFruit(int row, int col){
  GameState* gameState = &(this->gameState);
  if (row == gameState->fruitLoc.getRow() && col == gameState->fruitLoc.getCol()) {
    gameState->fruitSteps = 0;
    gameState->currScore += 100;
    gameState->fruitLoc.setRow(32);
    gameState->fruitLoc.setCol(32);
  }

  if (gameState->fruitSteps > 0) {
    gameState->fruitSteps--;

  } else if (gameState->fruitSteps == 0) {
    gameState->fruitLoc.setRow(32);
    gameState->fruitLoc.setCol(32);
  }

}

void GameAgent::collectPellet(int row, int col){
  GameState* gameState = &(this->gameState);

  // Return if no pellets to collect
  if (!(((gameState->pelletArr[row]) >> col) & 1)) {
    return;
  }

  bool superPellet = superPelletAt(row, col);
  
  gameState->pelletArr[row] &= ~(1 << col);

  if (superPellet){
    gameState->currScore += 50;
  }else{
    gameState->currScore += 10;
  }

  int numPellets = noPellets(gameState->pelletArr);
  if (numPellets == 174 || numPellets == 74){
    gameState->fruitSteps = 30;
    gameState->fruitLoc.setRow(17);
    gameState->fruitLoc.setCol(13);
  }

  if (numPellets < 20){
    if (gameState->gameMode == GameModes::SCATTER) {
      gameState->gameMode = GameModes::CHASE;
    } 
  }

  if (superPellet){
    for (int i = 0; i < ghostAgents.size(); i++){
      ghostAgents[i]->plannedDirection = REVERSE_DIRECTIONS.find(
        ghostAgents[i]->plannedDirection)->second;
    }
  }
}

int GameAgent::superPelletAt(int row, int col){
  GameState* gameState = &(this->gameState);
  return (((gameState->pelletArr[row]) >> col) & 1) && ((row == 3) || (row == 23)) && ((col == 1) || (col == 26));
}

int GameAgent::safetyCheck(){

  GameState* gameState = &(this->gameState);

  int pacmanRow = gameState->pacmanLoc.getRow();
  int pacmanCol = gameState->pacmanLoc.getCol();

  for (int i = 0; i < gameState->ghosts.size(); i++){
    Ghost ghost = gameState->ghosts[i];
    int ghostRow = ghost.location.getRow();
    int ghostCol = ghost.location.getCol();
    if (pacmanRow == ghostRow && pacmanCol == ghostCol){
      if (!ghost.isFreightened()){
        return 0;
      } else {
        ghost.location.setRow(32);
        ghost.location.setCol(32);
        ghost.setSpawning(true);
      }
    }
  }

  return 1;
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