#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "RootOutput.hh"
#include "G4MTRunManager.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4SystemOfUnits.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "FTFP_BERT.hh"
#include "Randomize.hh"
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cmath>

int main(int argc,char** argv) {
 try {
  std::string species="C",output="",macro="";
  int events=1000; long seed=12345; double birks=0.126; bool ui=false;
  for(int i=1;i<argc;++i) {
    std::string arg=argv[i];
    if(arg=="--help") { std::cout<<"Options: --beam C|U --events N --output FILE.root --seed N --birks VALUE(mm/MeV) --ui --macro FILE\n"; return 0; }
    if(arg=="--ui") { ui=true; continue; }
    if(i+1>=argc) throw std::runtime_error("Missing value for "+arg);
    std::string value=argv[++i];
    if(arg=="--beam") species=value;
    else if(arg=="--events") events=std::stoi(value);
    else if(arg=="--output") output=value;
    else if(arg=="--seed") seed=std::stol(value);
    else if(arg=="--birks") birks=std::stod(value);
    else if(arg=="--macro") macro=value;
    else throw std::runtime_error("Unknown option "+arg);
  }
  if((species!="C" && species!="U") || events<=0 || seed<=0 || !std::isfinite(birks) || birks<0)
    throw std::runtime_error("Require beam C or U, positive events/seed, and finite nonnegative Birks constant");
  int z=species=="C"?6:92, a=species=="C"?12:238;
  if(output.empty()) output="output/"+species+"_1GeVu.root";
  RootOutput outputFile(output,species,birks);
  CLHEP::HepRandom::setTheSeed(seed);
  auto run=std::make_unique<G4MTRunManager>();
  run->SetNumberOfThreads(1); // Macros select the worker count before initialization.
  auto detector=new DetectorConstruction(birks); run->SetUserInitialization(detector);
  auto physics=new FTFP_BERT(0);
  physics->ReplacePhysics(new G4EmStandardPhysics_option4(0));
  physics->SetDefaultCutValue(0.01*mm);
  auto optical=G4OpticalParameters::Instance();
  optical->SetProcessActivation("Cerenkov",false);
  physics->RegisterPhysics(new G4OpticalPhysics(0));
  physics->RegisterPhysics(new G4StepLimiterPhysics);
  run->SetUserInitialization(physics);
  run->SetUserInitialization(new ActionInitialization(*detector,outputFile,z,a));
  if(macro.empty()) run->Initialize();
  auto commands=G4UImanager::GetUIpointer();
  int uiargc=1;
  std::unique_ptr<G4UIExecutive> session;
  if(ui) session=std::make_unique<G4UIExecutive>(uiargc,argv,"Qt");
  std::unique_ptr<G4VisExecutive> vis;
  if(ui) { vis=std::make_unique<G4VisExecutive>(); vis->Initialize(); }
  if(!macro.empty() && commands->ApplyCommand("/control/execute "+macro)!=0)
    throw std::runtime_error("Macro failed: "+macro);
  if(ui) session->SessionStart();
  else if(macro.empty()) run->BeamOn(events);
  std::string metadata="beam="+species+"; Z="+std::to_string(z)+"; A="+std::to_string(a)+"; kinetic_GeV_per_u=1; seed="+std::to_string(seed)+"; birks_mm_per_MeV="+std::to_string(birks)+"; threads="+std::to_string(run->GetNumberOfThreads())+"; material=EJ200; size_mm=300,100,1; physics=FTFP_BERT+EM_option4+scintillation; full_produced_yield_counted; optional_limited_optical_transport; sipm_arrays_per_edge=9; sipm_array_pitch_mm=30; sipm_array_centres_mm=-120,-90,-60,-30,0,30,60,90,120; sipm_units_per_array=10; sipm_units_per_edge=90; sipm_active_mm=1x1; sipm_depth_mm=0.1; sipm_faces=y_plus_minus_50mm; ideal_absorber_efficiency=1; provisional_Birks";
  outputFile.Write(metadata);
  std::cout<<"Saved "<<outputFile.GetEntries()<<" events to "<<output<<"\n";
  vis.reset(); session.reset(); run.reset(); outputFile.Close();
 } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
