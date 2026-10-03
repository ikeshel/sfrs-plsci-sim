#pragma once

#include "G4VUserDetectorConstruction.hh"
class G4LogicalVolume;
class G4VPhysicalVolume;

class DetectorConstruction final: public G4VUserDetectorConstruction {
 public:
  explicit DetectorConstruction(double birks);
  G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetScintillator() const { return slab_; }
  int GetSiPMChannel(const G4VPhysicalVolume* volume) const;
 private:
  double birks_;
  G4LogicalVolume* slab_=nullptr;
  G4LogicalVolume* topSiPM_=nullptr;
  G4LogicalVolume* bottomSiPM_=nullptr;
};
