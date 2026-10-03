#include "EventAction.hh"
#include "RootOutput.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"

EventAction::EventAction(RootOutput& output): output_(output) {}
void EventAction::BeginOfEventAction(const G4Event* event) {
  data_=EventData{};
  data_.event=event->GetEventID();
  data_.run=G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  if(auto vertex=event->GetPrimaryVertex()) {
    data_.primaryTimeNs=vertex->GetT0()/ns;
    data_.beamXmm=vertex->GetX0()/mm;
    data_.beamYmm=vertex->GetY0()/mm;
    data_.beamZmm=vertex->GetZ0()/mm;
  }
}
void EventAction::EndOfEventAction(const G4Event*) { output_.Fill(data_); }
