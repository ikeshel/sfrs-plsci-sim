#include "StackingAction.hh"
#include "EventData.hh"
#include "G4GenericMessenger.hh"
#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

StackingAction::StackingAction(EventData& data): data_(data) {
  messenger_=std::make_unique<G4GenericMessenger>(this,"/optics/","Optical photon visualization controls");
  messenger_->DeclareProperty("transportPhotons",transport_,"Transport a limited sample of produced scintillation photons.");
  messenger_->DeclareProperty("maxPhotons",maximum_,"Maximum photons transported per event; -1 transports all generated photons.").SetParameterName("count",false).SetRange("count>=-1");
}
StackingAction::~StackingAction() = default;
G4ClassificationOfNewTrack StackingAction::ClassifyNewTrack(const G4Track* track) {
  if(track->GetDefinition()==G4OpticalPhoton::Definition()) {
    auto creator=track->GetCreatorProcess();
    if(creator && creator->GetProcessName()=="Scintillation") {
      ++data_.photons;
      if(transport_ && (maximum_<0 || data_.transportedPhotons<maximum_)) {
        ++data_.transportedPhotons;
        return fUrgent;
      }
    }
    // Full yield is counted, but only the first maximum_ photons are transported.
    return fKill;
  }
  return fUrgent;
}
