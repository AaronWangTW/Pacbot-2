#pragma once

#include "ghost/agent/IGhostAgent.hpp"
struct PinkGhostAgent : public IGhostAgent {
  std::pair<int, int> getChaseTarget(const GameState &gameState) const override;
  std::pair<int, int> getScatterTarget() const override;

  std::unique_ptr<IGhostAgent> clone() const override;
};
