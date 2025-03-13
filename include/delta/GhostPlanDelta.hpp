#pragma once
#include "delta/IDelta.hpp"
#include "ghost/agent/IGhostAgent.hpp"

/**
 * @class GhostPlanDelta
 * @brief A delta to the ghost's planned direction
 *
 */
class GhostPlanDelta : public IDelta {
private:
  IGhostAgent *ghostAgent;
  Directions newDirection;
  Directions oldDirection;

public:
  GhostPlanDelta(IGhostAgent *ghostAgent, Directions newDirection,
                 Directions oldDirection)
      : ghostAgent(ghostAgent), newDirection(newDirection),
        oldDirection(oldDirection) {}
  void perform() const override;
  void undo() const override;
  IDelta* clone() const override;
};
