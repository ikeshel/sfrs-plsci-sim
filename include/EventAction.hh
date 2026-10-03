#pragma once

#include "G4UserEventAction.hh"
#include "EventData.hh"
class RootOutput;

class EventAction final: public G4UserEventAction {
 public:
  explicit EventAction(RootOutput& output);
  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;
  EventData& GetData() { return data_; }
 private:
  RootOutput& output_;
  EventData data_;
};
