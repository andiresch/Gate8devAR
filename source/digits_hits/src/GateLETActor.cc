/*----------------------
  Copyright (C): OpenGATE Collaboration

  This software is distributed under the terms
  of the GNU Lesser General  Public Licence (LGPL)
  See GATE/LICENSE.txt for further details
  ----------------------*/


/*
  \brief Class GateLETActor :
  \brief
*/

// gate
#include "GateLETActor.hh"
#include "GateMiscFunctions.hh"


// A Resch 28.1.2019
#include "G4BGGNucleonElasticXS.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"


// g4
#include <G4EmCalculator.hh>
#include <G4VoxelLimits.hh>
#include <G4NistManager.hh>
#include <G4PhysicalConstants.hh>

//-----------------------------------------------------------------------------
GateLETActor::GateLETActor(G4String name, G4int depth):
  GateVImageActor(name,depth) {
  GateDebugMessageInc("Actor",4,"GateLETActor() -- begin\n");

  mIsTrackAverageDEDX=false;
  mIsTrackAverageEdepDX=false;
  mIsDoseAverageDEDX=false;
  mIsDoseAverageEdepDX=false;
  mIsAverageKinEnergy=false;
  mIsVerboseStoppingPower=false;
  mIsAlpha = false;
  mIsAlphaLinear=false;
  mIsFioriniFluenceFilm=false;
  mIsPalmansDoseFilm=false;
  mIsFioriniFluenceWater=false;
  mIsPalmansDoseWater=false;
  mIsAlphaLinearOverkillSaturation=false;
  mIsAlphaLinearOverkillReverse=false;
  mIsLETtoWaterEnabled = false;
  mIsParallelCalculationEnabled = false;
  mIsKillParticle=false;
  mIsCreatorProcess = false;
  mAveragingType = "DoseAverage";
  pMessenger = new GateLETActorMessenger(this);
  GateDebugMessageDec("Actor",4,"GateLETActor() -- end\n");

  emcalc = new G4EmCalculator;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Destructor
GateLETActor::~GateLETActor()  {
  delete pMessenger;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Construct
void GateLETActor::Construct() {
  GateDebugMessageInc("Actor", 4, "GateLETActor -- Construct - begin\n");
  GateVImageActor::Construct();

  //// Find G4_WATER. This it needed here because we will used this
  //// material for dedx computation for LETtoWater.
  //G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
  //G4Material* myWater =  G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
  //// A.Resch 28.1.2019
  //G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  //const G4ParticleDefinition* myProton  = particleTable->FindParticle("proton");
  //const G4ThreeVector & myVector = G4ThreeVector(0.,0.,0.);
  //G4double myEnergy = 100*MeV;
  ////G4DynamicParticle (const G4ParticleDefinition *aParticleDefinition, const G4ThreeVector &aMomentumDirection, G4double aKineticEnergy)
  //const  G4DynamicParticle myDynamicProton  = const G4DynamicParticle(myProton,myVector, myEnergy);
  ////G4double G4BGGNucleonElasticXS::GetElementCrossSection(const G4DynamicParticle*, G4int Z,
				  ////const G4Material* mat = 0);
   //G4double thisXS = G4BGGNucleonElasticXS::GetElementCrossSection(myDynamicProton, 6, myWater);
   //G4cout << "this worked?" <<G4endl;
   //G4cout << "XS: " <<thisXS<<G4endl;
   //G4cout << "did it?" <<G4endl<<G4endl<<G4endl<<G4endl<<G4endl<<G4endl<<G4endl;

  // Enable callbacks
  EnableBeginOfRunAction(true);
  EnableBeginOfEventAction(true);
  EnablePreUserTrackingAction(true);
  EnablePostUserTrackingAction(true);
  EnableUserSteppingAction(true);


  if (mAveragingType == "DoseAveraged" || mAveragingType == "DoseAverage" || mAveragingType == "doseaverage" || mAveragingType == "dose"){mIsDoseAverageDEDX = true;}
  else if (mAveragingType == "DoseAveragedEdep" || mAveragingType == "DoseAverageEdep" ){mIsDoseAverageEdepDX = true;}
  else if (mAveragingType == "TrackAveraged" || mAveragingType == "TrackAverage" || mAveragingType == "Track" || mAveragingType == "track" || mAveragingType == "TrackAveragedDXAveraged"){mIsTrackAverageDEDX = true;}
  else if (mAveragingType == "TrackAveragedEdep" || mAveragingType == "TrackAverageEdep" ){mIsTrackAverageEdepDX = true;}
  else if (mAveragingType == "AverageKinEnergy"){mIsAverageKinEnergy = true;}
  else if (mAveragingType == "killParticle"){mIsKillParticle = true;}
  else if (mAveragingType == "verbose"){mIsVerboseStoppingPower = true;mIsDoseAverageDEDX = true;}
  else if (mAveragingType == "creatorProcess"){mIsCreatorProcess = true;}
  else if (mAveragingType == "FioriniFluenceFilm" || mAveragingType == "FioriniFilm" || mAveragingType == "FioriniToFilm" ){mIsFioriniFluenceFilm = true;}
  else if (mAveragingType == "PalmansDoseFilm" || mAveragingType == "PalmansFilm" || mAveragingType == "PalmansToFilm" ){mIsPalmansDoseFilm = true;}
  else if (mAveragingType == "FioriniFluence" || mAveragingType == "FioriniFluenceWater" || mAveragingType == "FioriniWater" || mAveragingType == "FioriniToWater" || mAveragingType == "Fiorini" ){mIsFioriniFluenceWater = true;}
  else if (mAveragingType == "PalmansDose" || mAveragingType == "PalmansDoseWater" || mAveragingType == "PalmansWater" || mAveragingType == "PalmansToWater"  ||mAveragingType == "Palmans" ){mIsPalmansDoseWater = true;}
  else if (mAveragingType == "alphaLinear"){mIsAlphaLinear =true; mIsAlpha = true;}
  else if (mAveragingType == "alphaLinearOverkillSaturation"){mIsAlphaLinearOverkillSaturation =true; mIsAlpha = true;}
  else if (mAveragingType == "alphaLinearOverkillReverse"){mIsAlphaLinearOverkillReverse =true; mIsAlpha = true;}
   
  else {GateError("The LET averaging Type" << GetObjectName()
                  << " is not valid ...\n Please select 'DoseAveraged' or 'TrackAveraged')");}

  // Output Filename
  mLETFilename = mSaveFilename;
  if (mIsDoseAverageDEDX)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-doseAveraged."+ getExtension(mSaveFilename);
    }
  else if (mIsTrackAverageDEDX)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-trackAveraged."+ getExtension(mSaveFilename);
    }
  else if (mIsFioriniFluenceFilm)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-fioriniFluenceAveragedToFilm."+ getExtension(mSaveFilename);
    }
  else if (mIsPalmansDoseFilm)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-palmansDoseAveragedToFilm."+ getExtension(mSaveFilename);
    }
      else if (mIsFioriniFluenceWater)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-fioriniFluenceAveragedToWater."+ getExtension(mSaveFilename);
    }
  else if (mIsPalmansDoseWater)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-palmansDoseAveragedToWater."+ getExtension(mSaveFilename);
    }
  else if (mIsAlpha)
    {
      mLETFilename= removeExtension(mSaveFilename) + "-alpha."+ getExtension(mSaveFilename);
    }
  if (mIsLETtoWaterEnabled){
    mLETFilename= removeExtension(mLETFilename) + "-letToWater."+ getExtension(mLETFilename);
  }
  if (mIsAverageKinEnergy){
    mLETFilename= removeExtension(mLETFilename) + "-kinEnergyFluenceAverage."+getExtension(mLETFilename);
  }
  if (mIsParallelCalculationEnabled)
    {
      numeratorFileName= removeExtension(mLETFilename) + "-numerator."+ getExtension(mLETFilename);
      denominatorFileName= removeExtension(mLETFilename) + "-denominator."+ getExtension(mLETFilename);
    }

  // Set origin, transform, flag
  SetOriginTransformAndFlagToImage(mWeightedLETImage);
  SetOriginTransformAndFlagToImage(mNormalizationLETImage);
  SetOriginTransformAndFlagToImage(mDoseTrackAverageLETImage);

  // Resize and allocate images
  mWeightedLETImage.SetResolutionAndHalfSize(mResolution, mHalfSize, mPosition);
  mWeightedLETImage.Allocate();
  mNormalizationLETImage.SetResolutionAndHalfSize(mResolution, mHalfSize, mPosition);
  mNormalizationLETImage.Allocate();
  mDoseTrackAverageLETImage.SetResolutionAndHalfSize(mResolution, mHalfSize, mPosition);
  mDoseTrackAverageLETImage.Allocate();

  // Warning: for the moment we force to PostStepHitType. This is ok
  // (slightly faster) if voxel sizes are the same between the
  // let-actor and the attached voxelized volume. But wrong if not.
  mStepHitType = PostStepHitType;// RandomStepHitType; // Warning

  // Print information
  GateMessage("Actor", 1,
              "\tLET Actor      = '" << GetObjectName() << Gateendl <<
              "\tLET image      = " << mLETFilename << Gateendl <<
              "\tResolution     = " << mResolution << Gateendl <<
              "\tHalfSize       = " << mHalfSize << Gateendl <<
              "\tPosition       = " << mPosition << Gateendl);

  ResetData();
  GateMessageDec("Actor", 4, "GateLETActor -- Construct - end\n");
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Save datamDeltaRestricted
void GateLETActor::SaveData() {
  GateVActor::SaveData();


  if (mIsParallelCalculationEnabled) {
    mWeightedLETImage.Write(numeratorFileName);
    mNormalizationLETImage.Write(denominatorFileName);
  }
  else
    {
      GateImageDouble::const_iterator iter_LET = mWeightedLETImage.begin();
      GateImageDouble::const_iterator iter_Edep = mNormalizationLETImage.begin();
      GateImageDouble::iterator iter_Final = mDoseTrackAverageLETImage.begin();
      for(iter_LET = mWeightedLETImage.begin(); iter_LET != mWeightedLETImage.end(); iter_LET++) {
        if (*iter_Edep == 0.0) *iter_Final = 0.0; // do not divide by zero
        else *iter_Final = (*iter_LET)/(*iter_Edep);
        iter_Edep++;
        iter_Final++;
      }
      mDoseTrackAverageLETImage.Write(mLETFilename);

    }
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateLETActor::ResetData() {
  mWeightedLETImage.Fill(0.0);
  mNormalizationLETImage.Fill(0.0);

}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateLETActor::BeginOfRunAction(const G4Run * r) {
  GateVActor::BeginOfRunAction(r);
  GateDebugMessage("Actor", 3, "GateLETActor -- Begin of Run\n");
  // ResetData(); // Do no reset here !! (when multiple run);
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Callback at each event
void GateLETActor::BeginOfEventAction(const G4Event * e) {
  GateVActor::BeginOfEventAction(e);
  mCurrentEvent++;
  GateDebugMessage("Actor", 3, "GateLETActor -- Begin of Event: " << mCurrentEvent << Gateendl);
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GateLETActor::UserSteppingActionInVoxel(const int index, const G4Step* step) {
  GateDebugMessageInc("Actor", 4, "GateLETActor -- UserSteppingActionInVoxel - begin\n");
  GateDebugMessageInc("Actor", 4, "enedepo = " << step->GetTotalEnergyDeposit() << Gateendl);
  GateDebugMessageInc("Actor", 4, "weight = " <<  step->GetTrack()->GetWeight() << Gateendl);
  //	G4cout << "In LET actor: " << step->GetTrack()->GetDefinition()->GetAtomicNumber() << G4endl;
 //G4NistManager* man=G4NistManager::Instance();
  //G4Material* Water = man->FindOrBuildMaterial("Water");
  //G4cout<<"mean excitation: "<< Water->GetIonisation()->GetMeanExcitationEnergy()/eV<<G4endl;
  // Get edep and current particle weight
  const double weight = step->GetTrack()->GetWeight();

  // A.Resch tested calculation method:
  const double edep = step->GetTotalEnergyDeposit()*weight;

  double steplength = step->GetStepLength();

  //if no energy is deposited or energy is deposited outside image => do nothing
  if (edep == 0) {
    GateDebugMessage("Actor", 5, "GateLETActor edep == 0 : do nothing\n");
    return;
  }
  if (index < 0) {
    GateDebugMessage("Actor", 5, "GateLETActor pixel index < 0 : do nothing\n");
    return;
  }

  const G4Material* material = step->GetPreStepPoint()->GetMaterial();//->GetName();
  double energy1 = step->GetPreStepPoint()->GetKineticEnergy();
  double energy2 = step->GetPostStepPoint()->GetKineticEnergy();
  double energy=(energy1+energy2)/2;
  const G4ParticleDefinition* partname = step->GetTrack()->GetDefinition();//->GetParticleName();

  // Compute the dedx for the current particle in the current material
  double weightedLET =0;
  G4double dedx = emcalc->ComputeElectronicDEDX(energy, partname, material);
    //G4cout<<"in here"<<G4endl; 
        //G4cout<<energy <<" " << dedx <<G4endl;
   if (mIsVerboseStoppingPower) {
       //G4cout<<"in here"<<G4endl;
        //G4cout<<"dedx pre: " << emcalc->ComputeElectronicDEDX(energy1, partname, material)<<G4endl;
        //G4cout<<"dedx mid: " << emcalc->ComputeElectronicDEDX(energy, partname, material)<<G4endl;
        //G4cout<<"edep/dx : "<<edep/steplength<<G4endl;
        //G4cout<<"Energy pre: "<<energy1/MeV <<G4endl;
        //G4cout<<"Energy mid: "<<energy <<G4endl;
        //G4cout<<"Energy post: "<<energy2<<G4endl;
        //G4cout<<"StepLength: "<<steplength/mm<<G4endl;
        //G4cout<<"E1 - dx*dedx(E1) :" << energy1 - steplength/mm*emcalc->ComputeElectronicDEDX(energy1, partname, material) <<G4endl;
        //G4cout<<"E1 - dx*dedx(Eq) :" << energy1 - steplength/mm*dedx<<G4endl<<G4endl;
        ////G4NistManager* man=G4NistManager::Instance();
        ////G4Material* Water = man->FindOrBuildMaterial("Water");
        ////G4cout<<"mean excitation: "<< Water->GetIonisation()->GetMeanExcitationEnergy()/eV<<G4endl;
        //G4cout<<energy2<< " "<< emcalc->ComputeElectronicDEDX(energy2, partname, material)<<G4endl;
        //G4double GetElementCrossSection(const G4DynamicParticle*, G4int Z,
				  //const G4Material* mat = 0);
        //G4double GetElementCrossSection(step->GetTrack()->GetDynamicParticle()
    
   }

  if (dedx==0){
    GateDebugMessage("Actor", 5, "GateLETActor dedx == 0 : do nothing\n");
    return;
  }

  double normalizationVal = 0;
  if (mIsDoseAverageDEDX) {
    weightedLET=edep*dedx; // /(density/(g/cm3));
    normalizationVal = edep;
  }
  else if (mIsTrackAverageDEDX) {
    weightedLET=dedx*steplength;
    normalizationVal = steplength;
  }
  else if (mIsFioriniFluenceFilm) {
      double g_E = 2.13 - 1.12/ ( 1+ pow((1.25/energy), 2.05));
    weightedLET=g_E*steplength;
    normalizationVal = steplength;
  }
  else if (mIsPalmansDoseFilm) {
      double g_E = 2.13 - 1.12/ ( 1+ pow((1.25/energy), 2.05));
    weightedLET=edep/g_E;
    normalizationVal = edep;
  }
    else if (mIsFioriniFluenceWater) {
      double g_E = 1 + exp(-pow(log(energy/0.4)/1.49, 2));
    weightedLET=g_E*steplength;
    normalizationVal = steplength;
  }
  else if (mIsPalmansDoseWater) {
      double g_E = 1 + exp(-pow(log(energy/0.4)/1.49, 2));
    weightedLET=edep/g_E;
    normalizationVal = edep;
  }
  else if (mIsCreatorProcess){
    
      if (step->GetTrack()->GetCreatorProcess() )
      {
            G4cout<<step->GetTrack()->GetParticleDefinition()->GetParticleName()<<G4endl;
          if (strcmp(step->GetTrack()->GetCreatorProcess()->GetProcessName(), "hadElastic") == 0){
            G4cout<<"process: hadElastic; control: "<<step->GetTrack()->GetCreatorProcess()->GetProcessName() <<G4endl;
            G4cout<<"process Type (elastic) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessType() <<G4endl;
            G4cout<<"process SubType (elastic) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessSubType() <<G4endl;
            }
          else if (strcmp(step->GetTrack()->GetCreatorProcess()->GetProcessName(), "protonInelastic") == 0){
            G4cout<<"process: prot Inelastic; control: "<<step->GetTrack()->GetCreatorProcess()->GetProcessName() <<G4endl;
            G4cout<<"process Type (inelastic) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessType() <<G4endl;
            G4cout<<"process SubType (inelastic) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessSubType() <<G4endl;
            }
          else {
            G4cout<<"process: "<<step->GetTrack()->GetCreatorProcess()->GetProcessName() <<G4endl;
            G4cout<<"process Type (else) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessType() <<G4endl;
            G4cout<<"process SubType (else) : "<<step->GetTrack()->GetCreatorProcess()->GetProcessSubType() <<G4endl;
            }
        
      }
  }
  else if (mIsKillParticle) {
     
     if (  step->GetTrack()->GetParentID() > 0){
        //step->GetTrack()->SetTrackStatus(fStopAndKill);
        G4double thisCutVal=1.0;
        step->GetTrack()->SetKineticEnergy(thisCutVal*eV);
        //G4cout<<"kill part"<<G4endl;
        //G4cout<<"get kin energy"<< step->GetTrack()->GetKineticEnergy()*MeV<<" MeV"<<G4endl;
        weightedLET=edep;
    }
    normalizationVal = 1;
  }
  else if (mIsTrackAverageEdepDX) {
    weightedLET=edep;
    normalizationVal = steplength;
  }
  else if (mIsDoseAverageEdepDX) {
    weightedLET=edep*edep/steplength;
    normalizationVal = edep;
  }
  else if (mIsAverageKinEnergy) {
    weightedLET=steplength*energy*weight;
    normalizationVal = steplength;
  }
  else if (mIsAlpha) {
        
   double alpha_0 =0.112;
  double lambda_wilkensModel = 0.02;
    G4double dedx_star = dedx;
    if (dedx > 30){
        if (mIsAlphaLinearOverkillSaturation){
            dedx_star = 30;
        }
        else if (mIsAlphaLinearOverkillReverse){
            dedx_star = 60 - dedx;
            if (dedx_star <= 0){
                dedx_star = 1;
            }
        }
    }
    G4double alpha_i = alpha_0 +  lambda_wilkensModel*dedx_star;
    weightedLET=edep*alpha_i;
    normalizationVal =edep ;
  }

  if (mIsLETtoWaterEnabled){
    weightedLET = (weightedLET/dedx)*	emcalc->ComputeTotalDEDX(energy, partname->GetParticleName(), "G4_WATER") ;
  }

  mWeightedLETImage.AddValue(index, weightedLET);
  mNormalizationLETImage.AddValue(index, normalizationVal);

  GateDebugMessageDec("Actor", 4, "GateLETActor -- UserSteppingActionInVoxel -- end\n");
}
//-----------------------------------------------------------------------------
