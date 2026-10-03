#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TPad.h>
#include <string>
void viewSiPM(const char* path="output/C_sipm_positions.root") {
  auto file=TFile::Open(path);
  if(!file || file->IsZombie()) { Error("viewSiPM","Cannot open %s",path); return; }
  const char* names[]={"hSiPMTopVsPosition","hSiPMBottomVsPosition","hSiPMTotalVsPosition"};
  TH1* histograms[3];
  for(int i=0;i<3;++i) {
    histograms[i]=file->Get<TH1>(names[i]);
    if(!histograms[i]) { Error("viewSiPM","Missing %s; regenerate this file with the new geometry",names[i]); return; }
  }
  auto canvas=new TCanvas("sipm_positions","SiPM array sums versus position",1500,500);
  canvas->Divide(3,1);
  const double yMaximum=histograms[2]->GetMaximum()>0
    ? 1.2*histograms[2]->GetMaximum() : 1.;
  for(int i=0;i<3;++i) {
    canvas->cd(i+1);
    gPad->SetLeftMargin(0.18);
    gPad->SetRightMargin(0.04);
    histograms[i]->SetStats(false);
    histograms[i]->SetLineColor(i==0?kMagenta+1:i==1?kCyan+2:kBlue+1);
    histograms[i]->SetLineWidth(2);
    histograms[i]->SetMinimum(0);
    histograms[i]->SetMaximum(yMaximum);
    histograms[i]->GetYaxis()->SetTitle("Summed absorbed photons");
    histograms[i]->GetYaxis()->SetTitleOffset(1.8);
    for(int bin=1;bin<=histograms[i]->GetNbinsX();++bin) {
      auto label=std::to_string(static_cast<int>(histograms[i]->GetBinCenter(bin)));
      histograms[i]->GetXaxis()->SetBinLabel(bin,label.c_str());
    }
    histograms[i]->Draw("HIST");
  }
  canvas->SaveAs((std::string(path)+".sipm.png").c_str());
}
