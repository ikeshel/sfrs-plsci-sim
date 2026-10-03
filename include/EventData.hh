#pragma once
#include <array>
#include "SiPMLayout.hh"
// One worker's event totals, independent of ROOT output and other workers.
struct EventData {
  double edep=0, primaryEdep=0, exitEnergy=-1, opticalPath=0;
  int photons=0, event=0, transportedPhotons=0;
  int run=0;
  double beamXmm=0.,beamYmm=0.,beamZmm=0.;
  int topSiPMPhotons=0, bottomSiPMPhotons=0;
  std::array<int,SiPMLayout::totalUnits> siPMPhotons{};
  std::array<int,SiPMLayout::totalArrays> siPMArrayPhotons{};
  std::array<double,SiPMLayout::totalArrays> siPMTimeSumNs{};
  std::array<double,SiPMLayout::totalArrays> siPMTimeSumSqNs2{};
  double siPMFirstPhotonNs=-1.;
  double primaryTimeNs=0.;
  std::array<double,SiPMLayout::totalArrays> siPMFirstPhotonByArrayNs=[] {
    std::array<double,SiPMLayout::totalArrays> first;
    first.fill(-1.);
    return first;
  }();
};
