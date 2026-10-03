#include <TFile.h>
#include <TH1.h>
#include <TTree.h>
#include <TSystem.h>
#include <iostream>
#include <set>
#include <cmath>
void check(const char* path, int expected=100) {
 TFile f(path);
 auto h=f.Get<TH1>("hEdep"); auto p=f.Get<TH1>("hScintPhotons"); auto t=f.Get<TTree>("events");
 if(f.IsZombie() || !h || !p || !t || t->GetEntries()!=expected || h->GetEntries()!=expected || p->GetEntries()!=expected) { gSystem->Exit(1); return; }
 double edep; int photons,event,run=0;
 std::set<std::pair<int,int>> ids;
 if(t->GetBranch("run_id")) t->SetBranchAddress("run_id",&run);
 t->SetBranchAddress("event",&event);
 t->SetBranchAddress("edep_MeV",&edep); t->SetBranchAddress("scint_photons",&photons);
 double sumE=0,sumP=0;
 for(Long64_t i=0;i<t->GetEntries();++i) { t->GetEntry(i); if(!ids.emplace(run,event).second) {gSystem->Exit(4); return;} if(!std::isfinite(edep) || edep<0 || photons<0) {gSystem->Exit(2); return;} sumE+=edep; sumP+=photons; }
 if(sumE<=0 || sumP<=0) {gSystem->Exit(3); return;}
 std::cout<<"CHECK PASSED "<<path<<": "<<expected<<" events; mean edep "<<sumE/expected<<" MeV; mean photons "<<sumP/expected<<"; energy overflow "<<h->GetBinContent(h->GetNbinsX()+1)<<"; light overflow "<<p->GetBinContent(p->GetNbinsX()+1)<<std::endl;
}
