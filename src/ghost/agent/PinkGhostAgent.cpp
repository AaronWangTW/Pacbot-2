#include "ghost/agent/PinkGhostAgent.hpp"
#include "GameState.hpp"
#include "ghost/Ghost.hpp"

std::pair<int, int>
PinkGhostAgent::getChaseTarget(const GameState &gameState) const {
  return {gameState.pacmanLoc.getRow() + 4 * gameState.pacmanLoc.getRowDir(),
          gameState.pacmanLoc.getCol() + 4 * gameState.pacmanLoc.getColDir()};
}

std::pair<int, int> PinkGhostAgent::getScatterTarget() const {
  return {SCATTER_ROW[GhostColors::PINK], SCATTER_COL[GhostColors::PINK]};
}

std::unique_ptr<IGhostAgent> PinkGhostAgent::clone() const {
  return std::make_unique<PinkGhostAgent>(*this);
}
