#include "delta/GhostMoveDelta.hpp"

void GhostMoveDelta::perform() const {
  ghostAgent->currentLocation = newLocation;
}

void GhostMoveDelta::undo() const {
  ghostAgent->currentLocation = previousLocation;
}

std::unique_ptr<IDelta> GhostMoveDelta::clone() const {
  return std::make_unique<GhostMoveDelta>(*this);
}