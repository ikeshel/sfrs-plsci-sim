#include "PrimaryGeneratorAction.hh"
#include "G4GenericMessenger.hh"
#include "G4IonTable.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(int z,int a)
  : z_(z),a_(a),gun_(1),secondPosition_(-20*mm,0,-5*mm) {
  gun_.SetParticlePosition({0,0,-5*mm});
  gun_.SetParticleMomentumDirection({0,0,1});
  messenger_=std::make_unique<G4GenericMessenger>(this,"/beam/","Beam position controls");
  messenger_->DeclareProperty("alternatePositions",alternate_,"Alternate /gun/position and /beam/secondPosition by event ID.");
  messenger_->DeclarePropertyWithUnit("secondPosition","mm",secondPosition_,"Second beam position for odd-numbered events.");
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  gun_.SetParticleDefinition(G4IonTable::GetIonTable()->GetIon(z_,a_,0.));
  gun_.SetParticleCharge(z_*eplus);
  gun_.SetParticleEnergy(a_*GeV); // Total kinetic energy: 1 GeV/u * A.
  // Preserve the macro-defined gun position; select the second position only
  // while generating an odd event, then restore the gun configuration.
  auto firstPosition=gun_.GetParticlePosition();
  if(alternate_ && event->GetEventID()%2==1) gun_.SetParticlePosition(secondPosition_);
  gun_.GeneratePrimaryVertex(event);
  gun_.SetParticlePosition(firstPosition);
}
