#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TSystem.h>
void view(const char* path="output/C_1GeVu.root") {
  auto file=TFile::Open(path);
  if(!file || file->IsZombie()) { Error("view","Cannot open %s",path); return; }
  auto canvas=new TCanvas("sfrs_plsci","SFRS plastic scintillator",1200,500);
  canvas->Divide(2,1);
  canvas->cd(1); file->Get<TH1>("hEdep")->Draw();
  canvas->cd(2); file->Get<TH1>("hScintPhotons")->Draw();
  canvas->SaveAs((std::string(path)+".png").c_str());
}
