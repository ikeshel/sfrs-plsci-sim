#pragma once

#include "G4UserStackingAction.hh"
#include <memory>
struct EventData;
class G4GenericMessenger;

class StackingAction final: public G4UserStackingAction {
 public:
  explicit StackingAction(EventData& data);
  ~StackingAction() override;
  G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;
 private:
  EventData& data_;
  bool transport_=false;
  int maximum_=100;
  std::unique_ptr<G4GenericMessenger> messenger_;
};
