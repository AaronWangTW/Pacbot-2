#include "ghost/agent/OrangeGhostAgent.hpp"
#include "GameState.hpp"
#include "ghost/Ghost.hpp"
#include <cmath>

std::pair<int, int>
OrangeGhostAgent::getChaseTarget(const GameState &gameState) const {
  const Location &orangeGhostLocation =
      gameState.ghosts[GhostColors::ORANGE].location;
  int nextRow = orangeGhostLocation.getRow() + orangeGhostLocation.getRowDir();
  int nextCol = orangeGhostLocation.getCol() + orangeGhostLocation.getColDir();
  int squaredDistancePacman =
      std::pow(nextRow - gameState.pacmanLoc.getRow(), 2) +
      std::pow(nextCol - gameState.pacmanLoc.getCol(), 2);
  return squaredDistancePacman < 64
             ? std::pair<int, int>{gameState.pacmanLoc.getRow(),
                                   gameState.pacmanLoc.getCol()}
             : getScatterTarget();
}

std::pair<int, int> OrangeGhostAgent::getScatterTarget() const {
  return {SCATTER_ROW[GhostColors::ORANGE], SCATTER_COL[GhostColors::ORANGE]};
}

std::unique_ptr<IGhostAgent> OrangeGhostAgent::clone() const {
  return std::make_unique<OrangeGhostAgent>(*this);
}
