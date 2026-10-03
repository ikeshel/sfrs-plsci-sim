#pragma once

#include "EventData.hh"
#include <memory>
#include <mutex>
#include <string>
class TFile;
class TH1D;
class TH2D;
class TTree;

// Shared output sink. Workers submit completed events under one lock.
// ROOT owns histograms and the tree through the file; row_ provides stable
// branch addresses for the lifetime of that file.
class RootOutput {
 public:
  RootOutput(const std::string& path,const std::string& species,double birks);
  ~RootOutput();
  void Fill(const EventData& event);
  void Write(const std::string& metadata);
  long long GetEntries() const;
  void Close();
 private:
  std::unique_ptr<TFile> file_;
  std::mutex mutex_;
  EventData row_;
  TH1D* energy_=nullptr;
  TH1D* light_=nullptr;
  TH2D* correlation_=nullptr;
  TH1D* topSiPM_=nullptr;
  TH1D* bottomSiPM_=nullptr;
  TH1D* siPMChannels_=nullptr;
  TH1D* topPosition_=nullptr;
  TH1D* bottomPosition_=nullptr;
  TH1D* totalPosition_=nullptr;
  TTree* tree_=nullptr;
};
