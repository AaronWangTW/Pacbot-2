#include "ghost/agent/RedGhostAgent.hpp"
#include "GameState.hpp"
#include "ghost/Ghost.hpp"

std::pair<int, int>
RedGhostAgent::getChaseTarget(const GameState &gameState) const {
  return {gameState.pacmanLoc.getRow(), gameState.pacmanLoc.getCol()};
}

std::pair<int, int> RedGhostAgent::getScatterTarget() const {
  return {SCATTER_ROW[GhostColors::RED], SCATTER_COL[GhostColors::RED]};
}

std::unique_ptr<IGhostAgent> RedGhostAgent::clone() const {
  return std::make_unique<RedGhostAgent>(*this);
}
