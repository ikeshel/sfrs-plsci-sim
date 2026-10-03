#pragma once

#include "G4VUserActionInitialization.hh"
class DetectorConstruction;
class RootOutput;

class ActionInitialization final: public G4VUserActionInitialization {
 public:
  ActionInitialization(DetectorConstruction& detector,RootOutput& output,int z,int a);
  void Build() const override;
 private:
  DetectorConstruction& detector_;
  RootOutput& output_;
  int z_,a_;
};
