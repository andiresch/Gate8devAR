/*----------------------
  Copyright (C): OpenGATE Collaboration

  This software is distributed under the terms
  of the GNU Lesser General  Public Licence (LGPL)
  See GATE/LICENSE.txt for further details
  ----------------------*/

#include "GateEnergySpectrumActor.hh"
#ifdef G4ANALYSIS_USE_ROOT

#include "GateEnergySpectrumActorMessenger.hh"
#include "GateMiscFunctions.hh"

// g4 // inserted 30 Jan 2016:
#include <G4EmCalculator.hh>
#include <G4VoxelLimits.hh>
#include <G4NistManager.hh>
#include <G4PhysicalConstants.hh>
#include <math.h>


//-----------------------------------------------------------------------------
/// Constructors (Prototype)
GateEnergySpectrumActor::GateEnergySpectrumActor(G4String name, G4int depth):
  GateVActor(name,depth)
{
  GateDebugMessageInc("Actor",4,"GateEnergySpectrumActor() -- begin\n");

  mEmin = 0.;
  mEmax = 50.;
  mENBins = 100;
  
  mLETmin = 0.;
  mLETmax = 100.;
  mLETBins = 1000;

  mQmin = 0.;
  mQmax = 10.;
  mQBins = 2000;

  mEdepmin = 0.;
  mEdepmax = 50.;
  mEdepNBins = 100;

  Ei = 0.;
  Ef = 0.;
  newEvt = true;
  newTrack = true;
  sumNi=0.;
  nTrack=0;
  sumM1=0.;
  sumM2=0.;
  sumM3=0.;
  edep = 0.;
  pEnergySpectrumFluenceTrackNorm = 0.;
  mSaveAsTextFlag = false;
  mSaveAsDiscreteSpectrumTextFlag = false;
  mLETtoEBT3Flag = false;
  
  mEnableLETSpectrumFlag = true;
  mEnableQSpectrumFlag = true;
  mEnableEnergySpectrumNbPartFlag = false;
  mEnableEnergySpectrumFluenceCosFlag = false;
  mEnableEnergySpectrumFluenceTrackFlag = true;
  
  mEnableEnergySpectrumLETFlag = false;
  mEnableEnergySpectrumLETdoseWeightedFlag = false;
  mEnableEnergySpectrumEdepFlag = true;
  
  mEnableEdepHistoFlag = false;
  mEnableEdepTimeHistoFlag = false;
  mEnableEdepTrackHistoFlag = false;
  mEnableElossHistoFlag = false;
  
  mEnableLogBinning = false;  
  
  emcalc = new G4EmCalculator;

  pMessenger = new GateEnergySpectrumActorMessenger(this);
  GateDebugMessageDec("Actor",4,"GateEnergySpectrumActor() -- end\n");
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Destructor
GateEnergySpectrumActor::~GateEnergySpectrumActor()
{
  GateDebugMessageInc("Actor",4,"~GateEnergySpectrumActor() -- begin\n");
  GateDebugMessageDec("Actor",4,"~GateEnergySpectrumActor() -- end\n");
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Construct
void GateEnergySpectrumActor::Construct()
{
  GateVActor::Construct();

  // Enable callbacks
  EnableBeginOfRunAction(true);
  EnableBeginOfEventAction(true);
  EnablePreUserTrackingAction(true);
  EnablePostUserTrackingAction(true);
  EnableUserSteppingAction(true);
  EnableEndOfEventAction(true); // for save every n
  
  if (mLETtoEBT3Flag){
    G4NistManager::Instance()->FindOrBuildMaterial("EBTactive");
   }
  
  if (mEnableLogBinning){
     //G4cout<< "Before Loop: Emin: "<< mEmin<<G4endl;
     //G4cout<< "Before Loop: Emmax: "<< mEmax<<G4endl;
      double mEminLog = mEmin;
      if (mEminLog < 0.000001) mEminLog = 0.001;
      double energyLogBinV_LB = TMath::Log10(mEminLog);
      double energyLogBinV_UB = TMath::Log10(mEmax); 
      dEn = (energyLogBinV_UB - energyLogBinV_LB)/((double)mENBins );
      eBinV = new double[mENBins+1];
      //G4cout << "yes yes yes" <<G4endl<<G4endl;
      for (int j = 0; j < (mENBins+1); j++) {
          
          double linSpacedV = energyLogBinV_LB + (double)j   * dEn;
          //G4cout<<"Linear: "<< linSpacedV << G4endl;
          eBinV[j] = TMath::Power(10,  linSpacedV);
          //G4cout<< eBinV[j] << G4endl;
          //G4cout<<j<<G4endl;
          //j++;
      }
      //G4cout<< "end of loop"<<G4endl;
      //for (int i = 0; i <= mENBins; i++) G4cout << eBinV[i]<<G4endl;
  }
  //G4cout<< "After Loop: Emin: "<< mEmin<<G4endl;
     //G4cout<< "Before Loop: Emmax: "<< mEmax<<G4endl;
  //G4cout<< "Before init ifs"<<G4endl;
  
  pTfile = new TFile(mSaveFilename,"RECREATE");
  //G4cout<< "After init ifs"<<G4endl;
  if (mEnableEnergySpectrumNbPartFlag){
      //G4cout<<"Get EN bins"<<GetENBins()<<G4cout;
      pEnergySpectrumNbPart = new TH1D("energySpectrum","Energy Spectrum",GetENBins(),GetEmin() ,GetEmax() );
      pEnergySpectrumNbPart->SetXTitle("Energy (MeV)");  
  }
  //G4cout<< "Before init ifs"<<G4endl;
  if (mEnableEnergySpectrumFluenceCosFlag){
      if (!mEnableLogBinning){
          //G4cout<< "Do we enter here?"<<G4endl;
          pEnergySpectrumFluenceCos = new TH1D("energySpectrumFluenceCos","Energy Spectrum fluence 1/cos",GetENBins(),GetEmin() ,GetEmax() );
      }
      else {
          //G4cout<< "here is a problem?"<<G4endl;
          pEnergySpectrumFluenceCos = new TH1D("energySpectrumFluenceCosLogBin","Energy Spectrum fluence cos log bin",mENBins,eBinV);
          //G4cout<< "managed TH1D init"<<G4endl;
      }
      pEnergySpectrumFluenceCos->SetXTitle("Energy (MeV)");  
  }

  if (mEnableEnergySpectrumFluenceTrackFlag){
      pEnergySpectrumFluenceTrack = new TH1D("energySpectrumFluenceTrack","Energy Spectrum fluence Track",GetENBins(),GetEmin() ,GetEmax() );
      pEnergySpectrumFluenceTrack->SetXTitle("Energy (MeV)");
  } 

  
  if (mEnableEnergySpectrumLETdoseWeightedFlag){
      pEnergySpectrumLETdoseWeighted = new TH1D("energySpectrumLETdose","Energy Spectrum LET times energy deposit",GetENBins(),GetEmin() ,GetEmax() );
      pEnergySpectrumLETdoseWeighted->SetXTitle("LETd (MeV*MeV/mm)");
  } 
  if (mEnableEnergySpectrumLETFlag){
      pEnergySpectrumLET = new TH1D("energySpectrumLET","Energy Spectrum LET ",GetENBins(),GetEmin() ,GetEmax() );
      pEnergySpectrumLET->SetXTitle("LET (MeV/mm)");
  } 
  
  
  if (mEnableEnergySpectrumEdepFlag){
      pEnergyEdepSpectrum = new TH1D("energyEdepSpectrum","Energy Edep Spectrum",GetENBins(),GetEmin() ,GetEmax() );
      pEnergyEdepSpectrum->SetXTitle("Energy (MeV)");
  } 
  if (mEnableLETSpectrumFlag) {
      pLETSpectrum = new TH1D("LETSpectrum","LET Spectrum",GetNLETBins(),GetLETmin() ,GetLETmax() );
      pLETSpectrum->SetXTitle("LET (keV/um)");
  }
  if (mEnableQSpectrumFlag) {
      pQSpectrum = new TH1D("QSpectrum","Q Spectrum",GetNQBins(),GetQmin() ,GetQmax() );
      pQSpectrum->SetXTitle("Q (qq/MeV)");
  }
  
  if (mEnableEdepHistoFlag){
      pEdep  = new TH1D("edepHisto","Energy deposited per event",GetEdepNBins(),GetEdepmin() ,GetEdepmax() );
      pEdep->SetXTitle("E_{dep} (MeV)");
  } 

  if (mEnableEdepTimeHistoFlag){
      pEdepTime  = new TH2D("edepHistoTime","Energy deposited with time per event",
                            GetEdepNBins(),0,20,GetEdepNBins(),GetEdepmin(),GetEdepmax());
      pEdepTime->SetXTitle("t (ns)");
      pEdepTime->SetYTitle("E_{dep} (MeV)");
  } 

  if (mEnableEdepTrackHistoFlag){
      pEdepTrack  = new TH1D("edepTrackHisto","Energy deposited per track",GetEdepNBins(),GetEdepmin() ,GetEdepmax() );
      pEdepTrack->SetXTitle("E_{dep} (MeV)");
  } 

  if (mEnableElossHistoFlag){
      pDeltaEc = new TH1D("eLossHisto","Energy loss",GetEdepNBins(),GetEdepmin() ,GetEdepmax() );
      pDeltaEc ->SetXTitle("E_{loss} (MeV)");
  } 
  ResetData();
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Save data
void GateEnergySpectrumActor::SaveData()
{
  //pEnergySpectrumFluenceTrack->Scale(1/pEnergySpectrumFluenceTrackNorm);
  GateVActor::SaveData();
  pTfile->Write();
  pTfile->Close();
  
 //uncommented by A.Resch 30 Nov 2018; not sure it works anyway
  // Also output data as txt if enabled
  //if (mSaveAsTextFlag) {
    //SaveAsText(pEnergySpectrumNbPart, mSaveFilename);
    //SaveAsText(pEnergySpectrumFluenceCos, mSaveFilename);
    //SaveAsText(pEnergySpectrumFluenceTrack, mSaveFilename);
    //SaveAsText(pEnergyEdepSpectrum, mSaveFilename);
    //SaveAsText(pEdep, mSaveFilename);
    //// SaveAsText(pEdepTime, mSaveFilename); no TH2D
    //SaveAsText(pEdepTrack, mSaveFilename);
    //SaveAsText(pDeltaEc, mSaveFilename);
  //}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::ResetData()
{
  if (mEnableEnergySpectrumNbPartFlag){
    pEnergySpectrumNbPart->Reset();
  }
  
  if (mEnableEnergySpectrumFluenceCosFlag){
      pEnergySpectrumFluenceCos->Reset();
  }
  
  if (mEnableEnergySpectrumFluenceTrackFlag){
      pEnergySpectrumFluenceTrack->Reset();
  }
  if (mEnableEnergySpectrumEdepFlag){
      pEnergyEdepSpectrum->Reset();
  }
   
  if (mEnableEnergySpectrumLETdoseWeightedFlag){
      pEnergySpectrumLETdoseWeighted->Reset();
  }
  if (mEnableEnergySpectrumLETFlag){
      pEnergySpectrumLET->Reset();
  }  
  if (mEnableLETSpectrumFlag) {
      pLETSpectrum->Reset();
  }
  if (mEnableQSpectrumFlag){
      pQSpectrum->Reset();
  }
  
  if (mEnableEdepHistoFlag){
      pEdep->Reset();
  }
  if (mEnableEdepTimeHistoFlag){
      pEdepTime->Reset();
  }
  
  if (mEnableEdepTrackHistoFlag){
      pEdepTrack->Reset();
  }
  
  if (mEnableElossHistoFlag){
      pDeltaEc->Reset();
  }
  nEvent = 0;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::BeginOfRunAction(const G4Run *)
{
  GateDebugMessage("Actor", 3, "GateEnergySpectrumActor -- Begin of Run\n");
  ResetData();
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::BeginOfEventAction(const G4Event*)
{
  GateDebugMessage("Actor", 3, "GateEnergySpectrumActor -- Begin of Event\n");
  newEvt = true;
  edep = 0.;
  tof  = 0;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::EndOfEventAction(const G4Event*)
{
  GateDebugMessage("Actor", 3, "GateEnergySpectrumActor -- End of Event\n");
  
    if (edep > 0) {
        if (mEnableEdepHistoFlag){
            pEdep->Fill(edep/MeV);
        }
        if (mEnableEdepTimeHistoFlag){
            pEdepTime->Fill(tof/ns,edep/MeV);
        }
      }
  
  
  
      
  nEvent++;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::PreUserTrackingAction(const GateVVolume *, const G4Track* t)
{
  GateDebugMessage("Actor", 3, "GateEnergySpectrumActor -- Begin of Track\n");
  newTrack = true;
  if (t->GetParentID()==1) nTrack++;
  edepTrack = 0.;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::PostUserTrackingAction(const GateVVolume *, const G4Track* t)
{
  GateDebugMessage("Actor", 3, "GateEnergySpectrumActor -- End of Track\n");
  double eloss = Ei-Ef;
  if (mEnableElossHistoFlag && eloss > 0) pDeltaEc->Fill(eloss/MeV,t->GetWeight() );
  
  if (mEnableEdepTrackHistoFlag && edepTrack > 0)  pEdepTrack->Fill(edepTrack/MeV,t->GetWeight() );
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::UserSteppingAction(const GateVVolume *, const G4Step* step)
{
  assert(step->GetTrack()->GetWeight() == 1.); // edep doesnt handle weight

  if(step->GetTotalEnergyDeposit()>0.01) sumM1+=step->GetTotalEnergyDeposit();
  else if(step->GetTotalEnergyDeposit()>0.00001) sumM2+=step->GetTotalEnergyDeposit();
  else sumM3+=step->GetTotalEnergyDeposit();

  edep = step->GetTotalEnergyDeposit();
  edepTrack += step->GetTotalEnergyDeposit();

  //cout << "--- " << step->GetTrack()->GetTrackID() << " " << step->GetTrack()->GetParentID() << endl;
  if (newEvt) {
    double pretof = step->GetPreStepPoint()->GetGlobalTime();
    double posttof = step->GetPostStepPoint()->GetGlobalTime();
    tof = pretof + posttof;
    tof /= 2;
    //cout << "****************** new event tof=" << pretof << "/" << posttof << "/" << tof << " edep=" << edep << endl;
    newEvt = false;
  } else {
    double pretof = step->GetPreStepPoint()->GetGlobalTime();
    double posttof = step->GetPostStepPoint()->GetGlobalTime();
    double ltof = pretof + posttof;
    ltof /= 2;
    //cout << "****************** diff tof=" << ltof << " edep=" << edep << endl;
  }
  Ei=step->GetPreStepPoint()->GetKineticEnergy();
  Ef=step->GetPostStepPoint()->GetKineticEnergy();
  if(newTrack){
    
     
    if (mEnableEnergySpectrumNbPartFlag){
        pEnergySpectrumNbPart->Fill(Ei/MeV,step->GetTrack()->GetWeight());
    }
    
    G4ThreeVector momentumDir = step->GetTrack()->GetMomentumDirection(); 
    
    if (mEnableEnergySpectrumFluenceCosFlag){
        double dz = TMath::Abs( momentumDir.z());
        //G4Material* materialH = step->GetPreStepPoint()->GetMaterial();//->GetName(); 
        //G4ParticleDefinition* partnameH = step->GetTrack()->GetDefinition();//->GetParticleName();
        //double rangeCSDA = emcalc->GetCSDARange(Ei, partnameH->GetParticleName(), materialH->GetName(), "world");
          const double alphaB = 2.2e-3;
          const double  pB = 1.77;

        double lengthOfDetector = 0.1;
        double rangeCSDA = alphaB * pow(Ei,pB)/lengthOfDetector;
        ////4double 	GetCSDARange (G4double kinEnergy, const G4ParticleDefinition *, const G4Material *, const G4Region *r=0)
        ////G4double 	GetCSDARange (G4double kinEnergy, const G4String &part, const G4String &mat, const G4String &s="world")
        ////G4double 	GetRange (G4double kinEnergy, const G4ParticleDefinition *, const G4Material *, const G4Region *r=0)
        if (dz > 0){
            //double Emean = (Ei+Ef)/2/MeV;
            //double invAngle = 1/acos(momentumDir.z()); %% this is wrong: correct is: cos( acos(dz) ). cos(acos (x) ) = x
            double invAngle = 1/dz;
            //G4cout << "E = " << Ei << G4endl;
            //G4cout << "CSDA = " << rangeCSDA<<G4endl;
            if (invAngle > rangeCSDA) {
                //G4cout << "cos > range"<< G4endl;
                //G4cout << "cosApp =" << invAngle<<G4endl;
                invAngle = rangeCSDA; 
            }
            //G4cout << " " <<G4endl;
            //if (invAngle > 10) invAngle = 10;
            pEnergySpectrumFluenceCos->Fill(Ei,step->GetTrack()->GetWeight()*invAngle);
        }
    }
    // uncommented A.Resch 30.Nov 2018
    //if (mSaveAsDiscreteSpectrumTextFlag) {
      //mDiscreteSpectrum.Fill(Ei/MeV, step->GetTrack()->GetWeight());
    //}
    newTrack=false;
  }
  
   if (mEnableEnergySpectrumFluenceTrackFlag){
       G4double stepLength = step->GetStepLength();
       pEnergySpectrumFluenceTrack->Fill(Ei/MeV,step->GetTrack()->GetWeight()*stepLength);
       pEnergySpectrumFluenceTrackNorm+= stepLength;
   }
   if (mEnableEnergySpectrumEdepFlag){
       pEnergyEdepSpectrum->Fill(Ei/MeV,step->GetTrack()->GetWeight()*step->GetTotalEnergyDeposit()/MeV);
   }
  if(mEnableLETSpectrumFlag || mEnableEnergySpectrumLETdoseWeightedFlag || mEnableEnergySpectrumLETFlag) {
      //G4double density = step->GetPreStepPoint()->GetMaterial()->GetDensity();
      G4Material* material = step->GetPreStepPoint()->GetMaterial();//->GetName(); 
      G4double energy1 = step->GetPreStepPoint()->GetKineticEnergy();
      G4double energy2 = step->GetPostStepPoint()->GetKineticEnergy();
      G4double energy=(energy1+energy2)/2;
      G4ParticleDefinition* partname = step->GetTrack()->GetDefinition();//->GetParticleName();
      G4double dedx;
      G4double edepH = step->GetTrack()->GetWeight()*step->GetTotalEnergyDeposit()/MeV;
      if (mLETtoEBT3Flag){
        dedx = emcalc->ComputeElectronicDEDX(energy, partname->GetParticleName(), "EBTactive" );
        G4double dedxWater = emcalc->ComputeElectronicDEDX(energy, partname, material);
        if (dedxWater > 0){
            edepH *= dedx/ dedxWater;
            }
        }
      else {
           dedx = emcalc->ComputeElectronicDEDX(energy, partname, material);
       } 
          //weightedLET = (weightedLET/dedx)*	emcalc->ComputeTotalDEDX(energy, partname->GetParticleName(), "G4_WATER") ;
      if (mEnableLETSpectrumFlag) {
         pLETSpectrum->Fill(dedx/(keV/um), edepH/MeV);
      }
      if (mEnableEnergySpectrumLETdoseWeightedFlag){
          //G4double edep = step->GetTotalEnergyDeposit();
         pEnergySpectrumLETdoseWeighted->Fill(energy/MeV,step->GetTrack()->GetWeight()*dedx*edep);
      }
      
      if (mEnableEnergySpectrumLETFlag){
          //G4double edep = step->GetTotalEnergyDeposit();
         pEnergySpectrumLET->Fill(energy/MeV,step->GetTrack()->GetWeight()*dedx);
      }
  }  
  
  if(mEnableQSpectrumFlag) {
      //G4double density = step->GetPreStepPoint()->GetMaterial()->GetDensity();
      //G4Material* material = step->GetPreStepPoint()->GetMaterial();//->GetName();
      G4double energy1Q = step->GetPreStepPoint()->GetKineticEnergy();
      G4double energy2Q = step->GetPostStepPoint()->GetKineticEnergy();
      G4double energyQ=(energy1Q+energy2Q)/2;
      G4int chargeQ = step->GetTrack()->GetDefinition()->GetAtomicNumber();
      
      //G4ParticleDefinition* partname = step->GetTrack()->GetDefinition();//->GetParticleName();
      //G4double dedx = emcalc->ComputeElectronicDEDX(energy, partname, material);
      G4double Q =chargeQ; // to convert Int to Double
      Q*=Q; // now chargeQ is squared
      Q/=(energyQ/MeV); // now we divide chargeQ^2 / energyQ
      pQSpectrum->Fill(Q,step->GetTrack()->GetWeight()*step->GetTotalEnergyDeposit()/MeV);
  }
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
void GateEnergySpectrumActor::SaveAsText(TH1D * histo, G4String initial_filename)
{
  // Compute new filename: remove extension, add name of the histo, add txt extension
  std::string filename = removeExtension(initial_filename);
  filename = filename + "_"+histo->GetName()+".txt";

  // Create output file
  std::ofstream oss;
  OpenFileOutput(filename, oss);

  // FIXME
  if (mSaveAsDiscreteSpectrumTextFlag) {
    oss << "# First line is two numbers " << std::endl
        << "#     First value is '1', it means 'discrete energy mode'" << std::endl
        << "#     Second value is ignored" << std::endl
        << "# Other lines : 2 columns. 1) energy 2) probability (nb divided by NbEvent)" << std::endl
        << "# Number of bins = " << mDiscreteSpectrum.size() << std::endl
        << "# Number of events: " << nEvent << std::endl
        << "1 0" << std::endl;
    for(int i=0; i<mDiscreteSpectrum.size(); i++) {
      oss << mDiscreteSpectrum.GetEnergy(i) << " " << mDiscreteSpectrum.GetValue(i)/nEvent << std::endl;
    }
    oss.close();
  }
  else {
    // write as text file with header and 2 columns: 1) energy 2) probability
    // The header is two numbers:
    ///    1 because it is mode 1 (see gps UserSpectrum)
    //     Emin of the histo

    // Root convention
    // For all histogram types: nbins, xlow, xup
    //         bin = 0;       underflow bin
    //         bin = 1;       first bin with low-edge xlow INCLUDED
    //         bin = nbins;   last bin with upper-edge xup EXCLUDED
    //         bin = nbins+1; overflow bin
    oss << "# First line is two numbers " << std::endl
        << "#     First value is '2', it means 'histogram mode'" << std::endl
        << "#     Second value is 'Emin' of the histogram" << std::endl
        << "# Other lines : 2 columns. 1) energy 2) probability (nb divided by NbEvent)" << std::endl
        << "# Number of bins = " << histo->GetNbinsX() << std::endl
        << "# Content below the first bin: " << histo->GetBinContent(0) << std::endl
        << "# Content above the last  bin: " << histo->GetBinContent(histo->GetNbinsX()+2) << std::endl
        << "# Content above the last  bin: " << histo->GetBinContent(histo->GetNbinsX()+2) << std::endl
        << "# Number of events: " << nEvent << std::endl
        << "2 " << histo->GetBinLowEdge(1) << std::endl; // start at 1
    for(int i=1; i<histo->GetNbinsX()+1; i++) {
      oss << histo->GetBinLowEdge(i) + histo->GetBinWidth(i) << " " << histo->GetBinContent(i)/nEvent << std::endl;
    }
    oss.close();
  }
}
//-----------------------------------------------------------------------------

#endif
