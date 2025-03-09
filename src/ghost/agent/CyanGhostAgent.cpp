#include "ghost/agent/CyanGhostAgent.hpp"
#include "GameState.hpp"
#include "ghost/Ghost.hpp"

std::pair<int, int>
CyanGhostAgent::getChaseTarget(const GameState &gameState) const {
  return {
      2 * gameState.pacmanLoc.getRow() + 4 * gameState.pacmanLoc.getRowDir() -
          gameState.ghosts[GhostColors::RED].location.getRow(),
      2 * gameState.pacmanLoc.getCol() + 4 * gameState.pacmanLoc.getColDir() -
          gameState.ghosts[GhostColors::RED].location.getCol()};
}

std::pair<int, int> CyanGhostAgent::getScatterTarget() const {
  return {SCATTER_ROW[GhostColors::CYAN], SCATTER_COL[GhostColors::CYAN]};
}

std::unique_ptr<IGhostAgent> CyanGhostAgent::clone() const {
  return std::make_unique<CyanGhostAgent>(*this);
}