#pragma once

#include "GameState.hpp"
#include "ghost/agent/IGhostAgent.hpp"
struct CyanGhostAgent : public IGhostAgent {
  std::pair<int, int> getChaseTarget(const GameState &gameState) const override;
  std::pair<int, int> getScatterTarget() const override;

  IGhostAgent* clone() const override;
  ~CyanGhostAgent() override {};
};
