#include "delta/GhostPlanDelta.hpp"

void GhostPlanDelta::perform() const {
  ghostAgent->plannedDirection = newDirection;
}

void GhostPlanDelta::undo() const {
  ghostAgent->plannedDirection = oldDirection;
}

IDelta* GhostPlanDelta::clone() const {
  return new GhostPlanDelta(*this);
}