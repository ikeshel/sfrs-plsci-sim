#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "RootOutput.hh"
#include "PrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "StackingAction.hh"

ActionInitialization::ActionInitialization(DetectorConstruction& detector,RootOutput& output,int z,int a)
  : detector_(detector),output_(output),z_(z),a_(a) {}
void ActionInitialization::Build() const {
  auto eventAction=new EventAction(output_);
  auto& data=eventAction->GetData();
  SetUserAction(new PrimaryGeneratorAction(z_,a_));
  SetUserAction(eventAction);
  SetUserAction(new SteppingAction(detector_,data));
  SetUserAction(new StackingAction(data));
}
