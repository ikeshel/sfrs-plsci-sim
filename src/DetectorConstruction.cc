#include "DetectorConstruction.hh"
#include "SiPMLayout.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4IonisParamMat.hh"
#include "G4UserLimits.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

DetectorConstruction::DetectorConstruction(double birks): birks_(birks) {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
  auto nist=G4NistManager::Instance();
  auto material=new G4Material("EJ200",1.023*g/cm3,2);
  // Manufacturer H:C atomic ratio 1.104; approximate doped PVT bulk composition.
  material->AddElement(nist->FindOrBuildElement("C"),12.011/(12.011+1.104*1.008));
  material->AddElement(nist->FindOrBuildElement("H"),1.104*1.008/(12.011+1.104*1.008));
  material->GetIonisation()->SetBirksConstant(birks_*mm/MeV);
  auto mpt=new G4MaterialPropertiesTable;
  // Approximate emission spectrum, centred on the manufacturer's 425 nm peak.
  double energies[]={2.48*eV,2.70*eV,2.918*eV,3.10*eV,3.35*eV};
  double spectrum[]={0.,0.3,1.,0.3,0.};
  mpt->AddProperty("SCINTILLATIONCOMPONENT1",energies,spectrum,5);
  mpt->AddConstProperty("SCINTILLATIONYIELD",10000./MeV);
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1",2.1*ns);
  mpt->AddConstProperty("SCINTILLATIONYIELD1",1.);
  mpt->AddConstProperty("RESOLUTIONSCALE",1.);
  double opticalEnergy[]={2.0*eV,4.0*eV};
  double index[]={1.58,1.58}, absorption[]={380*cm,380*cm};
  mpt->AddProperty("RINDEX",opticalEnergy,index,2);
  mpt->AddProperty("ABSLENGTH",opticalEnergy,absorption,2);
  material->SetMaterialPropertiesTable(mpt);
  auto vacuum=nist->FindOrBuildMaterial("G4_Galactic");
  auto vacuumProperties=new G4MaterialPropertiesTable;
  double vacuumIndex[]={1.,1.};
  vacuumProperties->AddProperty("RINDEX",opticalEnergy,vacuumIndex,2);
  vacuum->SetMaterialPropertiesTable(vacuumProperties);
  auto world=new G4LogicalVolume(new G4Box("World",25*cm,15*cm,10*cm),nist->FindOrBuildMaterial("G4_Galactic"),"World");
  auto physical=new G4PVPlacement(nullptr,{},world,"World",nullptr,false,0,true);
  slab_=new G4LogicalVolume(new G4Box("Slab",15*cm,5*cm,0.5*mm),material,"Scintillator");
  slab_->SetUserLimits(new G4UserLimits(0.01*mm));
  new G4PVPlacement(nullptr,{},slab_,"Scintillator",world,false,0,true);

  // Ten 1 x 1 mm active faces at each y edge. A 0.1 mm silicon
  // thickness extends outward from the scintillator, with no air gap.
  auto silicon=nist->FindOrBuildMaterial("G4_Si");
  auto sensorShape=new G4Box("SiPMUnit",0.5*mm,0.05*mm,0.5*mm);
  topSiPM_=new G4LogicalVolume(sensorShape,silicon,"TopSiPM");
  bottomSiPM_=new G4LogicalVolume(sensorShape,silicon,"BottomSiPM");
  for(int array=0;array<SiPMLayout::arraysPerEdge;++array) {
    for(int unit=0;unit<SiPMLayout::unitsPerArray;++unit) {
      double x=(SiPMLayout::centreMm(array)+unit-4.5)*mm;
      int channel=array*SiPMLayout::unitsPerArray+unit;
      new G4PVPlacement(nullptr,{x,50.05*mm,0},topSiPM_,"TopSiPM",world,false,channel,true);
      new G4PVPlacement(nullptr,{x,-50.05*mm,0},bottomSiPM_,"BottomSiPM",world,false,channel+SiPMLayout::unitsPerEdge,true);
    }
  }
  // Ideal zero-reflection, unit-efficiency optical absorber. This bypasses
  // Fresnel reflection at the sensor and kills/counts every incident photon.
  auto surface=new G4OpticalSurface("IdealSiPMAbsorber");
  surface->SetType(dielectric_metal);
  surface->SetModel(unified);
  surface->SetFinish(polished);
  auto sensorProperties=new G4MaterialPropertiesTable;
  double reflectivity[]={0.,0.}, efficiency[]={1.,1.};
  sensorProperties->AddProperty("REFLECTIVITY",opticalEnergy,reflectivity,2);
  sensorProperties->AddProperty("EFFICIENCY",opticalEnergy,efficiency,2);
  surface->SetMaterialPropertiesTable(sensorProperties);
  new G4LogicalSkinSurface("TopSiPMAbsorber",topSiPM_,surface);
  new G4LogicalSkinSurface("BottomSiPMAbsorber",bottomSiPM_,surface);
  auto topStyle=new G4VisAttributes(G4Colour(1.,0.,1.));
  auto bottomStyle=new G4VisAttributes(G4Colour(0.,1.,1.));
  topStyle->SetForceSolid(true); bottomStyle->SetForceSolid(true);
  topSiPM_->SetVisAttributes(topStyle); bottomSiPM_->SetVisAttributes(bottomStyle);
  return physical;
}

int DetectorConstruction::GetSiPMChannel(const G4VPhysicalVolume* volume) const {
  if(!volume) return -1;
  auto logical=volume->GetLogicalVolume();
  if(logical!=topSiPM_ && logical!=bottomSiPM_) return -1;
  return volume->GetCopyNo();
}
