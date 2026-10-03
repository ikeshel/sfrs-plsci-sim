#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ThreeVector.hh"
#include <memory>
class G4GenericMessenger;

class PrimaryGeneratorAction final: public G4VUserPrimaryGeneratorAction {
 public:
  PrimaryGeneratorAction(int z,int a);
  ~PrimaryGeneratorAction() override;
  void GeneratePrimaries(G4Event* event) override;
 private:
  int z_,a_;
  G4ParticleGun gun_;
  bool alternate_=false;
  G4ThreeVector secondPosition_;
  std::unique_ptr<G4GenericMessenger> messenger_;
};
