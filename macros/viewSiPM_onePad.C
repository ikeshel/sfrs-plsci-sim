#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TGraphErrors.h>
#include <TMultiGraph.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string>

void viewSiPM_onePad(const char* path="output/C_sipm_positions.root",int runId=-1) {
  auto file=TFile::Open(path);
  if(!file || file->IsZombie()) {
    Error("viewSiPM_onePad","Cannot open %s",path);
    return;
  }
  const char* names[]={"hSiPMTopVsPosition","hSiPMBottomVsPosition","hSiPMTotalVsPosition"};
  TH1* histograms[3];
  for(int i=0;i<3;++i) {
    histograms[i]=file->Get<TH1>(names[i]);
    if(!histograms[i]) {
      Error("viewSiPM_onePad","Missing %s; regenerate this file with the new geometry",names[i]);
      return;
    }
  }
  auto events=file->Get<TTree>("events");
  if(runId>=0) {
    if(!events || !events->GetLeaf("run_id") || !events->GetLeaf("sipm_array_photons")) {
      Error("viewSiPM_onePad","Run filtering requires run_id and sipm_array_photons branches"); return;
    }
    int positions=histograms[2]->GetNbinsX();
    if(events->GetLeaf("sipm_array_photons")->GetLen()!=2*positions) {
      Error("viewSiPM_onePad","Incompatible array-count data"); return;
    }
    for(int side=0;side<3;++side) {
      auto name=std::string(names[side])+"_run"+std::to_string(runId);
      histograms[side]=static_cast<TH1*>(histograms[side]->Clone(name.c_str()));
      histograms[side]->SetDirectory(nullptr);
      histograms[side]->Reset();
    }
    std::vector<int> arrayCounts(2*positions);
    events->SetBranchAddress("sipm_array_photons",arrayCounts.data());
    Long64_t selected=0;
    for(Long64_t event=0;event<events->GetEntries();++event) {
      events->GetEntry(event);
      if(events->GetLeaf("run_id")->GetValue()!=runId) continue;
      ++selected;
      for(int position=0;position<positions;++position) {
        histograms[0]->AddBinContent(position+1,arrayCounts[position]);
        histograms[1]->AddBinContent(position+1,arrayCounts[position+positions]);
        histograms[2]->AddBinContent(position+1,arrayCounts[position]+arrayCounts[position+positions]);
      }
    }
    events->ResetBranchAddresses();
    if(selected==0) { Error("viewSiPM_onePad","No events for run %d",runId); return; }
    Info("viewSiPM_onePad","Selected %lld events from run %d",selected,runId);
  } else if(events && events->GetLeaf("run_id") && events->GetMinimum("run_id")!=events->GetMaximum("run_id")) {
    Warning("viewSiPM_onePad","Pooling multiple runs; pass a run ID as the second argument to isolate a beam position");
  }
  const std::string outputStem=std::string(path)+(runId>=0?".run"+std::to_string(runId):"");
  const double peak=histograms[2]->GetMaximum();
  const double yMaximum=peak>0 ? 1.2*peak : 1.;
  auto canvas=new TCanvas("sipm_onePad","SiPM array sums versus position",1000,650);
  canvas->SetLeftMargin(0.14);
  canvas->SetRightMargin(0.04);
  for(int i=0;i<3;++i) {
    auto histogram=histograms[i];
    histogram->SetStats(false);
    histogram->SetLineColor(i==0?kMagenta+1:i==1?kCyan+2:kBlue+1);
    histogram->SetLineWidth(2);
    histogram->SetMinimum(0);
    histogram->SetMaximum(yMaximum);
    histogram->GetYaxis()->SetTitle("Summed absorbed photons");
    histogram->GetYaxis()->SetTitleOffset(1.6);
    for(int bin=1;bin<=histogram->GetNbinsX();++bin) {
      auto label=std::to_string(static_cast<int>(histogram->GetBinCenter(bin)));
      histogram->GetXaxis()->SetBinLabel(bin,label.c_str());
    }
  }
  histograms[2]->SetTitle("SiPM array sums versus position");
  histograms[2]->Draw("HIST");
  histograms[0]->Draw("HIST SAME");
  histograms[1]->Draw("HIST SAME");
  auto legend=new TLegend(0.17,0.70,0.39,0.88);
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->AddEntry(histograms[0],"Top arrays","l");
  legend->AddEntry(histograms[1],"Bottom arrays","l");
  legend->AddEntry(histograms[2],"Top + bottom","l");
  legend->Draw();
  canvas->SaveAs((outputStem+".sipm_onePad.png").c_str());

  const char* timeFields[]={"sipm_array_photons","sipm_array_time_sum_ns","sipm_array_time_sum_sq_ns2"};
  if(!events) { Warning("viewSiPM_onePad","Missing event tree; cannot plot arrival times"); return; }
  const int positions=histograms[2]->GetNbinsX();
  for(auto field:timeFields) {
    if(!events->GetLeaf(field) || events->GetLeaf(field)->GetLen()!=2*positions) {
      Warning("viewSiPM_onePad","Missing/incompatible timing data; rerun the simulation with the updated executable");
      return;
    }
  }
  std::vector<int> eventCounts(2*positions);
  std::vector<double> eventSums(2*positions),eventSquares(2*positions);
  std::vector<double> counts(2*positions,0.),sums(2*positions,0.),squares(2*positions,0.);
  const bool hasFirstPhoton=events->GetLeaf("sipm_array_first_photon_ns")
    && events->GetLeaf("sipm_array_first_photon_ns")->GetLen()==2*positions
    && events->GetLeaf("primary_time_ns");
  double primaryTime=0.;
  std::vector<double> firstByArray(2*positions,-1.);
  std::vector<int> resolutionEvents(3*positions,0);
  std::vector<double> delayMeans(3*positions,0.),delayM2(3*positions,0.);
  if(hasFirstPhoton) {
    events->SetBranchAddress("sipm_array_first_photon_ns",firstByArray.data());
    events->SetBranchAddress("primary_time_ns",&primaryTime);
  }
  events->SetBranchAddress(timeFields[0],eventCounts.data());
  events->SetBranchAddress(timeFields[1],eventSums.data());
  events->SetBranchAddress(timeFields[2],eventSquares.data());
  for(Long64_t event=0;event<events->GetEntries();++event) {
    events->GetEntry(event);
    if(runId>=0 && events->GetLeaf("run_id")->GetValue()!=runId) continue;
    if(hasFirstPhoton) {
      for(int side=0;side<3;++side) {
        for(int position=0;position<positions;++position) {
          int index=position+(side==1?positions:0);
          double first=firstByArray[index];
          if(side==2) {
            double bottomFirst=firstByArray[position+positions];
            if(first<0 || (bottomFirst>=0 && bottomFirst<first)) first=bottomFirst;
          }
          if(first<0) continue;
          double delay=first-primaryTime;
          int slot=side*positions+position;
          int samples=++resolutionEvents[slot];
          double delta=delay-delayMeans[slot];
          delayMeans[slot]+=delta/samples;
          delayM2[slot]+=delta*(delay-delayMeans[slot]);
        }
      }
    }
    for(int array=0;array<2*positions;++array) {
      counts[array]+=eventCounts[array];
      sums[array]+=eventSums[array];
      squares[array]+=eventSquares[array];
    }
  }
  events->ResetBranchAddresses();
  auto arrivalCanvas=new TCanvas("sipm_arrival","SiPM arrival time and first-photon timing resolution",1000,1000);
  arrivalCanvas->Divide(1,2);
  for(int pad=1;pad<=2;++pad) {
    arrivalCanvas->cd(pad);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    gPad->SetBottomMargin(0.14);
    gPad->SetGridy();
  }
  arrivalCanvas->cd(1);
  auto arrival=new TMultiGraph;
  arrival->SetName("sipm_arrival_graphs");
  arrival->SetTitle("Mean photon arrival time versus position;Array centre x [cm];Mean arrival time [ns]");
  auto spread=new TMultiGraph;
  spread->SetName("sipm_time_resolution_graphs");
  spread->SetTitle("First-photon timing resolution relative to beam time;Array centre x [cm];#sigma(t_{first} - t_{beam}) [ns]");
  auto timeLegend=new TLegend(0.17,0.70,0.39,0.88);
  timeLegend->SetBorderSize(0);
  timeLegend->SetFillStyle(0);
  const char* labels[]={"Top arrays","Bottom arrays","Top + bottom"};
  double timeMinimum=std::numeric_limits<double>::infinity(),timeMaximum=0.;
  double spreadMaximum=0.;
  double spreadMinimum=std::numeric_limits<double>::infinity();
  for(int side=0;side<3;++side) {
    auto graph=new TGraphErrors;
    auto spreadGraph=new TGraph;
    spreadGraph->SetName(side==0?"gSiPMTopTimeResolution":side==1?"gSiPMBottomTimeResolution":"gSiPMTotalTimeResolution");
    graph->SetName(side==0?"gSiPMTopArrivalTime":side==1?"gSiPMBottomArrivalTime":"gSiPMTotalArrivalTime");
    int colour=side==0?kMagenta+1:side==1?kCyan+2:kBlue+1;
    graph->SetLineColor(colour); graph->SetMarkerColor(colour);
    graph->SetMarkerStyle(20+side); graph->SetLineWidth(2);
    spreadGraph->SetLineColor(colour); spreadGraph->SetMarkerColor(colour);
    spreadGraph->SetMarkerStyle(20+side); spreadGraph->SetLineWidth(2);
    for(int position=0;position<positions;++position) {
      int index=position+(side==1?positions:0);
      double n=counts[index],sum=sums[index],sumSq=squares[index];
      if(side==2) { n+=counts[position+positions]; sum+=sums[position+positions]; sumSq+=squares[position+positions]; }
      if(n<=0) continue; // A position with no absorbed photons has no arrival-time estimate.
      double mean=sum/n;
      double error=n>1 ? std::sqrt(std::max(0.,sumSq-sum*sum/n)/(n*(n-1))) : 0.;
      int slot=side*positions+position;
      if(resolutionEvents[slot]>1) {
        const double sigma=std::sqrt(std::max(0.,delayM2[slot])/(resolutionEvents[slot]-1));
        if(sigma>0.) { // Zero cannot be displayed on a logarithmic axis.
          spreadGraph->SetPoint(spreadGraph->GetN(),histograms[2]->GetBinCenter(position+1),sigma);
          spreadMaximum=std::max(spreadMaximum,sigma);
          spreadMinimum=std::min(spreadMinimum,sigma);
        }
      }
      int point=graph->GetN();
      graph->SetPoint(point,histograms[2]->GetBinCenter(position+1),mean);
      graph->SetPointError(point,0.,error);
      timeMinimum=std::min(timeMinimum,mean-error);
      timeMaximum=std::max(timeMaximum,mean+error);
    }
    if(graph->GetN()>0) { arrival->Add(graph,"LP"); timeLegend->AddEntry(graph,labels[side],"lp"); }
    else delete graph;
    if(spreadGraph->GetN()>0) spread->Add(spreadGraph,"LP");
    else delete spreadGraph;
  }
  if(!arrival->GetListOfGraphs()) {
    Warning("viewSiPM_onePad","No absorbed photons; enable full optical transport for arrival-time measurements");
    delete arrival; delete spread; delete timeLegend; delete arrivalCanvas;
    return;
  }
  arrival->Draw("A");
  const double timeSpan=std::max(timeMaximum-timeMinimum,0.1);
  arrival->SetMinimum(std::max(0.,timeMinimum-0.1*timeSpan));
  arrival->SetMaximum(timeMaximum+0.6*timeSpan);
  arrival->GetXaxis()->SetLimits(histograms[2]->GetXaxis()->GetXmin(),histograms[2]->GetXaxis()->GetXmax());
  arrival->GetYaxis()->SetTitleOffset(1.6);
  timeLegend->Draw();
  arrivalCanvas->cd(2);
  if(spread->GetListOfGraphs()) {
    gPad->SetLogy();
    spread->Draw("A");
    spread->SetMinimum(0.5*spreadMinimum);
    spread->SetMaximum(1.2*spreadMaximum);
    spread->GetXaxis()->SetLimits(histograms[2]->GetXaxis()->GetXmin(),histograms[2]->GetXaxis()->GetXmax());
    spread->GetYaxis()->SetTitleOffset(1.6);
  } else {
    const char* reason=hasFirstPhoton
      ? "No positive resolution estimates (need at least two detected events)"
      : "Rerun simulation: first-photon timestamps are missing";
    Warning("viewSiPM_onePad","%s",reason);
    auto message=new TLatex(0.16,0.50,reason);
    message->SetNDC(); message->SetTextSize(0.035); message->Draw();
    delete spread;
  }
  arrivalCanvas->SaveAs((outputStem+".sipm_arrival.png").c_str());
}
