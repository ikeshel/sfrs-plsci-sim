#include "SteppingAction.hh"
#include "DetectorConstruction.hh"
#include "EventData.hh"
#include "G4Step.hh"
#include "G4OpticalPhoton.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"

SteppingAction::SteppingAction(DetectorConstruction& detector,EventData& data)
  : detector_(detector),data_(data) {}
void SteppingAction::UserSteppingAction(const G4Step* step) {
  if(step->GetTrack()->GetDefinition()==G4OpticalPhoton::Definition()) {
    data_.opticalPath+=step->GetStepLength()/mm;
    int channel=detector_.GetSiPMChannel(step->GetPostStepPoint()->GetPhysicalVolume());
    if(channel>=0 && step->GetPostStepPoint()->GetStepStatus()==fGeomBoundary) {
      auto processes=step->GetTrack()->GetDefinition()->GetProcessManager()->GetProcessList();
      for(std::size_t i=0;i<processes->size();++i) {
        auto process=(*processes)[i];
        auto boundary=dynamic_cast<G4OpBoundaryProcess*>(process);
        if(boundary && boundary->GetStatus()==Detection) {
          ++data_.siPMPhotons.at(channel);
          ++data_.siPMArrayPhotons.at(channel/SiPMLayout::unitsPerArray);
          int array=channel/SiPMLayout::unitsPerArray;
          double arrivalNs=step->GetPostStepPoint()->GetGlobalTime()/ns;
          // Track processing order is not chronological; take the minimum.
          if(data_.siPMFirstPhotonNs<0 || arrivalNs<data_.siPMFirstPhotonNs)
            data_.siPMFirstPhotonNs=arrivalNs;
          auto& first=data_.siPMFirstPhotonByArrayNs.at(array);
          if(first<0 || arrivalNs<first) first=arrivalNs;
          data_.siPMTimeSumNs.at(array)+=arrivalNs;
          data_.siPMTimeSumSqNs2.at(array)+=arrivalNs*arrivalNs;
          if(channel<SiPMLayout::unitsPerEdge) ++data_.topSiPMPhotons;
          else ++data_.bottomSiPMPhotons;
          break;
        }
      }
    }
    return; // Exclude optical absorption from ion energy-deposition scoring.
  }
  if(step->GetPreStepPoint()->GetPhysicalVolume()->GetLogicalVolume()!=detector_.GetScintillator()) return;
  data_.edep+=step->GetTotalEnergyDeposit()/MeV;
  if(step->GetTrack()->GetTrackID()==1) {
    data_.primaryEdep+=step->GetTotalEnergyDeposit()/MeV;
    if(step->GetPostStepPoint()->GetStepStatus()==fGeomBoundary)
      data_.exitEnergy=step->GetPostStepPoint()->GetKineticEnergy()/MeV;
  }
}
