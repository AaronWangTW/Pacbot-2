#pragma once
#include "Location.hpp"
#include "delta/IDelta.hpp"
#include "ghost/agent/IGhostAgent.hpp"

class GhostMoveDelta : public IDelta {
private:
  IGhostAgent *ghostAgent;
  Location newLocation;
  Location previousLocation;

public:
  GhostMoveDelta(IGhostAgent *ghostAgent, Location newLocation,
                 Location previousLocation)
      : ghostAgent(ghostAgent), newLocation(newLocation),
        previousLocation(previousLocation) {}
  void perform() const override;
  void undo() const override;
  IDelta* clone() const override;
};
