#pragma once

#include "G4UserSteppingAction.hh"
class DetectorConstruction;
struct EventData;

class SteppingAction final: public G4UserSteppingAction {
 public:
  SteppingAction(DetectorConstruction& detector,EventData& data);
  void UserSteppingAction(const G4Step* step) override;
 private:
  DetectorConstruction& detector_;
  EventData& data_;
};
