#include "delta/GhostMoveDelta.hpp"

void GhostMoveDelta::perform() const {
  ghostAgent->currentLocation = newLocation;
}

void GhostMoveDelta::undo() const {
  ghostAgent->currentLocation = previousLocation;
}

IDelta* GhostMoveDelta::clone() const {
  return new GhostMoveDelta(*this);
}