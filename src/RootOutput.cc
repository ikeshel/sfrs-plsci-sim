#include "RootOutput.hh"
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TTree.h>
#include <TNamed.h>
#include <TROOT.h>
#include <filesystem>
#include <stdexcept>

RootOutput::RootOutput(const std::string& path,const std::string& species,double birks) {
  auto parent=std::filesystem::path(path).parent_path();
  if(!parent.empty()) std::filesystem::create_directories(parent);
  ROOT::EnableThreadSafety();
  file_=std::make_unique<TFile>(path.c_str(),"RECREATE");
  if(file_->IsZombie()) throw std::runtime_error("Cannot create ROOT output");
  double maximum=species=="C"?100.:10000.;
  double lightMaximum=birks==0 ? maximum*10000. : (species=="C"?200000.:10000000.);
  energy_=new TH1D("hEdep","Total deposited energy;E_{dep} [MeV];Events",1000,0,maximum);
  light_=new TH1D("hScintPhotons","Scintillation photons produced;Photons/event;Events",1000,0,lightMaximum);
  correlation_=new TH2D("hPhotonsVsEdep","Produced light versus deposited energy;E_{dep} [MeV];Photons/event",200,0,maximum,200,0,lightMaximum);
  topSiPM_=new TH1D("hSiPMTop","Photons absorbed by all top arrays;Absorbed photons/event;Events",1000,0,lightMaximum);
  bottomSiPM_=new TH1D("hSiPMBottom","Photons absorbed by all bottom arrays;Absorbed photons/event;Events",1000,0,lightMaximum);
  siPMChannels_=new TH1D("hSiPMChannels","Absorbed photons by channel;Channel (0-89 top, 90-179 bottom);Absorbed photons",SiPMLayout::totalUnits,-0.5,SiPMLayout::totalUnits-0.5);
  double lowCm=(SiPMLayout::centreMm(0)-SiPMLayout::pitchMm/2)/10.;
  double highCm=(SiPMLayout::centreMm(SiPMLayout::arraysPerEdge-1)+SiPMLayout::pitchMm/2)/10.;
  topPosition_=new TH1D("hSiPMTopVsPosition","Top arrays;Array centre x [cm];Absorbed photons (sum over events)",SiPMLayout::arraysPerEdge,lowCm,highCm);
  bottomPosition_=new TH1D("hSiPMBottomVsPosition","Bottom arrays;Array centre x [cm];Absorbed photons (sum over events)",SiPMLayout::arraysPerEdge,lowCm,highCm);
  totalPosition_=new TH1D("hSiPMTotalVsPosition","Top + bottom arrays;Array centre x [cm];Absorbed photons (sum over events)",SiPMLayout::arraysPerEdge,lowCm,highCm);
  tree_=new TTree("events","One row per incident ion");
  tree_->Branch("event",&row_.event); tree_->Branch("edep_MeV",&row_.edep);
  tree_->Branch("run_id",&row_.run);
  tree_->Branch("beam_x_mm",&row_.beamXmm);
  tree_->Branch("beam_y_mm",&row_.beamYmm);
  tree_->Branch("beam_z_mm",&row_.beamZmm);
  tree_->Branch("primary_edep_MeV",&row_.primaryEdep);
  tree_->Branch("scint_photons",&row_.photons); tree_->Branch("primary_exit_MeV",&row_.exitEnergy);
  tree_->Branch("transported_photons",&row_.transportedPhotons);
  tree_->Branch("optical_path_mm",&row_.opticalPath);
  tree_->Branch("sipm_top_photons",&row_.topSiPMPhotons);
  tree_->Branch("sipm_bottom_photons",&row_.bottomSiPMPhotons);
  tree_->Branch("sipm_photons",row_.siPMPhotons.data(),("sipm_photons["+std::to_string(SiPMLayout::totalUnits)+"]/I").c_str());
  tree_->Branch("sipm_array_photons",row_.siPMArrayPhotons.data(),("sipm_array_photons["+std::to_string(SiPMLayout::totalArrays)+"]/I").c_str());
  tree_->Branch("sipm_array_time_sum_ns",row_.siPMTimeSumNs.data(),("sipm_array_time_sum_ns["+std::to_string(SiPMLayout::totalArrays)+"]/D").c_str());
  tree_->Branch("sipm_array_time_sum_sq_ns2",row_.siPMTimeSumSqNs2.data(),("sipm_array_time_sum_sq_ns2["+std::to_string(SiPMLayout::totalArrays)+"]/D").c_str());
  tree_->Branch("sipm_first_photon_ns",&row_.siPMFirstPhotonNs);
  tree_->Branch("primary_time_ns",&row_.primaryTimeNs);
  tree_->Branch("sipm_array_first_photon_ns",row_.siPMFirstPhotonByArrayNs.data(),("sipm_array_first_photon_ns["+std::to_string(SiPMLayout::totalArrays)+"]/D").c_str());
}
RootOutput::~RootOutput() = default;

void RootOutput::Fill(const EventData& event) {
  std::lock_guard<std::mutex> lock(mutex_);
  row_=event;
  energy_->Fill(event.edep);
  light_->Fill(event.photons);
  correlation_->Fill(event.edep,event.photons);
  topSiPM_->Fill(event.topSiPMPhotons);
  bottomSiPM_->Fill(event.bottomSiPMPhotons);
  for(int channel=0;channel<SiPMLayout::totalUnits;++channel)
    if(event.siPMPhotons[channel]>0) siPMChannels_->Fill(channel,event.siPMPhotons[channel]);
  for(int array=0;array<SiPMLayout::arraysPerEdge;++array) {
    double xCm=SiPMLayout::centreMm(array)/10.;
    int top=event.siPMArrayPhotons[array];
    int bottom=event.siPMArrayPhotons[array+SiPMLayout::arraysPerEdge];
    if(top>0) topPosition_->Fill(xCm,top);
    if(bottom>0) bottomPosition_->Fill(xCm,bottom);
    if(top+bottom>0) totalPosition_->Fill(xCm,top+bottom);
  }
  tree_->Fill();
}

void RootOutput::Write(const std::string& metadata) {
  file_->cd();
  TNamed("configuration",metadata.c_str()).Write();
  file_->Write();
}
long long RootOutput::GetEntries() const { return tree_->GetEntries(); }
void RootOutput::Close() { file_->Close(); }
