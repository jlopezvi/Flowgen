// VinciaShower.cc is a part of the VINCIA plug-in to the PYTHIA 8 generator.
// VINCIA is licenced under the GNU GPL version 2.
// Copyright (C) 2014 P. Skands.
// Please respect the MCnet Guidelines for Event Generator Authors and Users, 
//               http://www.montecarlonet.org/GUIDELINES

// Function definitions (not found in the header) for the VinciaShower class, 
// as follows:
// 1) inherited member functions from the Pythia 8 TimeShower classs
// 2) non-inherited member functions for the VinciaShower class
// 3) member functions for the Branchelemental class

// Include the Vincia headers 
#include "VinciaShower.h"
#include "VinciaSubtractionTerms.h"

using namespace Pythia8;

namespace Vincia {
  
  //**********************************************************************
  
  // The definition of the TimeShowerInterface and SpaceShowerInterface 
  // methods
  
  //**********************************************************************
  
  // implementation of TimeShowerInterface methods
  
  // Initialize alphaStrong and related pTmin parameters.
  void TimeShowerInterface::init(BeamParticle* beamAPtrIn, 
                                 BeamParticle* beamBPtrIn) {
    showerPtr->initTimeShower(beamAPtrIn, beamBPtrIn);
  }
  
//  // Top-level routine to do a full time-like shower in resonance decay.
//  int TimeShowerInterface::shower( int iBeg, int iEnd, Event& event, 
//                                  double pTmax, int nBranchMax ){
//    return showerPtr->showerTimeShower(iBeg, iEnd, event, pTmax, nBranchMax);
//  }
  
  // Prepare system for evolution.
  void TimeShowerInterface::prepare(int iSys, Event& event,
                                    bool limitPTmaxIn ){
    showerPtr->prepareTimeShower( iSys, event, limitPTmaxIn );
  }
  
  // Update antenna list after each ISR emission.  
  void TimeShowerInterface::update( int iSys, Event& event, bool hasWeakRad){
    showerPtr->updateTimeShower( iSys, event, hasWeakRad );
  }
  
  // Select next pT in downwards evolution.
  // the argument nRadInt is ignored at present
  double TimeShowerInterface::pTnext( Event& event, double pTbegAll, 
                                     double pTendAll, bool isFirstTrial){
    return showerPtr->pTnextTimeShower( event, pTbegAll, pTendAll,
                                        isFirstTrial );
  }
  
  // Branch event
  bool TimeShowerInterface::branch( Event& event, bool isInterleaved){
    return showerPtr->branchTimeShower( event, isInterleaved);
  }
  
  // Update antenna list after a multiple interactions rescattering.
  void TimeShowerInterface::rescatterUpdate( int iSys, Event& event){
    showerPtr->rescatterUpdateTimeShower( iSys, event);
  }
  
  // Utility to print antenna list; for debug mainly.
  void TimeShowerInterface::list( ostream& os ) const {
    showerPtr->listTimeShower( os );
  }
  
  // implementation of SpaceShowerInterface methods
  
  // Initialize alphaStrong and related pTmin parameters.
  void SpaceShowerInterface::init(BeamParticle* beamAPtrIn, 
                                  BeamParticle* beamBPtrIn) {
    showerPtr->initSpaceShower(beamAPtrIn, beamBPtrIn);
  }
  
  // Prepare system for evolution; identify ME.
  void SpaceShowerInterface::prepare( int iSys, Event& event, bool ){
    showerPtr->prepareSpaceShower( iSys, event );
  }
  
  // Select next pT in downwards evolution.
  // the argument nRadInt is ignored at present
  double SpaceShowerInterface::pTnext(
    Event& event, double pTbegAll, double pTendAll, int nRadIn ){
    return showerPtr->pTnextSpaceShower( event, pTbegAll, pTendAll, nRadIn );
  }
  
  // Branch event
  bool SpaceShowerInterface::branch( Event& event){
    return showerPtr->branchSpaceShower( event );
  }
  
  // Utility to print antenna list; for debug mainly.
  void SpaceShowerInterface::list( ostream& os ) const {
    showerPtr->listSpaceShower( os );
  }
  
  //**********************************************************************
  
  // The VinciaShower class and its members
  
  //**********************************************************************
  
  // BEGIN INHERITED FUNCTIONS FOR INTERFACE WITH PYTHIA8
  //  : init     : global shower initialization function
  //  : shower   : perform shower off partonic system
  //  : prepare  : prepare partonic system for showering
  //  : update   : purely dummy
  //  : rescatterUpdate : purely dummy
  //  : rescatterPropagateRecoil : purely dummy
  //  : pTnext   : generate next scale in downwards evolution
  //  : branch   : perform a branching, store results in event record
  
  //*********
  
  // Initialize Shower
  
  void VinciaShower::init(BeamParticle* beamAPtrIn, BeamParticle* beamBPtrIn) {
    
    // Set state variables which are independent of settings
    headerIsPrinted = false;
    rescatterFail(false);
    
    // Initialize the event handler (is called first with null pointers
    //   from TimeShower, then with the real pointers from SpaceShower)
    evPtr->init(beamAPtrIn, beamBPtrIn);
    
    // Check if already initialized
    if (isInit) return;
    
    // Verbose level and version number
    verbose            =settingsPtr->mode("Vincia:verbose");
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::init","begin --------------");
    }
    
    version            =settingsPtr->parm("Vincia:versionNumber");
    
    vinciaOn = settingsPtr->flag("Vincia");
    
    doFSR = settingsPtr->flag("Vincia:FSR");
    doISR = settingsPtr->flag("Vincia:ISR");
    maxPartonMultiplicity = settingsPtr->mode("Vincia:maxPartonMultiplicity");
    
    printAfterBranching = settingsPtr->flag("Vincia:printAfterBranching");
    
    relativisticSpeed = settingsPtr->parm("Vincia:relativisticSpeed");
    
    // fill the vector with the information about the pdf set and the uncertainty variations
    
    useLHAPDF.push_back(settingsPtr->flag("PDF:useLHAPDF"));
    pSet.push_back(settingsPtr->mode("PDF:pSet"));
    LHAPDFset.push_back(settingsPtr->word("PDF:LHAPDFset"));
    LHAPDFmember.push_back(settingsPtr->mode("PDF:LHAPDFmember"));
    
    // Uncertainty Bands
    uncertaintyBands    =settingsPtr->flag("Vincia:uncertaintyBands");
    
    if (uncertaintyBands) {
      for (int iVar = 1; iVar <= NVARALPHASPDF; ++iVar) {
        useLHAPDF.push_back(
                            settingsPtr->flag("Vincia:useLHAPDFAlt:"+num2str(iVar,0)));
        pSet.push_back(settingsPtr->mode("Vincia:pSetAlt:"+num2str(iVar,0)));
        LHAPDFset.push_back(
                            settingsPtr->word("Vincia:LHAPDFsetAlt:"+num2str(iVar,0)));
        LHAPDFmember.push_back(
                               settingsPtr->mode("Vincia:LHAPDFmemberAlt:"+num2str(iVar,0)));
      }
    }
    
    useCollinearTerms = settingsPtr->flag("Vincia:useCollinearTerms");
    useFiniteTerms = settingsPtr->flag("Vincia:useFiniteTerms");
    
    // Default evolution parameters
    evolutionType     = settingsPtr->mode("Vincia:evolutionType");
    // Decide whether to force strong (evolution) ordering or not
    orderingMode      = settingsPtr->mode("Vincia:orderingMode");
    // Number of active quark flavours
    nGluonToQuark     = settingsPtr->mode("Vincia:nGluonToQuark");
    // Normalization factor for evolution variable
    pTnormalization   = settingsPtr->parm("Vincia:pTnormalization");
    mDnormalization   = settingsPtr->parm("Vincia:mDnormalization");
    qNormalization    = evolutionType == 2 ? mDnormalization : pTnormalization;
    normalizeInputPT  = settingsPtr->flag("Vincia:normalizeInputPT");
    
    // Whether to use first-order running trial alphaS
    runningAlphaStrial = settingsPtr->flag("Vincia:runningAlphaStrial");
    // If the evolution variable is not the argument of alphaS, fix alphaStrial
    if (settingsPtr->mode("Vincia:alphaSmode") >= 1 &&
        (evolutionType != 1 && evolutionType != 20)) {
      if (verbose >= 4) {
        printOut("VS::init", "Evolution not in argument of alphaS => "
                             "use fixed trial alphaS");
      }
      runningAlphaStrial = false;
    }
    
    // Whether to use the Ariadne factor for gluon splitting in global shower
    gluonSplittingCorrection =
      settingsPtr->flag("Vincia:gluonSplittingCorrection");
    
    // Recoil strategy for initial-final emissions
    kineMapTypeIF = settingsPtr->mode("Vincia:kineMapTypeIF");
    
    // Recoil strategy for initial-initial emissions
    kineMapTypeII =settingsPtr->mode("Vincia:kineMapTypeII");
    
    // Decide whether to impose sector ordering or not
    sectorShower       = settingsPtr->flag("Vincia:sectorShower");
    useSectorTerms     = settingsPtr->flag("Vincia:useSectorTerms");
    if ( !settingsPtr->flag("Vincia:useCollinearTerms") && sectorShower) {
      printOut("VS::init","Note - useCollinearTerms = off"
               ", so also forcing useSectorTerms = off");
      useSectorTerms = false;
    }
    
    // global flag for polarization
    helicityShower= settingsPtr->flag("Vincia:helicityShower");
    
    // RESTRICT SECTOR SHOWER FLEXIBILITY
    if (sectorShower) {
      if (evolutionType != 1) {
        printOut("VS::init",
                 "Note - forcing evolutionType = 1 for sector showers");
        evolutionType = 1;
      }
      if (orderingMode != 0) {
        printOut("VS::init",
                 "Note - forcing orderingMode = 0 for sector showers");
        orderingMode = 0;
      }
    }
    
    // fill vector of trial generators
    tgPtrs.push_back(&trialSoft);
    
    tgPtrs.push_back(&trialSplitI);
    tgPtrs.push_back(&trialSplitK);
    
    tgPtrs.push_back(&trialSoftIF);
    tgPtrs.push_back(&trialCollInIF);
    tgPtrs.push_back(&trialCollOutIF);
    tgPtrs.push_back(&trialSoftFI);
    tgPtrs.push_back(&trialCollInFI);
    tgPtrs.push_back(&trialCollOutFI);
    
    tgPtrs.push_back(&trialSplitKIF);
    tgPtrs.push_back(&trialSplitIFI);
    
    tgPtrs.push_back(&trialSoftII);
    tgPtrs.push_back(&trialCollIII);
    tgPtrs.push_back(&trialCollKII);
    
    tgPtrs.push_back(&trialConvertGIF);
    tgPtrs.push_back(&trialConvertGFI);
    tgPtrs.push_back(&trialConvertGIII);
    tgPtrs.push_back(&trialConvertGKII);
    
    tgPtrs.push_back(&trialConvertQIF);
    tgPtrs.push_back(&trialConvertQFI);
    tgPtrs.push_back(&trialConvertQIII1);
    tgPtrs.push_back(&trialConvertQIII2);
    tgPtrs.push_back(&trialConvertQIII3);
    tgPtrs.push_back(&trialConvertQKII1);
    tgPtrs.push_back(&trialConvertQKII2);
    tgPtrs.push_back(&trialConvertQKII3);
    
    for (vector<TrialGenerator*>::const_iterator i = tgPtrs.begin();
         i != tgPtrs.end(); ++i) {
      (*i)->initPtr(rndmPtr, settingsPtr, distPtr);
      (*i)->init();
    }
    
    // Initialise triGenMapGlob
    initTriGenMapGlob();
    
    pdfScaleFactor = settingsPtr->parm("Vincia:PDFscaleFactor");
    pdfMuFmin      = settingsPtr->parm("Vincia:PDFmuFmin");
    pdfThresholdB  = settingsPtr->parm("Vincia:PDFThresholdB");
    pdfThresholdC  = settingsPtr->parm("Vincia:PDFThresholdC");
    
    // The evolution window boundaries
    qMinVecSav.push_back(25*pdfThresholdB);
    qMinVecSav.push_back( 5*pdfThresholdB);
    qMinVecSav.push_back(   pdfThresholdB);
    qMinVecSav.push_back(   pdfThresholdC);
    
    // Perturbative cutoff
    cutoffType         = settingsPtr->mode("Vincia:cutoffType");
    if (cutoffType == 0) cutoffType = evolutionType;
    if (evolutionType == 3 && cutoffType != evolutionType) {
      printOut("VS::init",
               "Warning - forced cutoffType = 1 to regulate energy ordering");
      cutoffType = 1;
    }
    if (evolutionType == 20 && cutoffType != evolutionType) {
      printOut("VS::init",
               "Warning - forced cutoffType = evolutionType (=20)");
      cutoffType = evolutionType;
    }
    cutoffScale        =settingsPtr->parm("Vincia:cutoffScale");
    cutoffTypeNonEmit  =settingsPtr->mode("Vincia:cutoffTypeNonEmit");
    if (cutoffTypeNonEmit == 1) {
      printOut("VS:init", "Mode 1 for cutoffTypeNonEmit not implemented, "
                          "using mode 3.");
      cutoffTypeNonEmit = 3;
    }
    else if (cutoffTypeNonEmit == 2) {
      printOut("VS:init", "Mode 2 for cutoffTypeNonEmit can get trapped, "
                          "using mode 3.");
      cutoffTypeNonEmit = 3;
    }
    
    int nonEmitCutoffScaleMode =
    settingsPtr->mode("Vincia:nonEmitCutoffScaleMode");
    double nonEmitCutoffScaleFactor =
    settingsPtr->parm("Vincia:nonEmitCutoffScaleFactor");
    if (nonEmitCutoffScaleMode == 1) {
      cutoffScaleNonEmit = nonEmitCutoffScaleFactor*cutoffScale;
    } else if (nonEmitCutoffScaleMode == 2) {
      cutoffScaleNonEmit =
      nonEmitCutoffScaleFactor*settingsPtr->parm("StringPT:sigma");
    }
    
    mStringMin =settingsPtr->parm("Vincia:mStringMin");
    
    secondaryHadronizationVetos =
    settingsPtr->flag("Vincia:secondaryHadronizationVetos");
    
    uncertaintyBandsKmu =settingsPtr->parm("Vincia:uncertaintyBandsKmu");
    
    // Number of uncertainty variations (including USR)
    // Hardcoded here, no sense to make user specifiable, since user
    // can see a table of which variations are being performed, and can
    // then just choose the weight sets he or she wants to consider.
    nWeights = 1;
    allowReweighting =settingsPtr->flag("Vincia:allowReweighting");
    if (uncertaintyBands) nWeights = 15;
    weightLimit =settingsPtr->parm("Vincia:uncertaintyLimit");
    // Resize weight vectors to the relevant number of elements
    weightSave.resize(nWeights);
    weightSumSave.resize(nWeights);
    weightSum2Save.resize(nWeights);
    weightOld.resize(nWeights);
    weightMaxSave.resize(nWeights);
    for (int iVar=0; iVar<nWeights; iVar++) weightMaxSave[iVar]=0.0;
    
    // Hyperjet compatibility mode off (not implemented)
    hyperjet           = settingsPtr->flag("Vincia:hyperjet");
    if (hyperjet) {
      printOut("VinciaShower::init", "Deactivated hyperjet (not implemented)");
      hyperjet = false;
    }
    
    // Initialize Evolution Variable
    resolutionPtr->init();
    
    // Matching settings
    matchingLO         = settingsPtr->mode("Vincia:matchingLO");
    matching           = matchingLO >= 1;
    matchingNLO        = min(matchingLO,
                             settingsPtr->mode("Vincia:matchingNLO"));
    matchingFullColor  = settingsPtr->flag("Vincia:matchingFullColor");
    matchingRegOrder   = settingsPtr->mode("Vincia:matchingRegOrder");
    // Matching beyond first order can only be done for smoothly ordered
    // showers. Switch off matching beyond first order if user has selected
    // strong ordering (since the goal is then presumably to look at pure
    // showers anyway).
    if (orderingMode == 1 && matchingLO >= 2) {
      printOut("VS::init","warning; forcing matchingLO <= 1 "
               "for strongly ordered showers");
      matchingLO  = min(matchingLO,1);
      matchingNLO = min(matchingNLO,1);
    }
    // Matching scale can only be imposed at highest matched multiplicity
    // or not at all. (Matching assumes all previous multiplicities were
    // matched over all of phase space.)
    if (matchingRegOrder <= matchingLO -1) {
      printOut("VS::init","warning; forcing matchingRegOrder >= "
               "matchingLO");
      matchingRegOrder = max(matchingLO,matchingRegOrder);
    }
    // NLO matching: virtual correction assumes real corrections are
    // matched over all of phase space.
    if (matchingRegOrder <= matchingNLO) {
      printOut("VS::init","warning; forcing matchingRegOrder >= "
               "matchingNLO + 1");
      matchingRegOrder = max(matchingNLO+1,matchingRegOrder);
    }
    matchingRegShape   =settingsPtr->mode("Vincia:matchingRegShape");
    matchingRegType    =settingsPtr->mode("Vincia:matchingRegType");
    matchingScaleIsAbs =settingsPtr->flag("Vincia:matchingRegScaleIsAbsolute");
    matchingScale      =settingsPtr->parm("Vincia:matchingRegScale");
    matchingScaleRatio =settingsPtr->parm("Vincia:matchingRegScaleRatio");
    matchingIRcutoff  = settingsPtr->parm("Vincia:matchingIRcutoff");
    MESav = -1;
    MENew = 0.0;
    
    // Initialize headroom factors (optimized dynamically later)
    setHeadRoomEmit(1.5);
    setHeadRoomSplit(2.0);
    
    // Internal Histograms
    if (verbose >= 2) initVinciaHistos();
    
    // alphaS
    alphaSvalue        = settingsPtr->parm("Vincia:alphaSvalue");
    alphaSorder        = settingsPtr->mode("Vincia:alphaSorder");
    alphaSmode         = settingsPtr->mode("Vincia:alphaSmode");
    alphaScmw          = settingsPtr->flag("Vincia:alphaScmw");
    alphaSkMu          = settingsPtr->parm("Vincia:alphaSkMu");
    alphaSmuFreeze     = settingsPtr->parm("Vincia:alphaSmuFreeze");
    alphaScmwDef       = settingsPtr->flag("Vincia:alphaScmw");
    alphaSkMuDef       = settingsPtr->parm("Vincia:alphaSkMu");
    
    alphaSmax   =settingsPtr->parm("Vincia:alphaSmax");
    // Check freezeout scale
    if (alphaSorder >= 1) {
      //alphaSmuFreeze = max(alphaSmuFreeze,1.05*Lambda3);
      alphaSmax   = min(alphaSmax,alphaSptr->alphaS(pow2(alphaSmuFreeze)));
    }
    
    // Colour reconnections
    CRmode        = settingsPtr->mode("Vincia:CRmode");
    CRjunctions   = settingsPtr->flag("Vincia:CRjunctions");
    colourReconnectionsPtr->init();
    
    // Initialize nNegativeWeight, nNonunityWeight and nTotWeights to 0
    nNegativeWeight = 0;
    nNonunityWeight = 0;
    nTotWeights     = 0;
    
    // Initialize negative-weight and non-unity warning flags
    reweightingOccurred = false;
    setWeightAll(1.0);
    resetWeightSums(0.0);
    reweightMaxPSaveP      = 0.0;
    reweightMaxPSaveQ      = 0.0;
    reweightMaxQSaveP      = 0.0;
    reweightMaxQSaveQ      = 0.0;
    
    // Initialize VinClu and MadGraph interface objects,
    vinCluPtr->init();
    // Check MG interface and initialize matching class
    isMGOn = settingsPtr->flag("Vincia:MGInterface");
    if (isMGOn) {
      isMGOn = matchingPtr->init(nWeights);
      if (!matching && helicityShower) isMGOn = mgInterfacePtr->init();
      settingsPtr->flag("Vincia:MGInterface",isMGOn);
      if (isMGOn && verbose >= 2) {
        printOut("VinciaPlugin()","MGInterface switched on.");
      }
    }
    
    // Initialize USR antennae. Not done before to make sure we catch if
    // something was changed in Settings before.
    antennaSets[0]->init(false);
    antennaSetsSct[0]->init(true);
    
    // Initialize ofstream for accepted/rejected trial information if needed
    dumpTrialInfo =settingsPtr->flag("Vincia:dumpTrialInformation");
    if (dumpTrialInfo) {
      string fileName =settingsPtr->word("Vincia:dumpTrialInformationFile");
      trialInfoFile.open(fileName.c_str());
    }
    
    // Print VINCIA header and list of parameters
    if (verbose >= 1 && !headerIsPrinted) header();
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::init","end --------------");
    }
    
    // Is initialized now
    isInit = true;
    
  }
  
  //*********
  
  // Prepare system for evolution.
  
  void VinciaShower::prepare(int iSys, Event& event, bool /*limitPTmaxIn*/) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::prepare","begin --------------");
    }

    bool polarize =settingsPtr->flag("Vincia:helicityShower");
    evPtr->prepare(iSys, event, matchingPtr, polarize,
                   colourReconnectionsPtr, true);
    treatAsMassless = evPtr->isRelativistic(iSys);
    
    //If verbose, list event before showering
    if (verbose >= 4) {
      if (verbose >= 6 || iSys == 0) event.list();
      printOut("VS::prepare","preparing system no. "
               +num2str(iSys)+" for showering.");
    }
    
    // Get generated event weight from Pythia.
    // Problem: for e+e- the first one is 1
    //          for dijets, the first one is 0. 
    //          how to tell the first time we are being called for an event?
    if (iSys <= 1) {
      setWeightAll(evPtr->getStartingWeight(iSys));
      for (int iVar=0; iVar<nVariations(); iVar++) {
        weightOld[iVar] = 0.0;
      }
    }
    
    // qRestart does not impose a restriction at the start
    qRestart = numeric_limits<double>::max();
    
    // set starting scale of the antennae and set unOrdered if on
    for (int iAnt = 0; iAnt < evPtr->nAnt(); ++iAnt) {
      // Use the kinematic maximum for FF antennae
      if (evPtr->isFF(iAnt)) {
        evPtr->setQold(iAnt, evPtr->qKineMax(iAnt));
      }
      // For IF and II, use the antenna mass
      // TODO: The normalization of the evolution variable should enter, how?
      else {
        evPtr->setQold(iAnt, evPtr->getmAbs(iAnt));
      }
      if (orderingMode == 0 || orderingMode >= 2) {
        evPtr->setUnOrdered(iAnt, true);
      }
    }

    // Initialize scales for ordering variation uncertainty bands
    // (scale of last trial)
    pT2trialSav = 0.0;
    mD2trialSav = 0.0;
    q2trialSav  = 0.0;
    for (int iAnt = 0; iAnt < evPtr->nAnt(); ++iAnt) {
      pT2trialSav = max(pT2trialSav, evPtr->q2kineMaxEmit(iAnt, 1));
      mD2trialSav = max(mD2trialSav, evPtr->q2kineMaxEmit(iAnt, 2));
      q2trialSav  = max(q2trialSav,  evPtr->q2kineMaxEmit(iAnt, evolutionType));
    }
    

    //If verbose output, list antennae
    if (verbose >= 5) list();    
    
    // Note that we have not yet binned the first branching or trial scale
    firstQBranchBinned = false;
    firstQTrialBinned  = false;
    
    // If uncertainty variations are active, set pdf & alphaS variations
    if (uncertaintyBands) {
      // WARNING: the alphaS and pdf values I get from  the info pointer
      // do NOT agree with what I get by calling alphaS or the pdf's myself
      // (possibly due to alphaS for the cross-section not set correctly?)
      
      int exp = alphaSExponent(infoPtr);
      double Q2Ren = infoPtr->Q2Ren();
      double alphaSOrig = alphaSptr->alphaSofQ2(Q2Ren);
      double mu2F = infoPtr->Q2Fac();
      int id1 = infoPtr->id1();
      int id2 = infoPtr->id2();
      double x1 = infoPtr->x1();
      double x2 = infoPtr->x2();
      
      for (int iVar = 1; iVar < nVariations(); ++iVar) {
        double alphaScorr = 1.0;
        int iAlphaS = getiAlphaS(iVar);
        if (iAlphaS != 0) {
          double alphaSV = alphaSptr->alphaSofQ2(Q2Ren, iAlphaS);
          alphaScorr = pow(alphaSV/alphaSOrig, exp);
        }
        weightSave[iVar] *= alphaScorr;
      }
      
      if (evPtr->hasIncomingColouredParticles(iSys)) {
        double pdfOrig = evPtr->getPDFA(0, id1, x1, mu2F, 0)
                        *evPtr->getPDFB(0, id2, x2, mu2F, 0);
        for (int iVar = 1; iVar < nVariations(); ++iVar) {
          double pdfCorr = 1.0;
          int iPDF    = getiPDF(iVar);
          if (iPDF != 0) {
            double pdfV = evPtr->getPDFA(0, id1, x1, mu2F, iPDF)
            *evPtr->getPDFB(0, id2, x2, mu2F, iPDF);
            pdfCorr = pdfV/pdfOrig;
            // Bound the pdf correction factor by 0.1 and 10, mostly to avoid
            // 0 or infinity due to heavy quark thresholds, have to capture NaN
            if      (!(0.1 < pdfCorr) &&  (pdfCorr < 10)) { pdfCorr =  0.1; }
            else if ( (0.1 < pdfCorr) && !(pdfCorr < 10)) { pdfCorr = 10.0; }
            else if (!(0.1 < pdfCorr) && !(pdfCorr < 10)) { pdfCorr =  1.0; }
          }
          
          weightSave[iVar] *= pdfCorr;
        }
      }
    }
  }

//*********

// Select next pT in downwards evolution of the existing antennae.

double VinciaShower::pTnext(Event& event, double pTevolBegAll,
                            double pTevolEndAll) {
  // Verbose output
  if (verbose >= 6) {
    printOut("VinciaShower::pTnext","begin --------------");
  }
  
  //$ Check for reasons not to do anything
  if (evPtr->nAnt() == 0 ||
      (maxPartonMultiplicity >= 0 && evPtr->nPart() >= maxPartonMultiplicity)||
      (!(doFSR || doISR))) {
    return 0.;
  }
    
  // Translate input scales to VINCIA normalization (if asked for)
  // Denote VINCIA scales by "q", PYTHIA ones by "pTevol".
  double qBegAll = translateFrompTevol(pTevolBegAll);
  // End scale: (qEndAll is obtained from the input parameter pTevolEndAll)
  double qEndAll = translateFrompTevol(pTevolEndAll);
  
  //$ For strong ordering, ensure our starting scale is below all old scales
  if (orderingMode == 1) { qBegAll= min( qBegAll, evPtr->getQoldMin() ); }

  // Initialize winner scale
  double qWin = 0.0;
    
  // Verbose output
  if (verbose >= 4) {
    printOut("VS::pTnext","(re)starting evolution at scale Q = "
             +num2str(qBegAll));
    if (verbose >= 6)  {
      list();
    }
  }
  
  //$ Get trial scale for each antenna, the highest one is our branching scale
  int iWinner = 0;
  for (int iAnt = 0; iAnt < evPtr->nAnt(); ++iAnt) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VinciaShower::pTnext","processing antenna no."+num2str(iAnt,4));
    }

    // For unordered antennae, start from kinematic maximum (or from scale of
    // last failed trial, which is stored in qRestart)
    double qMax = evPtr->qKineMax(iAnt);
    double qBegin = min(evPtr->isUnordered(iAnt) ? qRestart : qBegAll, qMax);
    double qEnd = max(qEndAll, qCut(iAnt));

    double qTrial = qBegin;
    printOut(8, "VinciaShower::pTnext", "| qBegin = " + num2str(qBegin));
    
    //$ [does the anntenna have a saved trial scale we can still use?]
    if (evPtr->getTrialScale(iAnt) != 0.0    &&
        evPtr->getTrialScale(iAnt) <  qBegin) {
      //$1 Lookup saved trial scale
      qTrial = evPtr->getTrialScale(iAnt);
      if (verbose >= 8) {
        printOut("VinciaShower::pTnext","| Trial QE = "+num2str(qTrial)+
                 " retrieved");
      }
    }
    //$ [is the antenna ready for hadronization?]
    else if (evPtr->isReadyForHad(iAnt)) {
      //$1 set the trial scale to zero
      qTrial = 0.0;
      if (verbose >= 8) {
        printOut("VinciaShower::pTnext","| Antenna ready for hadronization");
      }
    }
    //$ [is the starting scale below the stopping scale?]
    else if (qBegin <= qEnd) {
      if (verbose >= 7) {
        printOut("VinciaShower::pTnext","| qBegin = "
                 +num2str(qBegin)+" < qEnd = "+num2str(qEnd)
                 +" (qCut = " + num2str(qCut(iAnt), 0) + ", qEndAll = "
                 +num2str(qEndAll, 0) + "). qTrial = 0.0");
      }
      //$1 set the trial scale to zero
      qTrial = 0.0;
      // Set antenna as being ready for hadronization if below qCut
      if (qBegin < qCut(iAnt)) { evPtr->setReadyForHad(iAnt, true); }
    }
    else {
      // Generate new trial scale
      
      // TODO: find out what this is supposed to do
      /*
       // Sector showers with pT-evolution: check smallest unconnected
       // gluon-emission pT to use as absolute upper bound on Emit evolution.
       double qT2sectorMin = pow2(qBegin);
       if (sectorShower && evolutionType == 1) {
       int iOld1 = trialPtr->iOld1;
       int iOld2 = trialPtr->iOld2;
       for (int iSct = 0; iSct < int(branchelementals.size()); ++iSct) {
       // Skip self
       if (iSct == iAnt) continue;
       // Only compute Emit scales: skip if ID2 is not g
       if (branchelementals[iSct].old2.id() != 21) continue;
       // Skip connected
       if (branchelementals[iSct].iOld2 == iOld1
       || branchelementals[iSct].iR == iOld1
       || branchelementals[iSct].iOld1 == iOld2) continue;
       // Check if this sector has smallest sector measure
       int ia = branchelementals[iSct].iOld1;
       int ir = branchelementals[iSct].iOld2;
       int ib = branchelementals[iSct].iR;
       qT2sectorMin =
       min(qT2sectorMin, resolutionPtr->Q2E(event[ia],event[ir],event[ib],1));
       
       }
       }
       */
      
      // Need to check interaction with sector vetos to reactivate this
      /*
       // For sector showers, compute current sector resolution scale
       if (sectorShower) { QS2Min = evPtr->getSectorR(iSysSel(), 10, 8); }
       */
      
      // For sector showers, allow several failed iterations when QE
      // tells us beforehand we will be outside acceptable sector
      
      // If the antenna is ready for hadronization, the trial scale is 0
      if (evPtr->isReadyForHad(iAnt)) { qTrial = 0.0; }
      
      // Initialise trial generators if necessary
      if (evPtr->nTrialGenerators(iAnt) == 0) {
        evPtr->setTrialGenerators(iAnt, determineTrialGenerators(iAnt));
      }
      
      if (verbose >= 9) {
        printOut(9, "VinciaShower::pTnext", "anntenna " + num2str(iAnt, 0)
                 + " has trial generators:");
        cout << setprecision(1);
        for (int iGen = 0; iGen < evPtr->nTrialGenerators(iAnt); ++iGen) {
          cout << evPtr->getTrialGeneratorCoefficient(iAnt, iGen) << "*"
          << evPtr->getTrialGenerator(iAnt, iGen)->name() << endl;
        }
      }
      
      //$1 Make sure every trial generator of the antenna has a trial scale
      for (int iTrial = 0; iTrial < evPtr->nTrialGenerators(iAnt);
           ++iTrial) {
        if (evPtr->getTrialScale(iAnt, iTrial) == 0.0) {
          double qTrialNew = min(qBegin, evPtr->qKineMax(iAnt, iTrial));
          
          int idi = evPtr->getIdDau1(iAnt, iTrial);
          int idj = evPtr->getIdDau2(iAnt, iTrial);
          int idk = evPtr->getIdDau3(iAnt, iTrial);
          int idI = evPtr->getIdMot1(iAnt);
          int idK = evPtr->getIdMot2(iAnt);
          Direction di = evPtr->getDirMot1(iAnt);
          Direction dj = Direction::out;
          Direction dk = evPtr->getDirMot2(iAnt);
          
          AntennaSet* antSetPtr= sectorShower ? antennaSetsSct[0]
          : antennaSets[0];
          int antNr = antSetPtr->getiAnt(idi, idj, idk, idI, idK, di, dj, dk);
          bool antennaIsOn = antSetPtr->getAnt(antNr)->isOn();
          
          if (antennaIsOn) {
            
            double qMinAll = max(qCut(iAnt, iTrial), qEnd);
            double colFac = antSetPtr->getAnt(antNr)->getChargeFactor();
            double psVol = evPtr->getPSVolumeFactor(iAnt);
            double pAriHeadroom = pAriHeadroomFactor(iAnt, iTrial);
            double headroomFactor = psVol*pAriHeadroom;
            
            bool acceptRegion = false;
            while (!acceptRegion) {
              
              double qMinNow = max(getQmin(qTrialNew), qMinAll);
              
              double muMinRunning = getMuMinRunning(iAnt, iTrial, qMinNow);
              double alphaSmuMin = alphaSptr->alphaS(pow2(muMinRunning));
              
              if (alphaSorder == 0 || !runningAlphaStrial ||
                  muMinRunning < 2.5*alphaSptr->Lambda3() ||
                  alphaSmax < alphaSmuMin) {
                double mu2min;
                if (alphaSorder == 0) {
                  mu2min = pow2(alphaSmuFreeze);
                }
                else if (alphaSmode == 1 &&
                         (evolutionType == 20 || evolutionType == 1)) {
                  mu2min = pow2(max(muMinRunning, alphaSmuFreeze));
                }
                else {
                  // TODO: insert real lower bound for mu^2 != qT^2
                  mu2min = max(pow2(translateTopTevol(qMinNow)*alphaSkMu)/8,
                               pow2(alphaSmuFreeze));
                }
                double alphaSval = min(alphaSptr->alphaS(mu2min), alphaSmax);
                qTrialNew = evPtr->genQ(iAnt, iTrial, qTrialNew, qMinNow,
                                        colFac, alphaSval, headroomFactor);
              }
              else if (alphaSorder >= 1) {
                
                int nf = getNfAlphaSofQ2(pow2(qMinNow));
                double b0 = (33. - 2.*nf) / (12. * M_PI);
                
                double kR = alphaSkMu;
                
                double Lambda = alphaSptr->Lambda(nf);
                
                qTrialNew = evPtr->genQ(iAnt, iTrial, qTrialNew, qMinNow,
                                        colFac, b0, kR, Lambda,
                                        headroomFactor);
              }
              
              if (qTrialNew > qMinNow || qMinNow == qMinAll) {
                acceptRegion = true;
              }
              else {
                qTrialNew = qMinNow;
              }
            }
          }
          else {
            evPtr->saveTrial(iAnt, iTrial, TriGenState());
          }
        }
      }
      //$1 Select the highest scale among trial generator scales
      qTrial = evPtr->getTrialScale(iAnt);
      if (qTrial < qEnd) {
        evPtr->setReadyForHad(iAnt, true);
      }
    }
    
    //Is the trial scale of this antenna the highest so far?
    if (qTrial >= qWin || qWin <= 0.0) {
      iWinner = iAnt;
      qWin=qTrial;
    }
    
    // Verbose output
    if (verbose >= 7) {
      printOut("VinciaShower::pTnext","qTrial   = "
               +num2str(qTrial)
               +(iAnt+1 == int(evPtr->nAnt()) ?
                 "; final qWin = ":"; current qWin = ")
               +num2str(qWin));  
    }  
  }

  evPtr->setProtoStateIndex(iWinner);
  
  // Update trial generation statistics
  // Verbose output  
  if (verbose >= 5 && qWin > 0.) {
    if (verbose >= 7)  list();
  }
  // Fill the diagnostic histogram with the first trial
  if (verbose >= 2) {
    if (!firstQTrialBinned) {
      double qWinNormalized = qWin/evPtr->eCMHad();
      
      vinciaHistos["1stTrialQE/eCM"].fill(qWinNormalized);
      firstQTrialBinned = true;
    }
  }

  
  //$ [check whether branching scale is above stopping scale]
  if (qWin > qEndAll) {
    // If unordered type, update current restart scale for unordering
    if (evPtr->isUnordered((iWinner))) { qRestart = qWin; }
    
    // Verbose output
    if (verbose >= 5) {
      if (verbose >= 7)  list();
      printOut("VS::pTnext","=== Winner at scale qWin = " +num2str(qWin)
               + ", trial type "
               + evPtr->getTrialGenerator(iWinner,
                                          evPtr->getWinnerIndex(iWinner))->name()
               + ", col = "+num2str(evPtr->getCol(iWinner),5));
    }
  }
  else {
    //$ Do cleanup since the shower will stop
    qWin = 0.0;
    
    // Verbose output
    if (verbose >= 5) {
      printOut("VS::pTnext","=== All trials now below lower q limit "
               "qEndAll = "+ num2str(qEndAll,0)+".");
      if (verbose >= 6) {
        printOut("VS::pTnext","Final configuration was:");
        event.list();
      }
      if (verbose >= 4) {
        bool checksOK = evPtr->checkAll();
        if (!checksOK) {
          bool pSystemsOK = evPtr->checkPartonSystemsPointer();
          bool antOK      = evPtr->checkAntennaePartonSystems();
          bool statOK     = evPtr->checkPartonSystemsStatus();
          bool colOK      = evPtr->checkCol();
          printOut("VS::pTnext", string("Consistency checks of EventHandler ")
                   + (checksOK ? "true" : "false"));
          if (!checksOK) {
            printOut("VS::pTnext", string("pSystemsOK ") + (pSystemsOK ? "true" : "false"));
            printOut("VS::pTnext", string("antOK      ") + (antOK      ? "true" : "false"));
            printOut("VS::pTnext", string("statOK     ") + (statOK     ? "true" : "false"));
            printOut("VS::pTnext", string("colOK      ") + (colOK      ? "true" : "false"));
          }
        }
      }
    }
    // if we have not yet binned a branching scale, bin 0 as the first
    // branching scale
    if (verbose >= 2 && !firstQBranchBinned) {
      vinciaHistos["1stBranchingQE/eCM"].fill(0);
      firstQBranchBinned = true;
    }

    // The procedure below is slightly unstable.
    // Should find if it is possible to check whether a given system is
    // the last system to be considered and then update.

    // Add weight correction to cumulative sum (!= w0 if more than one system)
    double w0 = weight();
    weightSumSave[0]  += w0 - weightOld[0];
    weightSum2Save[0] += pow2(w0) - pow2(weightOld[0]);

    // For first system, add one to number of events. 
    if (weightOld[0] == 0.0) {
      nTotWeights++;
      nUnityWeight++;
    }
    
    // Check for nonunity weights
    if (abs(abs(w0)-1.0) > TINY && 
        (abs(abs(weightOld[0])-1.0) < TINY || weightOld[0] == 0)) {
        nNonunityWeight++;
        nUnityWeight--;
    }

    // Check for negative weights
    if (w0 < 0.0 && weightOld[0] >= 0.0) nNegativeWeight++;

    // Update old weight for the USR antennae
    weightOld[0] = w0;

    // Save uncertainty band weights
    if (uncertaintyBands) {
      for (int iVar=1; iVar<nVariations(); iVar++) {
        // Add weight correction to cumulative sum 
        weightSumSave[iVar]  += weight(iVar) - weightOld[iVar];
        weightSum2Save[iVar] += pow2(weight(iVar)) - pow2(weightOld[iVar]);
        // Check for new max weight
        if (weightSave[iVar] > weightMaxSave[iVar]) 
          weightMaxSave[iVar] = weightSave[iVar];
        // Update old weights
        weightOld[iVar] = weight(iVar);
      }      
    }
  }
  
  //-------------------------------------------------------------------
  //$ End of pTnext: finalize and return to PYTHIA
  // Return nonvanishing value if branching found
  // If unordered branching won, return input scale - this should make
  // the overall evolution still proceed in an ordered way.
  return min(pTevolBegAll,max(0.0,pTevol(iWinner)));
  
}
  
double VinciaShower::pTevolRescaleFactor() const {
  double rescaleFactor = 1.0;
  if (normalizeInputPT)  {
    // Vincia pT is quadratic -> take square root
    if (evolutionType == 1 || evolutionType == 20) {
      rescaleFactor = sqrt(qNormalization);
    }
    // Vincia mD is linear -> no square root
    else if (evolutionType == 2) {
      rescaleFactor = qNormalization;
    }
  }
  return rescaleFactor;
}
  
double VinciaShower::translateTopTevol(double qIn) const {
  return qIn/pTevolRescaleFactor();
}
  
double VinciaShower::translateFrompTevol(double pTevolIn) const {
  return pTevolIn*pTevolRescaleFactor();
}
  
double VinciaShower::pTevol(int iAnt) const {
  return translateTopTevol(evPtr->getTrialScale(iAnt));
}
  
  // Translate the hadronization cutoff scale to a lower bound on the
  // evolution variable for emission
  double VinciaShower::qCutEmitHad(int iAnt) const {
    double qCutoffRet = 0;
    double sAnt = evPtr->getm2Abs(iAnt);
    if (evPtr->isFF(iAnt)) {
      qCutoffRet = qMinFF(evolutionType, qNormalization, sAnt, cutoffScale,
                          cutoffType);
    }
    else if (evolutionType == cutoffType) {
      qCutoffRet = sqrt(qNormalization)*cutoffScale;
    }
    else if (evolutionType == 20 && cutoffType == 1) {
      if (evPtr->isIF(iAnt)) {
        double xA = evPtr->getxIF(iAnt);
        double sffMax = (1-xA)/xA*sAnt;
        qCutoffRet = sqrt(qNormalization)*cutoffScale*sqrt(sAnt/(sAnt+2*sffMax));
      } else if (evPtr->isII(iAnt)) {
        double sHred = evPtr->sHadReduced(iAnt) - sAnt;
        qCutoffRet = sqrt(qNormalization)*cutoffScale*sqrt(sAnt/(sAnt+2*sHred));
      }
    } else {
      throw ExGeneral("Only evolutionType 20 with cutoffType 1 implemented "
                      "for ISR");
    }
    return qCutoffRet;
  }
  
  double VinciaShower::qCutEmit(int iAnt) const {
    return qCutEmitHad(iAnt);
  }
  
  // Translate the hadronization cutoff scale for non-emission branchings
  // into a minimum for the evolution variable
  double VinciaShower::qCutSplitConvMotHad(int iAnt, int iMot) const {
    double qC = cutoffScaleNonEmit;
    double m2Ant = evPtr->getm2Abs(iAnt);
    if (cutoffType == 2) {
      return qC;
    }
    if (evPtr->isFF(iAnt)) {
      double sijMin = 0.0;
      if (cutoffType == 1 || cutoffType == 20) {
        double sq = 1-4*pow2(cutoffScale)/m2Ant;
        if (sq > 0) { sijMin = m2Ant/2*(1-sqrt(sq));               }
        else        { sijMin = numeric_limits<double>::infinity(); }
        
      }
      return sqrt(sijMin);
    } else if (evPtr->isIF(iAnt)) {
      if (evPtr->isOutgoingMot(iAnt, iMot)) {
        if (cutoffType == 1) {
          return sqrt((sqrt(pow2(m2Ant)+4*m2Ant*pow2(qC))-m2Ant)/2);
        } else if (cutoffType == 20) {
          return sqrt((sqrt(pow2(m2Ant) + 4*pow4(qC)) - m2Ant + 2*pow2(qC))/2);
        }
        
      } else {
        double xA = evPtr->getxIF(iAnt);
        double sjkMax = (1-xA)/xA*m2Ant;
        if (cutoffType == 1) {
          return qC*sqrt(m2Ant/sjkMax);
        } else if (cutoffType == 20) {
          return qC*sqrt((m2Ant+2*sjkMax)/sjkMax);
        }
      }
    } else if (evPtr->isII(iAnt)) {
      double sHred = evPtr->sHadReduced(iAnt) - m2Ant;
      if (cutoffType == 1) {
        return sqrt((sHred - sqrt(pow2(sHred)-2*m2Ant*pow2(qC)) )/2);
      } else if (cutoffType == 20) {
        return sqrt((sHred - sqrt(pow2(sHred)-4*pow2(qC)*(2*sHred+m2Ant)))/2);
      }
    }
    return 0.0;
  }
  
  double VinciaShower::qCutSplitConvMot(int iAnt, int iMom) const {
    return qCutSplitConvMotHad(iAnt, iMom);
  }
  
  double VinciaShower::qCut(int iAnt) const {
    return min(qCutEmit(iAnt),min(qCutSplitConvMot(iAnt, 1),
                                  qCutSplitConvMot(iAnt, 2)));
  }
  
  double VinciaShower::qCut(int iAnt, int iTrial) const {
    const TrialGenerator* tgPtr = evPtr->getTrialGenerator(iAnt, iTrial);
    if (tgPtr->isEmission()) {
      return qCutEmit(iAnt);
    }
    else if (tgPtr->isSplitConvertI()) {
      return qCutSplitConvMot(iAnt, 1);
    }
    else if (tgPtr->isSplitConvertK()) {
      return qCutSplitConvMot(iAnt, 2);
    }
    else {
      throw ExGeneral("VinciaShower::qCut: Trial is none of emission, "
                      "splitting/conversion I, splitting/conversion K");
    }
  }
  
  double VinciaShower::getMuMinRunning(int iAnt, int iTrial, double qMin) const
  {
    const TrialGenerator* tgPtr = evPtr->getTrialGenerator(iAnt, iTrial);
    if (tgPtr->isEmission()) {
      return translateTopTevol(qMin)*alphaSkMu;
    }
    else {
      return qMin*alphaSkMu;
    }
  }
  
  //*********
  
  bool VinciaShower::branch(Event& event, bool isInterleaved) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::branch","begin --------------");
    }
    
    int iWinner = evPtr->getProtoStateIndex();
    //$ select daughter invariants and flavours, initialize proto-state
    evPtr->initProtoState(iWinner, evPtr->genTrial(iWinner));
    if (verbose >= 2) {
      updateTrialCount(trialStat["Won"]);
    }
    
    // Extract current QE and sAnt scales
    double qNew  = evPtr->getTrialScale(iWinner);
    double Q2new = pow2(qNew);
    double sAnt  = evPtr->getm2Abs(iWinner);

    //$ Perform consistency check
    if ( sAnt < TINY || Q2new < TINY) {
      if (verbose >= 3) {
        printOut("VinciaShower::branch",
                 "Warning: s ="+num2str(sAnt)+"  Q2 = "+num2str(Q2new));
      }
      evPtr->increaseNFailed(iWinner);
      if (evPtr->getNFailed(iWinner) >= 50) {
        // TODO: find out whether this is what is intended
        evPtr->setReadyForHad(iWinner, true);
      }
      // Classifying as "Out" is not really correct, but this should be rare
      if (verbose >= 2) {
        updateTrialCount(trialStat["Out"]);
      }
      return false;
    }
  
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::branch","* Processing Branching at scale Q = " 
               +num2str(qNew)+"   isInterleaved = "+bool2str(isInterleaved));
    }

    //------------------------------------------------------------------------
    //$ GENERATE FULL TRIAL KINEMATICS (AND REJECT IF OUTSIDE PHASE SPACE)
    //$ (includes hadronization vetos which only need the daughter kinematics)
    if (not generateKinematics()) {//$
      evPtr->vetoProtoState();
      return false;
    }

    //------------------------------------------------------------------------
    //$ VETO STEP: DECIDE WETHER TO ACCEPT OR REJECT BRANCHING
    if (not acceptTrial(event)) {//$
      evPtr->vetoProtoState();
      return false;
    }
    
    //------------------------------------------------------------------------
    //$ ASSIGN COLOUR FLOW
    // (more economical in use of colour tags to wait until we are sure
    // the branching is accepted)
    assignColourFlow();
    
    //------------------------------------------------------------------------
    
    // If diagnostic histograms are on, write in the branching scale
    if (verbose >= 2) {
      updateTrialCount(trialStat["Bra"]);
      if (!firstQBranchBinned) {
        vinciaHistos["1stBranchingQE/eCM"].fill( qNew/evPtr->eCMHad() );
        firstQBranchBinned = true;
      }
    }
    
    evPtr->branch(event); //$
    
    // Since there was an emission, qRestart does not restrict anything
    qRestart         = numeric_limits<double>::max();
    
    if (verbose >= 6 || printAfterBranching) {
      event.list();
      list();
    }
    
    // update the protected data member iSysSel in SpaceShowerInterface
    // and TimeShowerInterface
    iSysSel(evPtr->getiSys(iWinner));
    // Done.      
    return true;
    
  }

  //*********
  
  // Update list of branchelementals after ISR radiation
  
  void VinciaShower::update( int iSys, Event& event, bool hasWeakRad ) {
    if (verbose >= 1)
      printErr("VS::update","system "+num2str(iSys) + ", hasWeakRad = "
               + bool2str(hasWeakRad) +" (function not implemented)");
    if (verbose >= 5) event.list();
  }
  
  //*********
  
  // Update list of branchelementals after MPI rescattering 
  
  void VinciaShower::rescatterUpdate( int iSys, Event& event) {
    if (verbose >= 1)
      printErr("VS::rescatterUpdate","system "+num2str(iSys)
               +" (function not implemented)");
    if (verbose >= 5) event.list();
  }
  
  
  // Update list of branchelementals after MPI rescattering 
  
  bool VinciaShower::rescatterPropagateRecoil( Event& event, Vec4& pNew) {
    if (verbose >= 1)
      printErr("VinciaShower::rescatterPropagateRecoil",
               "function not implemented");
    if (verbose >= 5) event.list();
    if (verbose >= 9) printOut("VinciaShower::rescatterPropagateRecoil",
                               "pNew = "+num2str(pNew.px())+","
                               +num2str(pNew.py())+","+num2str(pNew.pz())
                               +","+num2str(pNew.e()));
    return false;
  }  
  
//**********************************************************************
  
// END OF INHERITED FUNCTIONS FOR INTERFACE TO PYTHIA8
// BEGIN VINCIA INTERNAL FUNCTIONS
  
//**********************************************************************

// VinciaShower class member functions

//*********

// Initialize internal and diagnostics histograms

  void VinciaShower::initVinciaHistos() {

  // Verbose output
  if (verbose >= 7) {
    printOut("VinciaShower::initVinciaHistos","begin --------------");
  }

  vinciaHistos.clear();

  // Don't book diagnostics histograms if verbose output not switched on.
  if (verbose <= 1) return;
    
    // ??? If matching is turned on again, have to check whether the right
    // histograms are initialised
    /*
  // Global
  int nBin;
  double binWid, wid, c, xMin, xMax;

  // MATCHING WEIGHTS
  nBin   = 61;
  binWid = 0.1;
  wid    = nBin * binWid;
  c      = 0.0;
  xMin   = c - wid/2.;
  xMax   = c + wid/2.;
  Hist HPME2LogUSR("Matching weight (USR)",nBin,xMin,xMax);

  // Histos for Paccept : gluon emission
  vinciaHistos["1stOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["2ndOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["3rdOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["4thOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["5thOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["6thOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["7thOrderMatchingLog10P(ME/PhysAnt):Emit"]=HPME2LogUSR;

  // Histos for Paccept : gluon splitting
  vinciaHistos["1stOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["2ndOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["3rdOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["4thOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["5thOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["6thOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;
  vinciaHistos["7thOrderMatchingLog10P(ME/PhysAnt):Split"]=HPME2LogUSR;

  // Histos for PME : gluon emission
  vinciaHistos["1stOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["2ndOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["3rdOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["4thOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["5thOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["6thOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;
  vinciaHistos["7thOrderMatchingLog10P(ME/TrialAnt):Emit"]=HPME2LogUSR;

  // Histos for PME : gluon splitting
  vinciaHistos["1stOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["2ndOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["3rdOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["4thOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["5thOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["6thOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  vinciaHistos["7thOrderMatchingLog10P(ME/TrialAnt):Split"]=HPME2LogUSR;
  */
    // Global
    int nBin;
    double binWid, wid, c, xMin, xMax;
    
    // MATCHING WEIGHTS
    nBin   = 61;
    binWid = 0.1;
    wid    = nBin * binWid;
    c      = 0.0;
    xMin   = c - wid/2.;
    xMax   = c + wid/2.;
    Hist HPME2LogUSR("Matching weight (USR)",nBin,xMin,xMax);
    
    // Histos for Paccept : gluon emission
    for (int nGtmp = 0; nGtmp <= 9; ++nGtmp) {
      for (int nQQtmp = 0; nQQtmp <= 5; ++nQQtmp) {
        string hisTitle="Log10(ME/AntPhys):";
        hisTitle += num2str(2*nQQtmp,1)+"q"+num2str(nGtmp,1)+"g";
        vinciaHistos[hisTitle]=HPME2LogUSR;
        hisTitle="Log10(ME/AntTrial):";
        hisTitle += num2str(2*nQQtmp,1)+"q"+num2str(nGtmp,1)+"g";
        vinciaHistos[hisTitle]=HPME2LogUSR;
      }
    }
    
    // Histos for pTevol scales and alphaS
    nBin   = 100;
    binWid = 0.1;
    wid    = nBin * binWid;
    xMax   = 0.;
    xMin   = xMax - wid;
    Hist HqLn("Ln(q2/sSystem) distribution (USR)",nBin,xMin,xMax);
    Hist HalphaS("alphaS distribution (USR)", nBin, 0, 1);
    Hist Hari("pAri distribution", 10, 0.0, 2.0);
    for (int nGtmp = 0; nGtmp < 100; ++nGtmp) {
      for (int nQQtmp = 0; 2*nQQtmp + nGtmp < 100; ++nQQtmp) {
        string state = num2str(2*nQQtmp,1)+"q"+num2str(nGtmp,1)+"g";
        string hisTitle="Ln(q2trial/sSystem):" + state;
        vinciaHistos[hisTitle + "emit"]=HqLn;
        vinciaHistos[hisTitle + "split"]=HqLn;
        hisTitle="Ln(q2/sSystem):" + state;
        vinciaHistos[hisTitle + "emit"]=HqLn;
        vinciaHistos[hisTitle + "split"]=HqLn;
        string hisTitleAlphaS = "alphaS:" + state;
        vinciaHistos[hisTitleAlphaS + "emit"] = HalphaS;
        vinciaHistos[hisTitleAlphaS + "split"] = HalphaS;
        string hisTitleAri = "pAri:" + state;
        vinciaHistos[hisTitleAri] = Hari;
      }
    }
    
    // Histogram of QE/eCMHad for the first branching
    Hist hQ2E("QE/eCM",1000,0.0,1.0);
    vinciaHistos["1stBranchingQE/eCM"] = hQ2E;
    
    // Histogram of QE/eCMHad for the first trial
    vinciaHistos["1stTrialQE/eCM"] = hQ2E;
  }
  
  const Hist& VinciaShower::getDiagnosticHistogram( string name ) {
    return vinciaHistos[name];
  }
  
  double VinciaShower::pAriHeadroomFactor(int iAnt, int iTrial) const {
    const TrialGenerator* tgPtr = evPtr->getTrialGenerator(iAnt, iTrial);
    bool splitI = tgPtr->isSplitConvertI() && evPtr->isOutgoingMot1(iAnt);
    bool splitK = tgPtr->isSplitConvertK() && evPtr->isOutgoingMot2(iAnt);
    bool usePari = gluonSplittingCorrection && (splitI || splitK);
    return usePari ? 2.0 : 1.0;
  }
  
  //*********
  
  // Phase space regions
  
  double VinciaShower::getQmin(double q) const {
    return getQmin(q, qMinVecSav);
  }
  
  // QMinVecNow needs to be sorted in descending order
  double VinciaShower::getQmin(double Q, const vector<double>& QMinVecNow)
    const {
    for (vector<double>::const_iterator itQ = QMinVecNow.begin(); 
         itQ != QMinVecNow.end(); ++itQ) {
      if (Q > *itQ) {
        return *itQ;
      }
    }
    return 0.0;
  }
  
  int VinciaShower::getNfPDF(double Q) const {
    if      (Q > pdfThresholdB) return 5;
    else if (Q > pdfThresholdC) return 4;
    else return 3;
  }
  
  bool VinciaShower::acceptTrial(const Event& event) {
    
    // Verbose output
    if (verbose >= 7) {
      printOut("VS::acceptTrial","begin --------------");
    }
    
    evPtr->checkRecPtr(event);
    
    int iProto = evPtr->getProtoStateIndex();
    double qNew = evPtr->getTrialScale(iProto);
    
    //$ Can we veto the trial without calculating an accept probability?
    
    //***********************************************************************
    //$1 Optional: start by checking sector veto (if doing sector showers)
    //***********************************************************************
    if (sectorShower) {
      if (!evPtr->isProtoStateInSector()) {
        if (verbose >= 5) {
          printOut("VS::acceptTrial",
                   "Branching Vetoed. Failed sector veto.");
        }
        if (verbose >= 2) { updateTrialCount(trialStat["Out"]); }
        return false;
      } else {
        if (verbose >= 7) {
          printOut("VS::acceptTrial",
                   "Trial passed sector veto. Continuing...");
        }
      }
    }
    
    //************************************************************************
    //$1 Check whether incoming heavy flavours can still convert
    //************************************************************************
    if (!conversionPossibleAfterBranching()) {
      printOut(5, "VinciaShower::acceptTrial",
               "Branching Vetoed. Not enough phase space for incoming "
               "heavy flavours to convert after branching.");
      if (verbose >= 2) { updateTrialCount(trialStat["HFl"]); }
      return false;
    }
    
    //************************************************************************
    //$1 Check global hadronization veto(s) (requiring full event)
    // Note: here only check neighbor invariants. Branching invariants
    // were already checked above.
    // Note 2: for neighbors, we are only checking
    // nvariant mass > pion mass. Could in principle be made more elaborate
    // by checking cutoffScale as well, given sufficient reason to do so.
    //************************************************************************
    
    // Require all neighbor dot products at least above pion mass
    // (consistent with length > thickness assumption of string model)
    if (secondaryHadronizationVetos) {
      double m2pi  = pow2(particleDataPtr->m0(111));
      double m2mP  = evPtr->getm2MinPostBranching();
      if (m2mP < 1.05*m2pi) {
        evPtr->increaseNFailed(iProto);
        if (evPtr->getNFailed(iProto) > 50 && evolutionType == cutoffType) {
          // Switch off this branchelemental.
          evPtr->setReadyForHad(iProto, true);
        }
        if (verbose >= 5) {
          printOut("VS::acceptTrial","=== Branching Vetoed."
                   " mNeighbor = " + num2str(sqrt(m2mP), 0) +
                   " < " + num2str(sqrt(1.05*m2pi), 0) + ".");
        }
        if (verbose >= 2) { updateTrialCount(trialStat["SHa"]); }
        return false;
      }
    }
    
    //************************************************************************
    //$ Check if the pre- and post-branching configurations can be treated
    //$ as being relativistic (massless), if yes create massless configuration
    //************************************************************************
    
    int iSys = evPtr->getiSys(iProto);
    vector<const Particle*> partonsOrgMassive = evPtr->getPartPtrVec(iSys);
    pair< vector<const Particle*>, std::tr1::array<int, 3> >
      partonsMassiveInsPositions = evPtr->getPartPtrVecPostBranching();
    vector<const Particle*> partonsMassive = partonsMassiveInsPositions.first;
    vector<Particle> partonsMassless;
    
    std::tr1::array<int, 3> insPositions = partonsMassiveInsPositions.second;
    int posi = insPositions[0];
    int posj = insPositions[1];
    int posk = insPositions[2];
    
    // If we have been treating the system as massless so far, check if
    // we should continue doing so
    bool treatAsMasslessNow = treatAsMassless &&
      vinCluPtr->isRelativistic(partonsMassive, relativisticSpeed);
    if (treatAsMasslessNow) {
      for (vector<const Particle*>::size_type i = 0;
           i < partonsMassive.size(); ++i) {
        partonsMassless.push_back(*partonsMassive[i]);
      }
      treatAsMasslessNow = vinCluPtr->mapToMassless(partonsMassless);
    }
    
    vector<const Particle*> partonsOrg;
    vector<const Particle*> partons;
    if (treatAsMasslessNow) {
      for (vector<Particle>::const_iterator itP = partonsMassless.begin();
           itP != partonsMassless.end(); ++itP) {
        partons.push_back(&*itP);
      }
      partonsOrg = evPtr->getPartPtrVecMassless(iSys);
    }
    else {
      partonsOrg = partonsOrgMassive;
      partons = partonsMassive;
    }
    
    // Verbose output
    if (verbose >= 6) {
      if (treatAsMasslessNow)
        printOut("VS::acceptTrial","Will treat particles as relativistic");
      else
        printOut("VS::acceptTrial","Will treat particles as massive");
    }
    
    // Dot products and masses (partons have already been made massless if
    // the system is treated relativistically)
    double m2ant = evPtr->getm2Abs(iProto);
    double yij = 2.0*dot4(*partons[posi], *partons[posj])/m2ant;
    double yjk = 2.0*dot4(*partons[posj], *partons[posk])/m2ant;
    double yijmassive = yij;
    double yjkmassive = yjk;
    double mui, muj, muk;
    double mAnt = sqrt(evPtr->getm2(iProto));
    if (!treatAsMasslessNow) {
      mui = partons[posi]->m() / mAnt;
      muj = partons[posj]->m() / mAnt;
      muk = partons[posk]->m() / mAnt;
    }
    else {
      mui = 0.0;
      muj = 0.0;
      muk = 0.0;
      // Check old vs new invariants
      yijmassive = evPtr->getProtoStatey12();
      yjkmassive = evPtr->getProtoStatey23();
      if (verbose >= 6) {
        cout << setprecision(3) << scientific;
        cout<<"      yij = "<<yij<<" (with masses = "<<yijmassive<<")\n";
        cout<<"      yjk = "<<yjk<<" (with masses = "<<yjkmassive<<")\n";
      }
    }
    
    //************************************************************************
    // wPhys will contain physical weights representing uncertainty variations
    // iVar = 0 : User settings
    //        1 : Vincia defaults
    //        2 : AlphaS-Hi    (with def ants)
    //        3 : AlphaS-Lo    (with def ants)
    //        4 : Ant-Hi       (with def alphaS)
    //        5 : Ant-Lo       (with def alphaS)
    //        6 : NLO-Hi       (with def ants & alphaS)
    //        7 : NLO-Lo       (with def ants & alphaS)
    //        8 : Ord:pT       (with def ants & alphaS, Pimp-PT)
    //        9 : Ord:mD       (with def ants & alphaS, Pimp-MD)
    //       10 : NLC-Hi       (with def ants & alphaS, CA for all emissions)
    //       11 : NLC-Lo       (with def ants & alphaS, 2CF for all emissions)
    //       12 : AlphaS-pdf-1 (with def ants, alphaS and pdf variation 1)
    //       13 : AlphaS-pdf-2 (def ants, alphaS and pdf variation 2)
    //       14 : AlphaS-pdf-3 (def ants, alphaS and pdf variation 3)
    // Global shower  :  Pimp * Pari * ant-global
    // Sector shower  :                ant-sector
    
    // Antennae in wPhys are spin-summed (spin selection is done below)
    //************************************************************************
    
    vector<double> wPhys(nVariations());
    
    //************************************************************************
    //$ Call antenna function (dimensionless)
    //************************************************************************
    
    // Only use polarized antennae for massless particles
    double hI = evPtr->getPolMot1(iProto);
    double hK = evPtr->getPolMot2(iProto);
    bool isPolarized =
      helicityShower && (hI != 9.0 && hK != 9.0) && treatAsMassless;
    double sAnt = evPtr->getm2Abs(iProto);
    
    // Define pointer to Global or Sector antennae, depending on shower setting
    const AntennaSet* antUsrPtr =
      sectorShower ? antennaSetsSct[0] : antennaSets[0];
    const AntennaSet* antDefPtr =
      sectorShower ? antennaSetsSct[1] : antennaSets[1];
    const AntennaSet* antMaxPtr =
      sectorShower ? antennaSetsSct[2] : antennaSets[2];
    const AntennaSet* antMinPtr =
      sectorShower ? antennaSetsSct[3] : antennaSets[3];
    
    // Compute spin-summed antenna function
    if (verbose >= 8) printOut("VS::acceptTrial",
                               "Evaluating antenna function, yij = " +
                               num2str(yij,0) + ", yjk = " + num2str(yjk, 0));
    
    // Unpolarized case: ignore parent helicities
    // Polarized case: use parent helicities
    double hIeval  = isPolarized ? hI : 9.0;
    double hKeval  = isPolarized ? hK : 9.0;
    
    vector<double> helSum(nVariations());
    
    helSum[0] =
      antUsrPtr->antennaFunctionProto(evPtr, yij,yjk,mui, muj,muk,
                                      hIeval,hKeval,9.0,9.0,9.0); //$
    
    if (uncertaintyBands) {
      helSum[1] = antDefPtr->antennaFunctionProto(
                                                  evPtr, yij,yjk,mui, muj,muk, hIeval,hKeval,9.0,9.0,9.0);
      // Use DEF antennae as basis for uncertainty variations
      for (int iVar=2; iVar<nVariations(); ++iVar)
        helSum[iVar] = helSum[1];
      helSum[4] = antMaxPtr->antennaFunctionProto(
                                                  evPtr, yij,yjk,mui, muj,muk, hIeval,hKeval,9.0,9.0,9.0);
      helSum[5] = antMinPtr->antennaFunctionProto(
                                                  evPtr, yij,yjk,mui, muj,muk, hIeval,hKeval,9.0,9.0,9.0);
    }
    
    // Verbose output
    if (verbose >= 8)
      printOut("VS::acceptTrial",
               "helSum[0] (dimensionless) = " +num2str(helSum[0]));
    
    //***********************************************************************
    //$ Calculate phase space volume factor due to parent masses
    //***********************************************************************
    
    double psVol = evPtr->getPSVolumeFactor(iProto);
    // Verbose output
    if (verbose >= 8) {
      printOut("VS::acceptTrial", "phase space volume factor = "
               +num2str(psVol));
    }
    
    //***********************************************************************
    //$ Apply colour factor
    //***********************************************************************
    
    for (int iVar=0; iVar<nVariations(); ++iVar) {
      double chargeFactor   = antDefPtr->getChargeFactorProto(evPtr);
      if (iVar == 0) chargeFactor = antUsrPtr->getChargeFactorProto(evPtr);
      else if (iVar == 4) chargeFactor = antMaxPtr->getChargeFactorProto(evPtr);
      else if (iVar == 5) chargeFactor = antMinPtr->getChargeFactorProto(evPtr);
      else if (iVar == 10 && evPtr->isProtoEmission()) chargeFactor = CA;
      else if (iVar == 11 && evPtr->isProtoEmission()) chargeFactor = CF;
      wPhys[iVar] = psVol * chargeFactor * helSum[iVar];
      
      // Verbose output
      if (iVar == 0 && verbose >= 8) {
        printOut("VS::acceptTrial", "chargeFactor[0]           = "
                 +num2str(chargeFactor));
      }
    }
    
    //***********************************************************************
    //$ Compute Pimp modification (only applied to global showers)
    //***********************************************************************
    
    //    Pimp (allows suppressed unordered branchings)
    double Pimp = 1.0;
    // Will only be initialized if this isn't the first shower branching
    double pT2UnOrdOld = 0.0;
    double mD2UnOrdOld = 0.0;
    if (evPtr->nPartShower(iSys) > 0) {
      // The resolution scale of the event, using pT for emission and splitting
      pT2UnOrdOld = evPtr->getSectorR(iSys, evolutionType, evolutionType);
      // The resolution scale of the event, using mD for emission, mqq for g->qq
      mD2UnOrdOld = evPtr->getSectorR(iSys, 2, 7);
      if (!sectorShower && orderingMode >= 2 && evPtr->isUnordered(iProto)) {
        double q2new  = pow2(qNew);
        double Q2UnOrdOld  = evPtr->getSectorR(iSys, evolutionType, 7);
        Pimp = 1.0/(1.0 + q2new/Q2UnOrdOld);
        for (int iVar = 0; iVar < nVariations(); ++iVar) {
          // pT-evolution (for g->qq splittings too)
          if (iVar == 8) {
            double pT2new;
            if (evPtr->isFF(iProto)) {
              pT2new = sAnt*yij*yjk;
            }
            else if (evPtr->isIF(iProto)) {
              if (evPtr->isIncomingMot1(iProto)) {
                pT2new = sAnt*(yij*yjk/(1.0 + 2*yjk));
              }
              else {
                pT2new = sAnt*(yij*yjk/(1.0 + 2*yij));
              }
            }
            else {
              pT2new = sAnt*(yij*yjk/(1.0 + 2*yij + 2*yjk));
            }
            double PimpPT = 1.0/(1.0 + pT2new/pT2UnOrdOld);
            wPhys[iVar] *= PimpPT;
          }
          // mD-evolution (with mD normalization and using mqq for g->qq)
          else if (iVar == 9) {
            int id2 = evPtr->getProtoStateIdDau2();
            double mD2new = (id2 == 21 || id2 == 22) ? sAnt*min(yij,yjk) : q2new;
            double PimpMD =  1.0/(1.0 + mD2new/mD2UnOrdOld);
            wPhys[iVar] *= PimpMD;
          }
          else {
            wPhys[iVar] *= Pimp;
          }
        }
      }
    }
    
    if (verbose >= 8)
      printOut("VS::acceptTrial","Pimp[0]                   = "+num2str(Pimp));
    
    //***********************************************************************
    //$ Compute pAri modification (only applied to global showers)
    //***********************************************************************
    
    bool usePari = !sectorShower && gluonSplittingCorrection;
    double pAri = usePari ? evPtr->getProtoStatePari() : 1.0;
    if (usePari) {
      for (int iVar = 0; iVar < nVariations(); ++iVar) {
        wPhys[iVar] *= pAri;
      }
    }
    if (verbose >= 8) {
      printOut("VS::acceptTrial","pAri                      = "+num2str(pAri));
    }
    
    //***********************************************************************
    //$ Determine alphaS
    //***********************************************************************
    // Impose default choice. Can differ slighly from trial even when running
    // inside trial integral, due to flavor thresholds. Here, alphaS(mu) is
    // returned directly, with the number of flavors active at mu
    
    // Default is Ariadne pT for gluon emission and mQQ for gluon splitting
    // In initial-final and initial-initial, use the generalization of pT
    // if it is the evolution variable
    double yMuDef     = yij * yjk;
    if (evolutionType == 20 && evPtr->isProtoEmission()) {
      yMuDef = pow2(pTevol(iProto))/sAnt;
    }
    else if (evPtr->isProtoSplittingMot1() || evPtr->isProtoConvertQMot1())
      yMuDef = evPtr->getProtoStatemSqua12()/sAnt;
    else if (evPtr->isProtoSplittingMot2() || evPtr->isProtoConvertQMot2())
      yMuDef = evPtr->getProtoStatemSqua23()/sAnt;
    else if (evPtr->isProtoConvertGMot1()) {
      yMuDef = evPtr->getProtoStates12()/sAnt;
    }
    else if (evPtr->isProtoConvertGMot2()) {
      yMuDef = evPtr->getProtoStates23()/sAnt;
    }
    // User choice:
    double yMuUsr = yMuDef;
    if (alphaSmode == 0) {
      double muRmax = 0.0;
      if (evPtr->isProtoEmission()) {
        muRmax = translateTopTevol(evPtr->qKineMaxEmit(iProto));
      } else if (evPtr->isProtoConvertMot1() || evPtr->isProtoSplittingMot1()) {
        muRmax = evPtr->qKineMaxSplitConvMot1(iProto);
      } else if (evPtr->isProtoConvertMot2() || evPtr->isProtoSplittingMot2()) {
        muRmax = evPtr->qKineMaxSplitConvMot2(iProto);
      }
      yMuUsr = pow2(muRmax)/sAnt;
    }
    yMuUsr *= pow2(alphaSkMu);
    yMuDef *= pow2(alphaSkMuDef);
    
    double mu2freeze  = pow2(alphaSmuFreeze);
    double mu2usr     = mu2freeze + yMuUsr * sAnt;
    
    double alphaSusr  = min(alphaSmax, alphaSptr->alphaS(mu2usr));
    
    if (verbose >= 8) {
      printOut("VS::acceptTrial",
               "alphaSusr                 = "+num2str(alphaSusr));
    }
    
    // alphaS factor
    wPhys[0] *= alphaSusr;
    // Do uncertainty variations
    if (uncertaintyBands) {
      double mu2def     = mu2freeze + yMuDef * sAnt;
      // Up/down variatons
      double kMuVar2   = pow2(uncertaintyBandsKmu);
      double mu2hi     = mu2freeze + yMuDef * sAnt * kMuVar2;
      double mu2lo     = mu2freeze + yMuDef * sAnt / kMuVar2;
      for (int iVar = 1; iVar < nVariations(); ++iVar) {
        int iAlphaS = getiAlphaS(iVar);
        double                mu2 = mu2def;
        if      (iVar == 2) { mu2 = mu2hi; }
        else if (iVar == 3) { mu2 = mu2lo; }
        wPhys[iVar] *= min(alphaSmax, alphaSptr->alphaSofQ2(mu2, iAlphaS));
      }
    }
    
    //***********************************************************************
    //$ Calculate pdf ratio
    //***********************************************************************
    
    double mu2F = 0.0;
    if (!evPtr->isFF(iProto)) {
      // Note: the factorization scale MUST NOT be translated to pTevol
      // normalization due to heavy flavour thresholds
      mu2F = pow2(qNew/pdfScaleFactor);
      evPtr->setProtoStatePDF(mu2F);
      for (int iVar = 0; iVar < nVariations(); ++iVar) {
        double rPDF = evPtr->getProtoStatePDFRatio(getiPDF(iVar));
        wPhys[iVar] *= rPDF;
      }
    }
    
    if (verbose >= 8) {
      printOut("VS::acceptTrial","pdfRatio[0]               = " +
               num2str(evPtr->getProtoStatePDFRatio(0)));
    }
    
    //***********************************************************************
    //$ Calculate trial weight
    //***********************************************************************
    
    double xRatioSqua = evPtr->getProtoStatexRatioSqua();
    double wTrial = evPtr->getProtoStateTrialWeightDimLess()*xRatioSqua;
    
    //***********************************************************************
    //$ Calculate accept probability pAccept (before matching)
    //***********************************************************************
    
    vector<double> Paccept(nVariations());
    for (vector<double>::size_type iVar = 0; iVar < wPhys.size(); ++iVar) {
      Paccept[iVar] = wPhys[iVar]/wTrial;
    }
    
    if (verbose >= 8) {
      printOut("VS::acceptTrial","wPhys[0] (before matching) = " +
               num2str(wPhys[0]));
    }
    
    if (verbose >= 8) {
      printOut("VS::acceptTrial","wTrial                     = "
               +num2str(wTrial));
    }
    if (verbose >= 9) {
      cout << scientific << setprecision(4);
      cout << "(xa*xb/xA/xB)^2 = " << xRatioSqua << endl;
      printTriGenWeights();
    }
    
    if (dumpTrialInfo) {
      trialInfoFile << scientific << setprecision(16);
      trialInfoFile << branchingStringLong()                  << ", "
                    << evPtr->getxA(iProto)                   << ", "
                    << evPtr->getIdA(iProto)                  << ", "
                    << evPtr->getxB(iProto)                   << ", "
                    << evPtr->getIdB(iProto)                  << ", "
                    << m2ant                                  << ", "
                    << evPtr->getProtoStatexNewA()            << ", "
                    << evPtr->getProtoStateIdNewA()           << ", "
                    << evPtr->getProtoStatexNewB()            << ", "
                    << evPtr->getProtoStateIdNewB()           << ", "
                    << yij                                    << ", "
                    << yjk                                    << ", "
                    << evPtr->getProtoStatey13()              << ", "
                    << helSum[0]                              << ", "
                    << antDefPtr->getChargeFactorProto(evPtr) << ", "
                    << Pimp                                   << ", "
                    << pAri                                   << ", "
                    << alphaSusr                              << ", "
                    << evPtr->getProtoStatePDFRatio()         << ", ";
      Trial       trial = evPtr->getProtoStateTrial();
      TriGenState tgs   = evPtr->getProtoStateTGS();
      vector<const TrialGenerator*> tgV = evPtr->getProtoStateTGForWeight();
      for (vector<const TrialGenerator*>::const_iterator itTG = tgV.begin();
           itTG != tgV.end(); ++itTG) {
        TrialWeight tw = (*itTG)->getWeight(tgs, trial);
        trialInfoFile << tw.aBar*m2ant     << ", "
                      << tw.alphaS         << ", "
                      << tw.headroomFactor << ", "
                      << tw.colFac         << ", "
                      << tw.rPDF           << ", ";
      }
      trialInfoFile << Paccept[0]                     << endl;
    }
    
    //***********************************************************************
    //$ Choose helicities for daughter particles
    //$ (so far only for massless polarized shower)
    //***********************************************************************
    
    // If we use polarizations, the mothers are polarized and everything is
    //   massless, choose the polarizations for the daughters
    if ( helicityShower && isPolarized && treatAsMasslessNow ) {
      AntennaSet* antSetPtr= sectorShower ? antennaSetsSct[0] : antennaSets[0];
      evPtr->chooseProtoStatePolarizations(antSetPtr, rndmPtr);
      
      // Uncertainty variations (would have picked these helicities with
      // a different probability if using different antennae).
      if (uncertaintyBands) {
        double PhelUsr = getProtoStateAntBarYPhys(0) / helSum[0];
        double PhelDef = getProtoStateAntBarYPhys(1) / helSum[1];
        for (int iVar = 1; iVar < nVariations(); ++iVar) {
          if (iVar == 4)
            Paccept[4] *= getProtoStateAntBarYPhys(2) / helSum[4] / PhelUsr;
          else if (iVar == 5)
            Paccept[5] *= getProtoStateAntBarYPhys(3) / helSum[5] / PhelUsr;
          else
            Paccept[iVar] *= PhelDef / PhelUsr;
        }
        
      }
    }
    // If not polarized
    else { evPtr->setProtoStatePolarizations(9, 9, 9); }
    
    //$ Check whether we match at this order
    
    //************************************************************************
    // Matching: Which order are we currently at?
    // Update and save color-ordered list of particles
    //************************************************************************
    
    int nGold = evPtr->nG(iSys);
    int nGnow;
    int nQQold = evPtr->nQQ(iSys);
    int nQQnow;
    
    if (evPtr->getProtoStateIdDau2() == 21){
      nGnow = nGold + 1;
      nQQnow = nQQold;
    }
    else {
      nGnow = nGold - 1;
      nQQnow = nQQold + 1;
    }
    int nQCDnow   = nGnow + 2*nQQnow -2;
    // Only match up to matchingLO and only above non-perturbative IR cutoff
    // and (for now) only resonance decays
    bool doMatch  = (nQCDnow <= matchingLO) && (qNew > matchingIRcutoff)
                    && evPtr->isProtoStateResonanceDecay();
    
    vector<const Particle*> pMatch;
    if (doMatch && evPtr->isProtoStateResonanceDecay()) {
      pMatch = evPtr->prependResonance(partons, iSys);
    }

    // Matching: check if ME exists for this state
    doMatch = doMatch && matchingPtr->isMatched(1, pMatch);
    
    //------------------------------------------------------------------------
    // Match to fixed-order QCD matrix elements (from MadGraph)
    
    // Default is not to impose any matching scale
    bool imposeMatchingScale = false;
    double mSystem = evPtr->getmOut(evPtr->getiSys(evPtr->getProtoStateIndex()));
    double q2match = 0.0;
    //$ Check whether to impose matching scale, if yes determine it
    if (doMatch && nQCDnow >= matchingRegOrder && matchingRegOrder != 0) {
      imposeMatchingScale = true;
      // If matchingRegType is relative, determine matching scale
      if (matchingScaleIsAbs) q2match = pow2(matchingScale);
      else q2match = pow2(matchingScaleRatio * mSystem);
    }
    
    // Compute matching scale dampening factor (if imposed)
    double DeltaDamp = 1.0;
    double q2damp    = pow2(mSystem);
    
    // Check matching scale
    if (imposeMatchingScale) {
      
      // Default is to interpret matching scale as Ariadne pT
      if (matchingRegType == 1) q2damp = yij * yjk * sAnt;
      else if (matchingRegType == 2) q2damp = min(yij,yjk) * sAnt;
      // Note: should probably check entire topology, so the scale is
      // uniquely defined (in a Markovian sense).
      
      // MatchingRegShape = 0: Step Function
      if (matchingRegShape == 0) {
        if (q2damp < q2match) {
          DeltaDamp = 0.0;
          doMatch = false;
        }
        else DeltaDamp = 1.0;
      }
      // MatchingRegShape = 1: linear in log(Q) between 1/2 QM2 < Q2 < 2 QM2
      else {
        double log2R = log(q2damp/q2match)/log(2.);
        if (log2R < -1) {
          DeltaDamp = 0.0;
          doMatch = false;
        } else if (log2R > 1.) {
          DeltaDamp = 1.0;
        } else {
          DeltaDamp = 0.5*log2R + 0.5 ;
        }
      }
    }
    
    string HPME = "none";
    bool didMatchNLO = false;
    //$ [do we match?]
    if (doMatch) {
      //$ Get matching coefficients
      //getPME computes the matching factor -> PME
      if (matchingPtr->getPME(1, pMatch, PME)) {
        
        if (verbose >= 5) {
          string massString = treatAsMasslessNow ? " (massless)" : " (massive)";
          printOut("VS::acceptTrial","LO ME correction = "+
                   string(num2str(1.0 + DeltaDamp*PME[0]-1.))+massString);
        }
        for (int iVar=0; iVar < nVariations(); ++iVar)
          Paccept[iVar] *= 1.0 + DeltaDamp * (PME[iVar] - 1.);
        
        // Do NLO matching
        // (only for massless states so far)
        if (matchingNLO >= nQCDnow + 1 && treatAsMasslessNow) {
          vector<double> VME;
          if ( matchingPtr->getVME(1,pMatch,VME) ) {
            if (verbose >= 5) {
              string massString = " (massless)";
              printOut("VS::acceptTrial","NLO ME correction = "+
                       string(num2str(1.0 + VME[0]))+massString);
            }
            // Apply NLO correction, for each variation separately
            for (int iVar = 0; iVar < nVariations(); ++iVar)
              Paccept[iVar] *= max(0., 1. + VME[iVar]);
            didMatchNLO = true;
          } else {
            if (verbose >= 5) printOut("VS::acceptTrial","NLO ME correction"
                                       " not available");
          }
        }
        
      }
      
      //$1 Check for accept probabilites above one for global showers
      if (!sectorShower) {
        if ((Paccept[0] > 1.0 + TINY) && verbose >= 3) {
          cout << "Paccept[0] = " << Paccept[0]
               <<" at order = "<<nQCDnow<<"  Bare PME = "<<PME[0]
               <<" at qNew = "<<qNew<<" mDamp = "<<sqrt(q2damp)
               <<" mDamp/mMatch = 1/"<<sqrt(q2match/q2damp)
               <<" Pimp = "<<Pimp << " Pari = "<<pAri
               <<" DeltaDamp = "<<DeltaDamp<<endl;
          cout <<"       ijk = "<< evPtr->getProtoStateIdDau1() <<" "
               << evPtr->getProtoStateIdDau2()<<" "
               << evPtr->getProtoStateIdDau3()
          <<"  mij = "<<evPtr->getProtoStatem12()
          <<" mjk = "<<evPtr->getProtoStatem23()
          <<" mijk = "<< mAnt;
          cout <<"      yij = "<< evPtr->getProtoStatey12()
          <<" yjk = "<<evPtr->getProtoStatey23()<<endl;
          cout << " Configuration for calculation of PME "
               << "(vectors listed (x, y, z, e, m)):" << endl;
          for (vector<const Particle*>::const_iterator itP = pMatch.begin();
                itP != pMatch.end(); ++itP) {
             cout << "id = " << setw(4) << (*itP)->id() << ", p = "
                  << vec4toStr((*itP)->p()) << endl;
          }
        }
      }
      
      // Fill diagnostics histos?
      if (verbose >= 2) {
        string state = num2str(2*nQQnow,1) + "q" + num2str(nGnow,1) + "g";
        string HPacc = "Log10(ME/AntTrial):" + state;
        vinciaHistos[HPacc].fill(log10(max(TINY,Paccept[0])));
        string HqTrial = "Ln(q2trial/sSystem):" + state;
        vinciaHistos[HqTrial].fill(log(max(TINY,pow2(qNew/mSystem))));
      }
    }
    
    //$ Determine NLO K-factor variation (if not doing explicit NLO matching)
    // (largely degenerate with scale variation at LO*LL, but could in
    // principle behave differently at (N)LO*(N)LL)
    if ( uncertaintyBands && !didMatchNLO ) {
      double mu2nlo    = max(mu2freeze, sAnt);
      double alphaSnlo = min(alphaSmax, alphaSptr->alphaS(mu2nlo));
      Paccept[6]  *= (1.+alphaSnlo);
      Paccept[7]  /= (1.+alphaSnlo);
      
      // Also apply Sudakov reweightings for mD vs pT ordering
      // of gluon emissions (if strongly ordered)
      if (nGnow > nGold && evPtr->nPartShower(iSys) > 0) {
        // Difference for an emission: IqqEmit(type1) - IqqEmit(type2)
        // Note: we use qq emission as a stand-in for any type of emission
        // since differences are quite subleading and this is only used
        // for an uncertainty estimate.
        // We use the strong-ordering integrals, setting the Sudakov factor
        // to unity for un-ordered phase-space points.
        // We use the point with equal invariants as a stand-in to compute
        // the Sudakov integrals.
        
        // First get the Sudakov integrals down to the current ordering scale
        double IqqEmitPTord, IqqEmitMDord;
        double yijordPT = min(0.5,sqrt(pT2UnOrdOld/sAnt/pTnormalization));
        double yjkordPT = yijordPT;
        double yijordMD = min(0.5,mD2UnOrdOld/sAnt/mDnormalization);
        double yjkordMD = yijordMD;
        IqqEmitPTord = subtractionTermsPtr->IqqEmit(yijordPT,yjkordPT,1);
        IqqEmitMDord = subtractionTermsPtr->IqqEmit(yijordMD,yjkordMD,2);
        
        // Next get the Sudakov integrals down to the previous trial scale
        // (representing the scale down to which we have already taken the
        // difference between mD and pT into account?)
        // (set at least equal to the ordered one, to enforce Sudakov = 1
        // in unordered region)
        double IqqEmitPTold, IqqEmitMDold;
        double yijoldPT = min(0.5,sqrt(pT2trialSav/sAnt/pTnormalization));
        double yjkoldPT = yijoldPT;
        double yijoldMD = min(0.5,mD2trialSav/sAnt/mDnormalization);
        double yjkoldMD = yijoldMD;
        IqqEmitPTold = max(IqqEmitPTord,
                           subtractionTermsPtr->IqqEmit(yijoldPT,yjkoldPT,1));
        IqqEmitMDold = max(IqqEmitMDord,
                           subtractionTermsPtr->IqqEmit(yijoldMD,yjkoldMD,2));
        
        // Finally, get the Sudakov integrals down to the current trial scale
        // (set at least equal to the ordered one, to enforce Sudakov = 1
        //  in unordered region)
        double IqqEmitPT, IqqEmitMD;
        IqqEmitPT = max(IqqEmitPTold,subtractionTermsPtr->IqqEmit(yij,yjk,1));
        IqqEmitMD = max(IqqEmitMDold,subtractionTermsPtr->IqqEmit(yij,yjk,2));
        
        // Calculate difference and apply as NLO-style correction
        if (verbose >= 9) {
          cout<<"IqqEmitPT(ord,old,new) = "<<IqqEmitPTord<<" "<<IqqEmitPTold
          <<" "<<IqqEmitPT<<endl;
          cout<<"IqqEmitMD(ord,old,new) = "<<IqqEmitMDord<<" "<<IqqEmitMDold
          <<" "<<IqqEmitMD<<endl;
        }
        // AlphaS factor: use the same as for NLO corrections
        double fac     = alphaSnlo/2./M_PI * CA;
        double diffSud = fac * (IqqEmitMD-IqqEmitPT) ;
        double Rsud = max(0., 1. - diffSud);
        if (evolutionType == 1) Paccept[9] *= Rsud;
        else if (evolutionType == 2) Paccept[8] /= Rsud;
      }
    }
    
    //$ Limit uncertainty to factor 2.0 for first branching, to
    //$ capture possibly large finite term differences, then level off at
    //$ 20% asymptotically, to capture multiple-emission uncertainties.
    if (uncertaintyBands) {
      double limit = 2.0 - min(0.8,0.05*(nQCDnow-1));
      for  (int iVar=1; iVar<nVariations(); iVar++) {
        if (Paccept[iVar]>limit*Paccept[0]) Paccept[iVar] = limit * Paccept[0];
        if (Paccept[iVar]<Paccept[0]/limit) Paccept[iVar] = Paccept[0] / limit;
      }
    }
    
    //------------------------------------------------------------------------
    // Save scale of last trial (now = this one)
    
    Direction di = evPtr->getProtoStateDirDau1();
    Direction dj = evPtr->getProtoStateDirDau2();
    Direction dk = evPtr->getProtoStateDirDau3();
    q2trialSav  = resolutionPtr->yE(yij,yjk, di, dj, dk, evolutionType)*sAnt;
    pT2trialSav = resolutionPtr->yE(yij,yjk, di, dj, dk, 1)*sAnt;
    mD2trialSav = resolutionPtr->yE(yij,yjk, di, dj, dk, 2)*sAnt;
    
    //------------------------------------------------------------------------
    //$ Accept heavy quark conversion at threshold by fiat
    
    if (evPtr->getNFailed(iProto) >= 100 && evPtr->isProtoConvertQ()) {
      int idQ = evPtr->isProtoConvertQMot1() ? evPtr->getIdMot1(iProto)
                                          : evPtr->getIdMot2(iProto);
      double m2th = pow2(mThresholdPDF(idQ));
      if (eqTol(mu2F, m2th)) {
        if (verbose >= 3) {
          printOut("VinciaShower::acceptTrial",
                   "Forcing acceptance of heavy quark conversion");
        }
        for (vector<double>::iterator itP = Paccept.begin();
             itP != Paccept.end(); ++itP) {
          (*itP) = 1.0;
        }
      }
    }
    
    //------------------------------------------------------------------------
    //$ Accept or Reject Trial Emission
    
    // Verbose output
    if (verbose >= 8) {
      cout<<" Paccept[0]  = "<<setprecision(6)<<Paccept[0]<<endl;
    }
    
    // Accept/Reject Trial
    double ran = rndmPtr->flat();
    if (!(ran < Paccept[0])) {
      if (verbose >= 2) { updateTrialCount(trialStat["Vet"]); }
      if (verbose >= 5) {
        printOut("VS::branch","=== Branching Vetoed. wPhys/wTrial = "
                 +num2str(Paccept[0], 0));
      }
      //$1 If we do uncertainty bands, check if branching would have happened
      //$1 if we had been using another radiation function
      if (uncertaintyBands) {
        for  (int iVar=1; iVar<nVariations(); iVar++) {
          double Pacc = Paccept[0];
          double Pvar = Paccept[iVar];
          double rw = (1. - Pvar)/(1. - Pacc);
          scaleWeight(rw,iVar);
        }
      }
      // For each failed trial, attempt to (slowly) optimize headroomfactors
      if (nQCDnow <= matchingLO && evPtr->getNFailed(iProto) >= 1) {
        if (nGnow > evPtr->nG(iSys)) {
          double headRoomFactor = getHeadRoomFactorEmit(evPtr->nG(iSys),
                                                        evPtr->nQQ(iSys));
          if (headRoomFactor > 0.5)
            setHeadRoomEmit(0.999*headRoomFactor, evPtr->nG(iSys),
                            evPtr->nQQ(iSys));
        } else {
          double headRoomFactor = getHeadRoomFactorSplit(evPtr->nG(iSys),
                                                         evPtr->nQQ(iSys));
          if (headRoomFactor > 0.5)
            setHeadRoomSplit(0.999*headRoomFactor, evPtr->nG(iSys),
                             evPtr->nQQ(iSys));
        }
      }
      return false;
    } else if (Paccept[0] > 1.0+TINY) {
      if ( (verbose >= 2 && Paccept[0] > 1.1)
          || (verbose >= 1 && Paccept[0] > 1.5) ) {
        printOut("VS::branch",
                 "===  wPhys/wTrial = "+num2str(Paccept[0],6)
                 +" at QCD order "+num2str(nQCDnow,1)
                 +", "+ evPtr->getNameMot1(iProto)
                 + evPtr->getNameMot2(iProto)
                 +"->"+ evPtr->getProtoStateNameDau1()
                 + evPtr->getProtoStateNameDau2() + evPtr->getProtoStateNameDau3()
                 +", QE = "+num2str(qNew)
                 +", mIK = "+num2str(evPtr->getmAbs(iProto),6));
        if (verbose >= 3) {
          evPtr->listProtoState();
          cout << scientific;
          cout << "wTrial = " << num2str(wTrial) << endl;
          cout << "(xa*xb/xA/xB)^2 = " << xRatioSqua << endl;
          printTriGenWeights();
          cout << "wPhys  = " << num2str(wPhys[0])         << endl;
          if (!doMatch) {
            cout << "aBar (dimless) = " << helSum[0] << endl;
            cout << "alphaS         = " << alphaSusr << endl;
            cout << "colFac         = " << antUsrPtr->getChargeFactorProto(evPtr) << endl;
            cout << "rPDF           = " << evPtr->getProtoStatePDFRatio() << endl;
            cout << "pAri           = " << pAri << endl;
            cout << "pImp           = " << Pimp << endl;
          }
          if (evPtr->isIF(iProto)) {
            cout << "xParent   = " << num2str(evPtr->getxIF(iProto)) << endl;
            cout << "xDaughter = ";
            if (evPtr->getProtoStateMomDau1().pz() > 0) {
              cout << num2str(evPtr->getProtoStatexNewA()) << endl;
            }
            else {
              cout << num2str(evPtr->getProtoStatexNewB()) << endl;
            }
          }
          else if (evPtr->isII(iProto)) {
            cout << "xA = " << num2str(evPtr->getxA(iProto)) << endl;
            cout << "xa = " << num2str(evPtr->getProtoStatexNewA()) << endl;
            cout << "xB = " << num2str(evPtr->getxB(iProto)) << endl;
            cout << "xb = " << num2str(evPtr->getProtoStatexNewB()) << endl;
          }
        }
        
        if (doMatch && verbose >= 8) cout<<" PME = "<<PME[0]<<endl;
        if (verbose >= 5) evPtr->listProtoState();
        if ((Paccept[0] > 1.0 && verbose >= 5) ||
            (Paccept[0] > 2.0 && verbose >= 4) ||
            (Paccept[0] > 10.0 && verbose >= 3)) {
          cout << "wTrial = " << num2str(wTrial, 6)
               << ", wPhys = " << num2str(wPhys[0], 6) << endl;
          cout << " Post-branching particles: "<<endl;
          for (int ip=0; ip<int(partonsMassive.size()); ++ip) {
            cout << " id"<<ip<<" = "<<num2str(partonsMassive[ip]->id(),7)
                 << " p"<<" = " << partonsMassive[ip]->p();
          }
          cout << " Configuration used to calculate pAccept:" << endl;
          for (vector<const Particle*>::const_iterator itP = partons.begin();
               itP != partons.end(); ++itP) {
            cout << " id" << itP - partons.begin() << " = "
                 << num2str((*itP)->id(),7) <<" p"<<" = " << (*itP)->p();
          }
        }
      }
      
      // Max reweight by factor 10
      if (allowReweighting) scaleWeight(min(10.,Paccept[0]),0);
      
      // Automatically increase headroom factor when violations encountered,
      // up to a maximum of a factor 5.
      if (nQCDnow <= matchingLO) {
        if (nGnow > evPtr->nG(iSys)) {
          double headRoomFactor = getHeadRoomFactorEmit(evPtr->nG(iSys),
                                                        evPtr->nQQ(iSys));
          setHeadRoomEmit(min(1.2,Paccept[0])*headRoomFactor,
                          evPtr->nG(iSys), evPtr->nQQ(iSys));
        } else {
          double headRoomFactor = getHeadRoomFactorSplit(evPtr->nG(iSys),
                                                         evPtr->nQQ(iSys));
          setHeadRoomSplit(min(1.2,Paccept[0])*headRoomFactor,
                           evPtr->nG(iSys), evPtr->nQQ(iSys));
        }
      }
      
    }
    else if (verbose >= 5) {
      printOut("VinciaShower:acceptTrial","Branching accepted, wPhys/wTrial = "
               + num2str( Paccept[0], 0 ) + ".");
    }
    
    // For accepted branching, rescale uncertainty event weights
    if (uncertaintyBands) {
      for (int iVar=1; iVar<nVariations(); iVar++) {
        scaleWeight(Paccept[iVar]/Paccept[0],iVar);
      }
    }
    
    // Fill diagnostics histos?
    if (verbose >= 2) {
      string state = num2str(2*nQQnow,1) + "q" + num2str(nGnow,1) + "g";
      string desc = state + (nGnow == nGold + 1 ? "emit" : "split");
      
      string HqPhys = "Ln(q2/sSystem):" + desc;
      if (vinciaHistos.find(HqPhys) != vinciaHistos.end()) {
      
        vinciaHistos[HqPhys].fill(log(max(TINY,pow2(qNew/mSystem))));
        
      }
      string HalphaS = "alphaS:" + desc;
      if (vinciaHistos.find(HalphaS) != vinciaHistos.end()) {
        vinciaHistos[HalphaS].fill(alphaSusr);
      }
      if (pAri != 1.0) {
        string Hari = "pAri:" + state;
        if (vinciaHistos.find(Hari) != vinciaHistos.end()) {
          vinciaHistos[Hari].fill(pAri);
        }
      }
      if (doMatch) {
        string HPacc = "Log10(ME/AntPhys):" + desc;
        if (vinciaHistos.find(HPacc) != vinciaHistos.end()) {
          vinciaHistos[HPacc].fill(log10(max(TINY,PME[0])));
        }
      }
    }
    
    //------------------------------------------------------------------------
    
    // Update global saved quantities
    MESav           = MENew;
    treatAsMassless = treatAsMasslessNow;

    return true;
  }

  bool VinciaShower::generateKinematics() {
  
    // yij = 2*pi*pj/sAbs() or (pi + pj)^2/sAbs() (for gluon splitting)
    double yij = evPtr->getProtoStatey12();
    double yjk = evPtr->getProtoStatey23();
    
    //$ Return if the trial is outside the physical phase space
    if (!evPtr->invProtoAllowed()) {
      printOut(5, "VS::generateKinematics",
               "=== Branching Vetoed. Outside physical Phase Space.");
      if (verbose >= 2) { updateTrialCount(trialStat["Out"]); }
      return false;
    }
    
    //************************************************************************
    //$ Check hadronization veto(s) 1: hadronization cutoff & meson masses
    //$ Note: here only check branching invariants. Invariants with respect
    //$ to neighbors can optionally be checked after the branching kinematics
    //$ have been fully defined, below.
    //************************************************************************
    
    // Check hadronization cutoff: Ariadne pT or alternatives
    int iProto = evPtr->getProtoStateIndex();
    double sAnt = evPtr->getm2Abs(iProto);
    double q2forHad;
    if (evPtr->isProtoEmission() || cutoffTypeNonEmit == 2) {
      q2forHad = yij * yjk * sAnt;
      // Use pT generalisation for ISR
      if (evolutionType == 20 && cutoffType == 20) {
        if      (evPtr->isIF(iProto)) {
          if (evPtr->isIncomingMot1(iProto)) { q2forHad = yij*yjk/(1+2*yjk)*sAnt; }
          else                            { q2forHad = yij*yjk/(1+2*yij)*sAnt; }
        }
        else if (evPtr->isII(iProto)) {
          q2forHad = yij*yjk/(1+2*yij+2*yjk)*sAnt;
        }
      }
      // Alternative: Cutoff in dot product
      else if (cutoffType == 2) q2forHad = min(yij,yjk) * sAnt;
    }
    else if (cutoffTypeNonEmit == 3) {
      if (evPtr->isProtoSplittingMot1() || evPtr->isProtoConvertQMot1()) {
        q2forHad = evPtr->getProtoStatemSqua12();
      }
      else if (evPtr->isProtoSplittingMot2() || evPtr->isProtoConvertQMot2()) {
        q2forHad = evPtr->getProtoStatemSqua23();
      }
      else if (evPtr->isProtoConvertGMot1()) {
        q2forHad = evPtr->getProtoStates12();
      }
      else if (evPtr->isProtoConvertGMot2()) {
        q2forHad = evPtr->getProtoStates23();
      }
      else {
        throw ExGeneral("VS::generateKinematics: branching not recognized");
      }
    }
    else {
      throw ExGeneral ("VS::generateKinematics: cutoffTypeNonEmit unknown");
    }
  
    double cut = evPtr->isProtoEmission() ? cutoffScale : cutoffScaleNonEmit;
    // Check cutoffScale
    if (q2forHad < pow2(cut)) {
      if (verbose >= 2) { updateTrialCount(trialStat["Cut"]); }
      if (verbose >= 8) {
        printOut("VS::acceptTrial",
                 "Hadronization scale measure " + num2str(sqrt(q2forHad), 0) +
                 " < cutoff scale " + num2str(cut, 0) + ", returning false.");
      }
      return false;
    }
    
    //$1 Require all color-connected invariants at least above lowest
    //$1 physical meson mass
    //$2 (consistent with length > thickness assumption of string model)
    
    if (secondaryHadronizationVetos) {
      // 12
      if ( evPtr->getProtoStateColDau1() == evPtr->getProtoStateAcolDau2() ) {
        int id1 = abs(evPtr->getProtoStateIdDau1());
        if (id1 == 21 || id1 <= 2) id1 = 1;
        int id2 = abs(evPtr->getProtoStateIdDau2());
        if (id2 == 21 || id1 <= 2) id2 = 1;
        int idMes = max(id1,id2)*100 + min(id1,id2)*10 + 1;
        // Special for ssbar, allow to form eta rather than eta'.
        if (idMes == 331) idMes = 221;
        double m2mes = pow2(particleDataPtr->m0(idMes));
        // Translate from dot product to invariant mass
        double m2    = evPtr->getProtoStatemSqua12();
        // Verbose test
        if (verbose >= 9) {
          cout << scientific << setprecision(3);
          cout<<" Had Test (12): m("<<evPtr->getProtoStateIdDau1()<<","
          <<evPtr->getProtoStateIdDau2()<<") = "<< sqrt(m2)
          <<" vs m("<<idMes<<") = " << sqrt(m2mes)  <<endl;
        }
        if (m2 < 1.01*m2mes) {
          if (verbose >= 5) printOut("VS::acceptTrial",
                                     "=== Branching Vetoed. m2 < 1.01*m2mes.");
          if (verbose >= 2) { updateTrialCount(trialStat["SHa"]); }
          return false;
        }
        double m2String = m2 - pow2(vinComPtr->getConstMass(id1))
                             - pow2(vinComPtr->getConstMass(id2));
        if (m2String < pow2(mStringMin)) {
          if (verbose >= 5) printOut("VS::acceptTrial", "=== Branching Vetoed."
                                     " mString = "
                                     + num2str(sqrt(m2String), 0)
                                     + " < mStringMin = "
                                     + num2str(mStringMin, 0) + ".");
          if (verbose >= 2) { updateTrialCount(trialStat["SHa"]); }
          return false;
        }
      }
      
      // 23
      if ( evPtr->getProtoStateColDau2() == evPtr->getProtoStateAcolDau3() ) {
        int id1 = abs(evPtr->getProtoStateIdDau2());
        if (id1 == 21) id1 = 1;
        int id2 = abs(evPtr->getProtoStateIdDau3());
        if (id2 == 21) id2 = 1;
        int idMes = max(id1,id2)*100 + min(id1,id2)*10 + 1;
        double m2mes = pow2(particleDataPtr->m0(idMes));
        // Translate from dot product to invariant mass
        double m2    = evPtr->getProtoStatemSqua23();
        // Verbose test
        if (verbose >= 9) {
          cout<<" Had Test (23): m("<<evPtr->getProtoStateIdDau2()<<","
          << evPtr->getProtoStateIdDau3()<<") = "<< sqrt(m2)
          <<" vs m("<<idMes<<") = "<< sqrt(m2mes) <<endl;
        }
        // Hadronization test
        if (m2 < 1.01*m2mes) {
          if (verbose >= 5) printOut("VS::acceptTrial",
                                     "=== Branching Vetoed. m2 < 1.01*m2mes.");
          if (verbose >= 2) { updateTrialCount(trialStat["SHa"]); }
          return false;
        }
        double m2String = m2 - pow2(vinComPtr->getConstMass(id1))
        - pow2(vinComPtr->getConstMass(id2));
        if (m2String < pow2(mStringMin)) {
          if (verbose >= 5) printOut("VS::acceptTrial", "=== Branching Vetoed."
                                     " mString = "
                                     + num2str(sqrt(m2String), 0)
                                     + " < mStringMin = "
                                     + num2str(mStringMin, 0) + ".");
          if (verbose >= 2) { updateTrialCount(trialStat["SHa"]); }
          return false;
        }
      }
    }
    
    //***********************************************************************
    //$ Generate full kinematics for this trial branching
    //***********************************************************************
    
    if (!generateFourMomenta()) {//$
      printOut(5, "VS::acceptTrial", "generateFourMomenta returned false");
      if (verbose >= 2) {
        updateTrialCount(trialStat["Out"]);
      }
      return false;
    }
    
    // check that post-branching momentum fractions are allowed
    double xa = evPtr->getProtoStatexNewA();
    double xb = evPtr->getProtoStatexNewB();
    double xaMax = evPtr->getxAmax(iProto);
    double xbMax = evPtr->getxBmax(iProto);
    if (!(xa <= xaMax && xb <= xbMax)) {
      if (verbose >= 2) {
        updateTrialCount(trialStat["Out"]);
      }
      return false;
    }
    
    return true;
  }

  // ******
  //
  // assignColourFlow():
  //
  // Paint a trialPtr with colour-flow information,
  // according to the type of branching.
  // Nearest-neighbours are never assigned the same "index" (for CR).
  
  void VinciaShower::assignColourFlow() {
    // If no new colour line has been created, the colours are already set
    if (evPtr->isProtoSplitting() || evPtr->isProtoConvertG()) {
      return;
    }
    else {
    
      int iProto = evPtr->getProtoStateIndex();
      int colAnt = evPtr->getCol(iProto);
      int colNeighbourI = evPtr->getColTagNeighborMot1(iProto);
      int colNeighbourK = evPtr->getColTagNeighborMot2(iProto);
      
      double yA, yB;
      if (evPtr->isProtoEmission()) {
        yA = evPtr->getProtoStatey12();
        yB = evPtr->getProtoStatey23();
      }
      else if (evPtr->isProtoConvertQMot1()) {
        // Colour connection 2-1-3 (2 outgoing quark, 1 incoming gluon)
        yA = evPtr->getProtoStatey12();
        yB = evPtr->getProtoStatey13();
      }
      else {
        // Colour connection 1-3-2 (2 outgoing antiquark, 1 incoming gluon)
        yA = evPtr->getProtoStatey13();
        yB = evPtr->getProtoStatey23();
      }
      
      // collect the existing colour tags
      vector<int> colTags;
      for (int iAnt = 0; iAnt < evPtr->nAnt(); ++iAnt) {
        if (iAnt != iProto) {
          colTags.push_back(evPtr->getCol(iAnt));
        }
      }
      bool newTagFound = false;
      int nTryColTag = 0;
      int col1 = 0;
      int col2 = 0;
      while (!newTagFound) {
        // Gluon emission: (convention) largest invariant inherits colour
        // New colour selected in 1...9 non-identical to adjacent ones
        int nextTag = 10*int(evPtr->nextColTag()/10) + 10;
        int colNew = nextTag + int(colAnt%10 + rndmPtr->flat()*8)%9 + 1;
        if (yA > yB) {
          while (colNew%10 == colNeighbourK%10)
            colNew = nextTag + int(colAnt%10 + rndmPtr->flat()*8)%9 + 1;
          col1 = colAnt;
          col2 = colNew;
        } else {
          while (colNew%10 == colNeighbourI%10)
            colNew = nextTag + int(colAnt%10 + rndmPtr->flat()*8)%9 + 1;
          col1 = colNew;
          col2 = colAnt;
        }
        newTagFound =
          find(colTags.begin(), colTags.end(), colNew) == colTags.end();
        ++nTryColTag;
        if (nTryColTag > 100) {
          throw ExGeneral("VinciaShower::assignColourFlow: Too many tries.");
        }
      }
      evPtr->setProtoStateColors(col1, col2);
    }
  }

  // Print details of weights of trial generators, for debugging
  void VinciaShower::printTriGenWeights() const {
    int iWinner = evPtr->getProtoStateIndex();
    double sAnt = evPtr->getm2Abs(iWinner);
    Trial       trial = evPtr->getProtoStateTrial();
    TriGenState tgs   = evPtr->getProtoStateTGS();
    
    vector<const TrialGenerator*> tgVec = evPtr->getProtoStateTGForWeight();
    
    for (vector<const TrialGenerator*>::const_iterator itTG = tgVec.begin();
         itTG != tgVec.end(); ++itTG) {
      
      TrialWeight tw = (*itTG)->getWeight(tgs, trial);
      cout << "trial generator " + (*itTG)->name() << endl;
      cout << scientific << setprecision(4);
      cout << setw(21) << " " << "aBar (dimless) = " << tw.aBar*sAnt
      << endl;
      cout << setw(21) << " " << "alphaS         = " << tw.alphaS
      << endl;
      cout << setw(21) << " " << "headroomFactor = " << tw.headroomFactor
      << endl;
      cout << setw(21) << " " << "colFac         = " << tw.colFac
      << endl;
      cout << setw(21) << " " << "rPDF           = " << tw.rPDF
      << endl;
    }
  }

  bool VinciaShower::conversionPossibleAfterBranching() const {
    int iProto = evPtr->getProtoStateIndex();
    for (int d = -1; d <= 1; d += 2) {
      if (evPtr->hasNeighbor(iProto, d) &&
          !evPtr->isOutgoingNeighbor(iProto, d) &&
          4 <= abs(evPtr->getIdNeighbor(iProto, d)) &&
               abs(evPtr->getIdNeighbor(iProto, d)) <= 5) {
        double m2Post = evPtr->getm2NeighborPostBranching(d);
        double xNow = evPtr->getxNeighbor(iProto, d);
        double xMax = evPtr->getxMaxNeighbor(iProto, d);
        double m2Conv = pow2(mThresholdPDF(evPtr->getIdNeighbor(iProto, d)));
        double m2Max  = m2Post*xMax/xNow;
        return m2Conv < m2Max;
      }
    }
    return true;
  }

  //*********

  double VinciaShower::getProtoStateAntBarYPhys(int iAntSet) const {
    double yij = evPtr->getProtoStatey12();
    double yjk = evPtr->getProtoStatey23();
    // Get masses of the particles m0 / mAnt (0 if isMassive() false)
    double mui = evPtr->getProtoStateVCmuDau1();
    double muj = evPtr->getProtoStateVCmuDau2();
    double muk = evPtr->getProtoStateVCmuDau3();
    
    // Find out whether we should use the sector antennae or not
    AntennaSet* antSetPtr = sectorShower ? antennaSetsSct[iAntSet] 
                                         : antennaSets[iAntSet];
    
    return antSetPtr->antennaFunctionProto(evPtr, yij, yjk, mui, muj, muk);
  }
  
  // Generate the four-vectors for a trial branching
  bool VinciaShower::generateFourMomenta() const {
    int iProto = evPtr->getProtoStateIndex();
    if      ( evPtr->isFF(iProto) ) { return generateFourMomentaFF();      }
    else if ( evPtr->isIF(iProto) ) { return generateFourMomentaIF();      }
    else if ( evPtr->isII(iProto) ) { return generateFourMomentaII();      }
    else                            { throw ExGeneral("not FF, IF or II"); }
  }
  
  bool VinciaShower::generateFourMomentaFF() const {
    int iProto = evPtr->getProtoStateIndex();
    double sij = evPtr->getProtoStates12();
    double sjk = evPtr->getProtoStates23();
    double sik = evPtr->getProtoStates13();
    double massi = evPtr->getProtoStateVCMassDau1();
    double massj = evPtr->getProtoStateVCMassDau2();
    double massk = evPtr->getProtoStateVCMassDau3();
    
    double yij = evPtr->getProtoStatey12();
    double yjk = evPtr->getProtoStatey23();
    double yik = evPtr->getProtoStatey13();
    double mui = evPtr->getProtoStateVCmuDau1();
    double muj = evPtr->getProtoStateVCmuDau2();
    double muk = evPtr->getProtoStateVCmuDau3();
  
    int kineMapType = getAntProto()->getKineMapType();
    double sAnt = evPtr->getm2Abs(iProto);
    double mAnt = evPtr->getmAbs(iProto);
    // Can check alternative hadronization vetos here
    if (verbose >= 7) {
      cout<<" (VS::generateBranching:) m  = "<<num2str(sqrt(sAnt))
      <<"   m12 ="<<num2str(sqrt(sij))<<"   m23 ="<<num2str(sqrt(sjk))
      <<"   m13 ="<<num2str(sqrt(sik))<<"   mi ="<<num2str(massi)
      <<"   mj ="<<num2str(massj)<<"   mk ="<<num2str(massk)<<endl;
      RotBstMatrix M;
      M.toCMframe(evPtr->getMomMot1(iProto), evPtr->getMomMot2(iProto));
      Vec4 p1cm=evPtr->getMomMot1(iProto);
      Vec4 p2cm=evPtr->getMomMot2(iProto);
      p1cm.rotbst(M);
      p2cm.rotbst(M);
      cout <<" (VS::generateBranching:) starting antenna in CM\n";
      cout <<" p1cm = "<<p1cm<<" p2cm = "<<p2cm<<endl;
    }
    
    // Set up kinematics in rest frame.
    double Ei = 1/mAnt*(pow2(massi) + sij/2 + sik/2);
    double Ej = 1/mAnt*(sij/2 + pow2(massj) + sjk/2);
    double Ek = 1/mAnt*(sik/2 + sjk/2 + pow2(massk));
    double api = sqrt( pow2(Ei) - pow2(massi) );
    double apj = sqrt( pow2(Ej) - pow2(massj) );
    double apk = sqrt( pow2(Ek) - pow2(massk) );
    double cosij = (Ei*Ej - sij/2)/(api*apj);
    // use positive square root for sine
    double sinij = sqrt(abs(1.0 - pow2(cosij)));
    double cosik = (Ei*Ek - sik/2)/(api*apk);
    double sinik = sqrt(abs(1.0 - pow2(cosik)));
    // make sure that cosine and sine are both between zero and one
    if (verbose >= 3) {
      if (!(-1 <= cosij && cosij <= 1 && -1 <= sinij && sinij <= 1
            && -1 <= cosik && cosik <= 1 && -1 <= sinik && sinik <= 1
            && 0 < Ei && 0 < Ej && 0 < Ek && 0 < api && 0 < apj && 0 < apk)){
        double gDetDimless = gDet(yij, yjk, yik, mui, muj, muk);
        printOut("generateKinematics",
                 "inconsistent invariants, values are\nsAnt = "
                 + num2str(sAnt) + ",\nsij = "
                 + num2str(sij) + ",\nsik = " + num2str(sik)
                 + ",\nsjk = " + num2str(sjk) + ",\nmi = "
                 + num2str(massi) + ",\nmj = " + num2str(massj)
                 + ",\nmk = " + num2str(massk) + ",\nEi = "
                 + num2str(Ei) + ",\nEk = " + num2str(Ek)
                 + ",\nEk = " + num2str(Ek) + ",\napi = " + num2str(api)
                 + ",\napj = " + num2str(apj) + ",\napk = "
                 + num2str(apk) + ",\ngdetdimless = "
                 + num2str(gDetDimless)
                 );
      }
    }
    // Check
    if (verbose >= 3) {
      double smiChk = pow2(massi);
      double smjChk = pow2(massj);
      double smkChk = pow2(massk);
      double sijChk = sij + smiChk + smjChk;
      double sikChk = sik + smiChk + smkChk;
      double sjkChk = sjk + smjChk + smkChk;
      double xChk=2.0*sqrt(sAnt);
      double EiChk=(sAnt-sjkChk+smiChk)/xChk;
      double EjChk=(sAnt-sikChk+smjChk)/xChk;
      double EkChk=(sAnt-sijChk+smkChk)/xChk;
      double apiChk=sqrt(pow2(EiChk)-smiChk);
      double apjChk=sqrt(pow2(EjChk)-smjChk);
      double apkChk=sqrt(pow2(EkChk)-smkChk);
      double cosijChk=(2*EiChk*EjChk+smiChk+smjChk-sijChk)/2.0/apiChk/apjChk;
      double sinijChk=sqrt(abs(1.0-cosijChk*cosijChk));
      double cosikChk=(2*EiChk*EkChk+smiChk+smkChk-sikChk)/2.0/apiChk/apkChk;
      double sinikChk=sqrt(abs(1.0-cosikChk*cosikChk));
      if (abs(sinij-sinijChk) > TINY || abs(sinik-sinikChk) > TINY
          || abs(apk-apkChk) > TINY || abs(apj-apjChk) > TINY
          || abs(api-apiChk) > TINY) {
        printOut("generateBranching",string("Error! consistency check failed.")
                 + string(" Old != New "));
        if (verbose >= 3) {
          cout<<" New : |p| = " <<api<<" "<<apj<<" "<<apk<<" sinij = "<<sinij<<" sinik = "<<sinik<<endl;
          cout<<" Old : |p| = " <<apiChk<<" "<<apjChk<<" "<<apkChk<<" sinij = "<<sinijChk<<" sinik = "<<sinikChk<<endl;
          cout<<" Flavors = "<<evPtr->getProtoStateIdDau1()<<" "<<evPtr->getProtoStateIdDau2()<<" "
          <<evPtr->getProtoStateIdDau3()<<" masses = "<<massi<<" "<<massj<<" "<<massk
          <<" vs mChk = "<<sqrt(smiChk)<<" "<<sqrt(smjChk)<<" "<<sqrt(smkChk)
          <<endl;
        }
      } else {
        if (verbose>=6) {
          printOut("VS::generateBranching",
                   string("OK: consistency check passed. Old = New "));
        }
        
        
      }
    }
    
    // Set momenta in CMz frame
    // (= frame with 1 oriented along positive z axis and event in (x,z) plane)
    Vec4 p1(0.0,0.0,api,Ei);
    Vec4 p2(-apj*sinij,0.0,apj*cosij,Ej);
    Vec4 p3(apk*sinik,0.0,apk*cosik,Ek);
    
    // Verbose output
    if (verbose >= 8) {
      cout <<" (generateBranching:) configuration in CM* (def: 1 along z)\n";
      cout <<" k1* =  "<<p1<<" k2* =  "<<p2<<" k3* =  "<<p3<<endl;
    }
    
    // Choose azimuthal (phi) angle around antenna axis
    double phi = 2*M_PI*rndmPtr->flat();
    
    // Choose Global rotation around axis perpendicular to event plane
    double psi;
    
    // Check whether any of the daughters are massive, complain if
    //   kineMapType is 1 or 2 which are only implemented for the m=0 case
    if (mui >= TINY ||  muj >= TINY || muk >= TINY) {
      // Massive particles
      // MR 7.7.2010: massive generalization of the Kosower map as in the notes
      // the formula is unstable for sik close to its minimum, maybe additional
      // checks are needed
      // I and K  denote the parents
      double gDetDimless = gDet(yij, yjk, mui, muj, muk);
      double gdet = pow3(sAnt)*gDetDimless;
      double mI = evPtr->getVCMassMot1(iProto);
      double mK = evPtr->getVCMassMot2(iProto);
      double sigmasqua = sAnt + pow2(mI) - pow2(mK);
      double sIK = sAnt - pow2(mI) - pow2(mK);
      double sIKmin = 2*mI*mK;
      double sijmin = 2*massi*massj;
      double sjkmin = 2*massj*massk;
      double sikmin = 2*massi*massk;
      double rantmap =
      (
       sigmasqua+
       sqrt( pow2(sIK) - pow2(sIKmin) )*
       ((sjk-sjkmin)-(sij-sijmin))/(sij-sijmin + sjk-sjkmin)
       )/(2*sAnt);
      double bigRantmap =
      sqrt(
           16*gdet*
           (sAnt*rantmap*(1-rantmap) - (1 - rantmap)*pow2(mI)
            - rantmap*pow2(mK))
           +(pow2(sik) - pow2(sikmin))*(pow2(sIK) - pow2(sIKmin))
           );
      double p1dotpI =
      (sigmasqua*(pow2(sik) - pow2(sikmin))*
       (sAnt + pow2(massi) - pow2(massj) - pow2(massk) - sjk)
       +8*rantmap*(sAnt + pow2(massi) - pow2(massj) - pow2(massk) - sjk)*gdet
       -bigRantmap*(pow2(sik) - pow2(sikmin) + sij*sik-2*sjk*pow2(massi)))
      /(4*(4*gdet+sAnt*(pow2(sik) - pow2(sikmin))));
      // need the norm of the three-momentum and the energy of the first parent
      // particle
      double apI =
      sqrt(
           (pow2(sAnt) + pow4(mI) + pow4(mK) - 2*sAnt*pow2(mI)
            - 2*sAnt*pow2(mK) -2*pow2(mI)*pow2(mK))
           )/(2*mAnt);
      double EI = sqrt( pow2(apI) + pow2(mI) );
      double cospsi = ((Ei*EI) - p1dotpI)/(api*apI);
      // restrict cos(psi) to (-1,1)
      cospsi = min(1.0, cospsi);
      cospsi = max(-1.0, cospsi);
      psi = acos( cospsi );
    } else {
      if (kineMapType == 1) {
        // ARIADNE map.
        psi = Ek*Ek/(Ei*Ei+Ek*Ek)*(M_PI-acos(cosik));
      } else if (kineMapType == 2) {
        // psi PYTHIA-like. "Recoiler" remains along z-axis
        psi = 0.;
        if (sij < sjk || (sij == sjk && rndmPtr->flat() > 0.5) ) psi=M_PI-acos(cosik);
      } else if (kineMapType == 3) {
        // Kosower's map. Similar to ARIADNE.
        double f=yjk/(yij+yjk);
        double rho=sqrt(1.0+4.0*f*(1.0-f)*yij*yjk/yik);
        double yaa=-((1.0-rho)*yik+2.0*f*yij*yjk)/2.0/(1.0-yij);
        // Bug corrected for v 1.019
        //    psi=1.0+2.0*yaa/(1.0-yjk);
        psi=acos(1.0+2.0*yaa/(1.0-yjk));
      }
      // MR 11.7.2010: implement massive dipole mapping, should
      // be type 2 but I write it down as type 4 to separate it
      else if ( kineMapType == 4 ) {
        if (verbose>=7) {
          printOut("generateKinematics", "using map 4");
        }
        // parton 3 is the default recoiler
        psi = M_PI - acos(cosik);
        if ( !(sij < sjk) ) {
          if (verbose>=8) {
            printOut("generateKinematics",
                     "parton 1 is chosen as the recoiler");
          }
          psi = 0;
        }
      }
      else {
        // if the kineMapType is not recognized, complain and return
        // false
        if (verbose >= 3) {
          printOut("generateKinematics", "kineMapType "
                   + num2str( kineMapType )
                   + " not recognized, returning false ");
        }
        return false;
      }
    }
    
    // Perform global rotations
    p1.rot(psi,phi);
    p2.rot(psi,phi);
    p3.rot(psi,phi);
    
    // verbose output
    if (verbose >= 7) {
      cout <<" (generateBranching:) phi = "<<phi<<" psi = "<<psi<<endl;
      cout <<" (generateBranching:) final momenta in CM\n";
      cout <<" k1cm = "<<p1<<" k2cm = "<<p2<<" k3cm = "<<p3<<endl;
    }
    
    // Rotate and boost to LAB frame
    RotBstMatrix M;
    M.fromCMframe(evPtr->getMomMot1(iProto),evPtr->getMomMot2(iProto));
    if (verbose >= 8) {
      cout <<" (generateBranching:) boosting to LAB frame defined by\n";
      cout <<" p1 =   "<<evPtr->getMomMot1(iProto)<<" p2 =   "
      << evPtr->getMomMot2(iProto) <<endl;}
    
    p1.rotbst(M);
    p2.rotbst(M);
    p3.rotbst(M);
    if (verbose >= 7) {
      cout <<" (generateBranching:) final momenta in LAB\n";
      cout <<" k1 =   "<<p1<<" k2 =   "<<p2<<" k3 =   "<<p3<<endl;}
    
    // Save momenta
    evPtr->setProtoStateMomenta(p1, p2, p3);
    
    // Verbose output: check momentum mapping by explicit reclustering
    if (verbose >= 3) {
      vector<Vec4> pNew, pClu;
      pNew.push_back(evPtr->getProtoStateMomDau1());
      pNew.push_back(evPtr->getProtoStateMomDau2());
      pNew.push_back(evPtr->getProtoStateMomDau3());
      // In the massive case, the map needs to know the parent
      // masses, use a variant vc3to2massive which is called for massive
      // daughter particles (again, technically I should check for massive
      // parents as well)
      bool clusterok = false;
      if (mui >= TINY ||  muj >= TINY || muk >= TINY) {
        double mI = evPtr->getVCMassMot1(iProto);
        double mK = evPtr->getVCMassMot2(iProto);
        // Massive case only implemented for types >= 3
        int kineMapTemp = max(3,kineMapType);
        clusterok = vinCluPtr->vc3to2massive(kineMapTemp,pNew,0,1,2,mI,mK,pClu);
        if (!clusterok && verbose >= 3)
          printOut("VinciaShower::generateKinematics",
                   "vc3to2massive returned false");
      }
      else {
        clusterok = vinCluPtr->vc3to2(kineMapType,pNew,0,1,2,pClu);
        if (!clusterok && verbose >= 3)
          printOut("VinciaShower::generateKinematics",
                   "vc3to2 returned false");
      }
      // Check (e,p) conservation

      // dimensionless comparison
      Vec4 pI = evPtr->getMomMot1(iProto);
      Vec4 pK = evPtr->getMomMot2(iProto);
      double DeltaEP = (  pow2(pClu[0].px()-pI.px())
                        + pow2(pClu[0].py()-pI.py())
                        + pow2(pClu[0].pz()-pI.pz())
                        + pow2(pClu[0].e() -pI.e())
                        + pow2(pClu[1].px()-pK.px())
                        + pow2(pClu[1].py()-pK.py())
                        + pow2(pClu[1].pz()-pK.pz())
                        + pow2(pClu[1].e() -pK.e()));
      DeltaEP /= sAnt;
      if ( !(clusterok && abs(DeltaEP) < TINY) ) {
        // Since the on-shell condition for the reclustered
        // momenta seems to be the most sensitive quantity, have a
        // reduced message if vc3to2 returned false but the reclustering
        // was within the limits
        if (!clusterok && abs(DeltaEP) < TINY && ! isnan(DeltaEP) ) {
          printOut("VinciaShower::generateKinematics", "   ... but "
                   + string("reclustering test passed.") );
          // print all the detail if verbose >= 4
          if (verbose >= 4) {
            cout << "p1    = " << p1;
            cout << "p2    = " << p2;
            cout << "p3    = " << p3;
            cout << "pahat = " << pClu[0];
            cout << "pbhat = " << pClu[1];
            cout << "pI    = " << pI;
            cout << "pK    = " << pK;
            cout << setprecision(16) << scientific;
            cout << "TINY                 = " << TINY << endl;
            cout << "m(p1)                = " << p1.mCalc() << endl;
            cout << "m(p2)                = " << p2.mCalc() << endl;
            cout << "m(p3)                = " << p3.mCalc() << endl;
            cout << "m(pI)                = " << pI.mCalc() << endl;
            cout << "m(pK)                = " << pK.mCalc() << endl;
            cout << "m(pahat)   = " << pClu[0].mCalc() << endl;
            cout << "m(pbhat)   = " << pClu[1].mCalc() << endl;
            cout << "DeltaEP = " << DeltaEP << endl;
            cout << "Ecm         = " << mAnt << endl;
            cout << "sij         = " << sij << endl;
            cout << "sjk         = " << sjk << endl;
            cout << "psi         = " << psi << endl;
            cout << "phi         = " << phi << endl;
          }
          else {
            printOut("generateKinematics", "reclustering failed");
            // If the reclustering fails, give a lot of information
            cout << "p1    = " << p1;
            cout << "p2    = " << p2;
            cout << "p3    = " << p3;
            cout << "pahat = " << pClu[0];
            cout << "pbhat = " << pClu[1];
            cout << "pI    = " << pI;
            cout << "pK    = " << pK;
            cout << scientific << setprecision(16);
            cout << "TINY                = " << TINY << endl;
            cout << "m(p1)                = " << p1.mCalc() << endl;
            cout << "m(p2)                = " << p2.mCalc() << endl;
            cout << "m(p3)                = " << p3.mCalc() << endl;
            cout << "m(pI)                = " << pI.mCalc() << endl;
            cout << "m(pK)                = " << pK.mCalc() << endl;
            cout << "m(pahat)   = " << pClu[0].mCalc() << endl;
            cout << "m(pbhat)   = " << pClu[1].mCalc() << endl;
            cout << "DeltaEP = " << DeltaEP << endl;
            cout << "Ecm         = " << mAnt << endl;
            cout << "sij         = " << sij << endl;
            cout << "sjk         = " << sjk << endl;
            cout << "phi         = " << phi << endl;
          }
        }
        if (verbose >= 7) {
          /*
           cout<<" Checking reclustering to "<<trial->iOld1<<"-"
           <<trial->iOld2<<" using map "<<kineMapType<<endl;
           cout<<"      pahat = "<<num2str(pClu[0].px())<<" "<<num2str(pClu[0].py())
           <<" "<<num2str(pClu[0].pz())<<" "<<num2str(pClu[0].e());
           cout<<"      pbhat = "<<num2str(pClu[1].px())<<" "<<num2str(pClu[1].py())
           <<" "<<num2str(pClu[1].pz())<<" "<<num2str(pClu[1].e())<<endl;
           cout<<"      "<<setw(5)<<trial->iOld1<<" = "<<num2str(trial->old1.px())
           <<" "<<num2str(trial->old1.py())
           <<" "<<num2str(trial->old1.pz())
           <<" "<<num2str(trial->old1.e());
           cout<<"      "<<setw(5)<<trial->iOld2<<" = "<<num2str(trial->old2.px())
           <<" "<<num2str(trial->old2.py())
           <<" "<<num2str(trial->old2.pz())
           <<" "<<num2str(trial->old2.e())<<endl;
           */
        }
      }
    }
    return true;
  }
  
  bool VinciaShower::generateFourMomentaIF() const {
    int iProto = evPtr->getProtoStateIndex();
    double saj;
    double sjk;
    Vec4 pA;
    Vec4 pK;
    Vec4 pb;
    if (evPtr->isIncomingMot1(iProto)) {
      pA = evPtr->getMomMot1(iProto);
      pK = evPtr->getMomMot2(iProto);
      pb = evPtr->isMot1inBeamA(iProto) ? evPtr->getpB(iProto)
                                     : evPtr->getpA(iProto);
      saj = evPtr->getProtoStates12();
      sjk = evPtr->getProtoStates23();
    }
    else {
      pA = evPtr->getMomMot2(iProto);
      pK = evPtr->getMomMot1(iProto);
      pb = evPtr->isMot2inBeamA(iProto) ? evPtr->getpB(iProto)
                                     : evPtr->getpA(iProto);
      saj = evPtr->getProtoStates23();
      sjk = evPtr->getProtoStates12();
    }
    double sAb = +m2(pA + pb);
    double mR2 = +m2(pA + pb - pK);
    double sAK = -m2(pA - pK);
    double f2 = momFracjIF(saj, sjk, sAK, kineMapTypeIF);
    
    pair<double, double> sajbMinMaxNow =
      sajbMinMax(sAb, mR2, saj, sjk, sAK, f2);
    double sajbMin = sajbMinMaxNow.first;
    double sajbMax = sajbMinMaxNow.second;
    
    double isajb = M_PI*rndmPtr->flat();
    double sajb = sajbMin*(1.0+cos(isajb))/2.0 + sajbMax*(1.0-cos(isajb))/2.0;
    
    
    double sign = rndmPtr->flat() < 0.5 ? +1.0 : -1.0;
    std::tr1::array<Vec4, 3> papjpKbeforeAlign =
      constructMomentaIF(pA, pb, pK, saj, sjk, sajb, sign, f2);
    
    Vec4 paBeforeAlign = papjpKbeforeAlign[0];
    Vec4 pjBeforeAlign = papjpKbeforeAlign[1];
    Vec4 pkBeforeAlign = papjpKbeforeAlign[2];
    
    AlignmentBoostIF align(pA, paBeforeAlign, pb);
    
    Vec4 pa = align(paBeforeAlign);
    Vec4 pj = align(pjBeforeAlign);
    Vec4 pk = align(pkBeforeAlign);
    
    // check in this order
    //  1) momenta are on-shell
    //  2) antenna momentum is conserved
    //  3) momenta correspond to invariants
    //  4) momenta cluster back to parents
    if (verbose >= 3) {
      double offShella = m2abs(pa)/sAK;
      if (offShella > TINY) {
        pa.e( pa.pAbs() );
      }
      double offShellj = m2abs(pj)/sAK;
      if (offShellj > TINY) {
        pj.e( pj.pAbs() );
      }
      double offShellk = m2abs(pk)/sAK;
      if (offShellk > TINY) {
        pk.e( pk.pAbs() );
      }
      Vec4 deltapAnt = paBeforeAlign - pjBeforeAlign - pkBeforeAlign - pA + pK;
      if (pow2(maxAbs(deltapAnt))/sAK > TINY) {
        printOut("VS::generateFourMomentaIF",
                 "antenna momentum not conserved by " + vec4toStr(deltapAnt));
        return false;
      }
      double sajCheck = -m2(pa-pj);
      double sjkCheck = +m2(pj+pk);
      double sajbCheck = +m2(pa-pj+pb);
      double deltasaj = relErrAbs(saj, sajCheck);
      double deltasjk = relErrAbs(sjk, sjkCheck);
      double deltasajb = relErrAbs(sajb, sajbCheck);
      // Note: changed to a very high tolerance because it fires too often
      if (max(max(deltasaj, deltasjk), deltasajb) > 1e-3 && verbose >= 4) {
        printOut("VS::generateFourMomentaIF",
                 "momenta don't reproduce input invariants"
                 ", saj = " + num2str(saj, 0) +
                 ", sajCheck = " + num2str(sajCheck, 0) +
                 ", relErr = " + num2str(deltasaj, 0) +
                 ", sjk = " + num2str(sjk, 0) +
                 ", sjkCheck = " + num2str(sjkCheck, 0) +
                 ", relErr = " + num2str(deltasjk, 0) +
                 ", sajb = " + num2str(sajb, 0) +
                 ", sajbCheck = " + num2str(sajbCheck, 0) +
                 ", relErr = " + num2str(deltasajb, 0));
        return false;
      }
      
      pair<Vec4, Vec4> pApKcheck =
        clusterMomentaIF(paBeforeAlign, pjBeforeAlign, pkBeforeAlign, f2);
      Vec4 deltaA = pA - pApKcheck.first;
      Vec4 deltaK = pK - pApKcheck.second;
      // triggers too often on TINY, use 1e-3 to only catch gross cases
      double relErrAK = pow2(max(maxAbs(deltaA), maxAbs(deltaK)))/sAK;
      if (relErrAK > 1e-3) {
        printOut("VinciaShower::generateFourMomentaIF",
                 "re-clustering check failed, relErrAK = "
                 + num2str(relErrAK, 0));
      }
    }
    
    if (evPtr->isIncomingMot1(iProto)) {
      evPtr->setProtoStateMomenta(pa, pj, pk, align);
    }
    else {
      evPtr->setProtoStateMomenta(pk, pj, pa, align);
    }

    return true;
  }
  
  bool VinciaShower::generateFourMomentaII() const {
    // Construct the post-branching momenta in the realigned frame in which
    //   the incoming momenta are aligned with the beam
    int iProto = evPtr->getProtoStateIndex();
    double sij = evPtr->getProtoStates12();
    double sjk = evPtr->getProtoStates23();
    double yij = evPtr->getProtoStatey12();
    double yjk = evPtr->getProtoStatey23();
    int idi = evPtr->getProtoStateIdDau1();
    int idj = evPtr->getProtoStateIdDau2();
    int idk = evPtr->getProtoStateIdDau3();
    pair<double, double> rirk =
      evPtr->rescaleikII(iProto, yij, yjk, idi, idj, idk);
    double m2ant = evPtr->getm2Abs(iProto);
    
    Vec4 pI = evPtr->getMomMot1(iProto);
    Vec4 pK = evPtr->getMomMot2(iProto);
    Vec4 pi = pI/rirk.first;
    Vec4 pk = pK/rirk.second;
    double e2 = (sij/pi.e() + sjk/pk.e())/4.0;
    double signcostheta = evPtr->isMot1inBeamA(iProto) ? -1.0 : 1.0;
    double costheta = signcostheta*(sij/pi.e() - sjk/pk.e())/(4.0*e2);
    if (verbose >= 3 && abs(costheta) > 1.0 + TINY) {
      printOut("VS::generateFourMomentaII", "cos(theta_j) unphysical");
      if (verbose >= 4) {
        cout << "pI = " << pI << endl;
        cout << "pK = " << pK << endl;
        cout << "yij = " << yij << endl;
        cout << "yjk = " << yjk << endl;
        cout << "(r_i, r_k) = (" << rirk.first << ", " << rirk.second << ")"
             << endl;
      }
    }
    costheta = min(max(costheta, -1.0) , 1.0);
    double sintheta = sqrt(1-pow2(costheta));
    double phi = 2*M_PI*rndmPtr->flat();
    Vec4 pj(e2*sintheta*sin(phi), e2*sintheta*cos(phi), e2*costheta, e2);
    
    double offShellj = m2abs(pj)/m2ant;
    if (!(offShellj < TINY)) {
      if (verbose >= 8) {
        printOut("VS::generateFourMomenta", "setting pj on-shell manually");
      }
      pj.e(sqrt(pj.pAbs2() + pow2(evPtr->VCMass(idj))));
    }
    
    Vec4 q    = pI + pK;
    Vec4 qBar = pi - pj + pk;
    
    AlignmentBoostII align(q, qBar);
    
    AlignmentBoostII boostBack(qBar, q);
    Vec4 piBefore = boostBack(pi);
    Vec4 pjBefore = boostBack(pj);
    Vec4 pkBefore = boostBack(pk);
    
    // check momentum conservation
    if (verbose >= 3) {
      Vec4 deltapAnt = q - (piBefore - pjBefore + pkBefore);
      // Using TINY this triggers repeatedly
      if (pow2(maxAbs(deltapAnt))/m2ant > 1e-5) {
        printOut("VS::generateFourMomentaII",
                 "antenna momentum not conserved, deviation measure "
                 + num2str(pow2(maxAbs(deltapAnt))/m2ant, 0) +
                 ", absolute difference " + vec4toStr(deltapAnt));
        return false;
      }
    }
    
    evPtr->setProtoStateMomenta(pi, pj, pk, align);
    
    return true;
  }
  
//*********

// Set event weight, check for non-unity and negative weights

  bool VinciaShower::setWeight(double weightIn, int iWeightIn) {

    // Verbose output
    if (verbose >= 7) {
      printOut("VS::setWeight","begin --------------");
    }

    if (! existsWeight(iWeightIn) ) return false;
    
    // For default / USR weight set, check for nonunity event weights.
    if ( iWeightIn == 0 ) {
      if (abs(abs(weightIn) - 1.0) > TINY && 
          abs(abs(weightSave[iWeightIn])-1.0) < TINY) {
        reweightingOccurred = true;     
        if (nNonunityWeight <= 5.0) 
          if (verbose >= 1) 
            printOut("VS::setWeight","Nonunity weight occurred, w = "+
                     num2str(weightIn)+((abs(nNonunityWeight-5.0) < TINY) ? 
                                        ": further warnings suppressed" : " "));
      }
      
      // Check for negative weights 
      if (weightSave[iWeightIn] > 0.0 && weightIn < 0.0) {
        if (nNegativeWeight <= 5)
          if (verbose >= 1) 
            printOut("VS::setWeight","Negative weight occurred, w = "+
                     num2str(weightIn)+((nNegativeWeight == 5) ? 
                                        ": further warnings suppressed" : " "));
      } 

    }

    // Save new weight
    weightSave[iWeightIn] = weightIn;

    // Everything ok
    return true;
    
  }
  
//*********
// Functions to update the trial statistics
  void VinciaShower::updateTrialCount(tr1::array<map<string, TrialCount>, 3>& m) {
    ++m[0][branchingStringKinematics()];
    ++m[1][branchingStringShort()];
    ++m[2][branchingStringLong()];
  }
  
//*********


//*********

  // TO DO: display information for massless and massless polarized antennae,
  //        as well as for the massive antennae
// OUTPUT ROUTINES

void VinciaShower::printOut(int level, string place, string message) const {
  if (level <= verbose) {
    Vincia::printOut(place, message);
  }
}
  
void VinciaShower::printOut(string place, string message) const {
  Vincia::printOut(place, message);
}
  
// Print header information
void VinciaShower::header() {

    // Avoid printing header several times
    headerIsPrinted = true;

    // Verbose output
    if (verbose >= 7) {
      printOut("VS::header","begin --------------");
    }
  
    cout<<setprecision(3);
    cout.setf(ios::left);
    cout << "\n";
    cout << " *-------  VINCIA "<<fixed<<setw(6)<<version
    <<" Global Initialization  --------------------------------------*\n";
    cout << " | \n";
    cout << " | Vincia                             = "
         <<bool2str(vinciaOn,9) <<"\n |\n";
    if (vinciaOn) {
      cout << " | Matching (MECs):                   = ";
      if (matching && matchingLO >= 1) {
        cout << bool2str(true,9)<<"\n"; 
        cout <<" |                 LO                 = "
             << num2str(matchingLO,9);
        if (matchingFullColor) cout<<"  (FullColor)";
        else cout<<"  (LeadingColor)";
        cout<<"\n";
        cout <<" |                 NLO                = "
             << num2str(matchingNLO,9);
        if (matchingNLO >= 2) cout<<"  (LeadingColor)";
        cout<<"\n";
        cout <<" |                 regOrder           = "
             << num2str(matchingRegOrder,9)<<endl;
        if (matchingScaleIsAbs) 
          cout <<" |                 regScale           = "
               << num2str(matchingScale,9)<<endl;
        else 
          cout <<" |                 regScaleRatio      = "
               << num2str(matchingScaleRatio,9)<<endl;
        if (verbose >= 2) {
          cout <<" |                 regShape           = "
               << num2str(matchingRegShape,9)<<endl;
          cout <<" |                 regType            = "
               << num2str(matchingRegType,9)<<endl;
        }
        cout <<" |                 MGInterface        = "
             << bool2str(isMGOn,9)<<"\n";
      }    
      else {
        cout<<bool2str(false,9)<<"\n";
      }
      cout << " |\n";
      cout << " | Shower:         evolutionType      = "
           << num2str(evolutionType,9)
           << "  (1:pT, 2:mD)"<<"\n";
      cout << " |                 orderingMode       = "
           << num2str(orderingMode,9)
           << "  (1:strong, 2:smooth)" <<"\n";
      cout << " |                 helicityShower     = "
           << bool2str(helicityShower,9)<<"\n";
      cout << " |                 sectorShower       = "
           << bool2str( sectorShower,9)<<"\n";
      cout << " |                 nGluonToQuark      = "
           << num2str(nGluonToQuark,9)<<"\n";
      cout << " |                 gluonSpl.Corr.     = "
           << bool2str( gluonSplittingCorrection,9)<<"\n";
      cout << " |                 relativisticSpeed  = "
           << num2str(relativisticSpeed,9)<<"\n";
      cout << " |                 Perturbative CR    = "
           << bool2str( (CRmode >= 2), 9)<<"  (CRmode = "
           << num2str(CRmode,1)<<")\n";
    if (verbose >= 2) {
      cout << " |                 kineMapTypeIF      = "
      << num2str(kineMapTypeIF,9)<<"\n";
      cout << " |                 kineMapTypeII      = "
      << num2str(kineMapTypeII,9)<<"\n";
    }
    cout << " |\n | Alpha_s:        alphaS(mZ)|MSbar   = "
	 << num2str(alphaSvalue,9)<<"\n"
	 << " |                 order              = "
	 << num2str(alphaSorder,9)<<"\n";
    if (alphaSorder >= 1) {
      cout << " |                 LambdaQCD[nF]      = "
           << num2str(alphaSptr->Lambda3(),9)<<"[3] "
           << num2str(alphaSptr->Lambda4(),7)<<"[4] "
           << num2str(alphaSptr->Lambda5(),7)<<"[5] "
           << num2str(alphaSptr->Lambda6(),7)<<"[6]\n";
      cout << " |                 mode               = "
	   << num2str(alphaSmode,9)<<"\n";
      cout << " |                 CMW rescaling      = "
	   << bool2str(alphaScmw,9)<<"\n";
      cout << " |                 kMu                = "
	   << num2str(alphaSkMu,9)<<"\n";
      cout << " |                 alphaSmuFreeze     = "
           << num2str(alphaSmuFreeze,9)<<"\n";
      cout << " |                 alphaSmax          = "
           << num2str(alphaSmax,9)<<"\n";
    }
    
    cout << " |\n"
         << " | IR Reg.:        cutoffType         = "
         << num2str(cutoffType,9)
         <<(cutoffType== -1 ? " (Pythia 8 pTevol)" :"")<<"\n"
         << " |                 cutoffScale        = "
         << num2str(cutoffScale,9)<<"\n";
    if (matchingLO > 0) {
      cout << " |                 matchingIRcutoff   = "
           << num2str(matchingIRcutoff,9)<<"\n";
    }
      
    
    cout << " |" << endl
         << " | Colour" << endl
         << " | Reconnections:  CRmode             = "
         << num2str(CRmode, 9)
         << "  (0:off, 1:non-perturbative only, 2:all)" << endl;
    
    
    if ( settingsPtr->word("Vincia:antennaFile") != "none") {
      cout<<" | Antennae:       antennaFile              = "
          <<settingsPtr->word("Vincia:antennaFile") <<"\n |\n";
    }
    
    if (verbose >= 2) {
      for (int iAnt=0;iAnt<getNAnt();++iAnt) {
        cout.setf(ios::left);
        cout<<setprecision(2);
  cout<<" | *------------------------------------------------------------------------\n";
	cout<<" | * "<<getAnt(iAnt)->vinciaName()<<" ["<<
	  getAnt(iAnt)->humanName()<<"]"<<endl;
	if (getAnt(iAnt)->isOn()) { 
	  // Print color/charge factor
	  double chargeFactor = getAnt(iAnt)->getChargeFactor();
	  cout<<" |   chargeFactor       = "<<fixed
	      <<setw(6)<<chargeFactor<<"\n";
	  int kineMapType = getAnt(iAnt)->getKineMapType();
	  cout<<" |   kineMapType        = "
	      <<setw(6)<<kineMapType<<"\n";
          cout<<" |   useCollinearTerms  = "<<setw(6)<<
            ( useCollinearTerms ? "on":"off")
              <<"\n"; 
          cout<<" |   useFiniteTerms     = "<<setw(6)<<
            ( useFiniteTerms ? "on":"off")<<"\n";
	  cout << " | \n";
	}
	
        // Switched off
        else {
          cout<<" |   OFF \n | \n";          
        }
      }
    }
    cout <<     " |-------------------------------------------"
         << "------------------------------------------\n |\n";
    cout << " | References :"<<endl;
    cout << " |    VINCIA     : Giele, Kosower, Skands, PRD78(2008)014026"<<endl;
    cout << " |    PYTHIA 8   : Sjostrand, Mrenna, Skands, CPC178(2008)852"<<endl;
    if (matching && matchingLO >= 1 && isMGOn) {
      cout << " | The MADGRAPH interface relies on:"<<endl;
      cout << " |    HELAS      : Murayama et al., KEK-91-11 (1992)"<<endl;
      cout << " |    MADGRAPH 4 : Alwall et al., JHEP09(2007)028"<<endl;
    }
  }
  cout << " |\n *-------  End VINCIA Global Initialization  "
       << "-----------------------------------------*\n\n";
  cout.setf(ios::right);  
}  
  
// Print the list of antennae.

void VinciaShower::list(ostream& os) const {
  // Loop over antenna list and print it.
  evPtr->list(os);
}

void VinciaShower::printInfo(ostream& os) {    
  os << "\n";
  os << " *-------  VINCIA Statistics  -------------------------------------------------------------------------------------*\n";
  os << " |                                                                                                                 |\n"; 
  os << " |                                                                        "
     <<setw(40)<<" "<<" |\n"; 
  os << " | Number of nonunity-weight events                            = "
     <<( (nNonunityWeight-nNegativeWeight <= TINY) ? "     none                    " 
         : num2str(int(nNonunityWeight-nNegativeWeight),9)+" <-- WEIGHTED EVENTS" )<<setw(20)<<" "<<" |\n";
  os << " | Number of negative-weight events                            = "
     <<( (nNegativeWeight <= TINY) ? "     none" 
         : num2str(int(nNegativeWeight),9)) <<setw(40)<<" "<<" |\n";
  os << " |                                                                        "
     <<setw(40)<<" "<<" |\n";   
  os << " |                                   weight(i)          Avg Wt   Avg Dev  rms(dev)      kUnwt     Expected effUnw  |\n";
  os << " | This run                            i =     IsUnw       <w>     <w-1>                1/<w>   Max Wt  <w>/MaxWt  |\n";
  os << " |"<<setw(4)<<" "<<"User settings                         0 "; 
  if (abs(1.-weightSumSave[0]/nTotWeights) < TINY) os << "   yes " ;
  else os << "    no ";
  os << num2str(weightSumSave[0]/nTotWeights,9)<<" ";
  os << num2str(weightSumSave[0]/nTotWeights-1.,9)<<" ";
  os << setw(9)<<"-"<<"  ";
  os << num2str(nTotWeights/weightSumSave[0]);
      os << setw(9)<<"-"<<" ";
      os << setw(9)<<"-"<<" ";
      os <<"  |\n";
  if (uncertaintyBands) {
    for (int iVar=1; iVar<nVariations(); iVar++) {
      if (iVar== 1) os<<" |"<<setw(4)<<" "<<"Var : Default Antennae              ";
      if (iVar== 2) os<<" |"<<setw(4)<<" "<<"Var : AlphaS-Hi                     ";
      if (iVar== 3) os<<" |"<<setw(4)<<" "<<"Var : AlphaS-Lo                     ";
      if (iVar== 4) os<<" |"<<setw(4)<<" "<<"Var : Antennae-Hi                   ";
      if (iVar== 5) os<<" |"<<setw(4)<<" "<<"Var : Antennae-Lo                   ";
      if (iVar== 6) os<<" |"<<setw(4)<<" "<<"Var : NLO-Hi                        ";
      if (iVar== 7) os<<" |"<<setw(4)<<" "<<"Var : NLO-Lo                        ";
      if (iVar== 8) os<<" |"<<setw(4)<<" "<<"Var : Ord-Stronger                  ";
      if (iVar== 9) os<<" |"<<setw(4)<<" "<<"Var : Ord-mDaughter                 ";
      if (iVar==10) os<<" |"<<setw(4)<<" "<<"Var : NLC-Hi                        ";
      if (iVar==11) os<<" |"<<setw(4)<<" "<<"Var : NLC-Lo                        ";
      if (iVar==12) os<<" |"<<setw(4)<<" "<<"Var : " << alphaSPDFVarString(1);
      if (iVar==13) os<<" |"<<setw(4)<<" "<<"Var : " << alphaSPDFVarString(2);
      if (iVar==14) os<<" |"<<setw(4)<<" "<<"Var : " << alphaSPDFVarString(3);
      os << num2str(iVar,3)<<"   ";
      if (abs(1.-weightSumSave[iVar]/nTotWeights) < TINY) os << " yes " ;
      else os << "  no ";
      os << num2str(weightSumSave[iVar]/nTotWeights,9)<<" ";
      os << num2str(weightSumSave[iVar]/nTotWeights-1.,9)<<" ";
      os << setw(9)<<"-"<<"  ";
      os << ((weightSumSave[iVar] != 0.) ? num2str(nTotWeights/weightSumSave[iVar],9): num2str(0.0,9));
      os << num2str(weightMaxSave[iVar],9)<<" ";
      os << ((weightMaxSave[iVar] != 0.) ? num2str(weightSumSave[iVar]/nTotWeights/weightMaxSave[iVar],9): num2str(0.0,9))<<" ";
      os <<"  |\n";
    }
  }
  os << " |                                                                        "
     <<setw(40)<<" "<<" |\n"; 
  /*
  os << " | Largest Paccept (occurred at QE = "<<num2str(reweightMaxPSaveQ)
     <<" )               = "<<num2str(reweightMaxPSaveP)<<"   |\n";
  if (reweightMaxPSaveQ > TINY && reweightMaxQSaveQ > TINY) { 
    os << " | Largest QE with Paccept > 1 ( P = "<<num2str(reweightMaxQSaveP)
       <<" )               = "<<num2str(reweightMaxQSaveQ)<<"   |\n";
       }
  os << " |                                                                           |\n"; 
  */
  os << " *-------  End VINCIA Statistics ----------------------------------------------------------------------------------*\n\n";
}
  
  string VinciaShower::alphaSPDFVarString(int iVar) {
    stringstream os;
    if (1 <= iVar && iVar <= NVARALPHASPDF) {
      double alphaSVal = alphaSptr->alphaSofQ(91.188, iVar);
      os << "aS(m_Z)=" << setprecision(6) << left << setw(7) << alphaSVal;
      os <<  ", pSet=";
      if (!useLHAPDF[iVar]) {
        os << right << setw(2);
        os << pSet[iVar];
      }
      else {
        os << LHAPDFset[iVar];
        os << LHAPDFmember[iVar];
      }
    }
    return os.str();
  }
  
  void VinciaShower::printTrialStat(ostream& os) {
    const int widthName = 24;
    const int widthNum  = 11;
    const int precision =  2;
    os.setf(ios::scientific);
    os.precision(precision);
    
    
    // make a version of trialStat["Won"] in which each category is sorted
    // according to nWon
    tr1::array< vector< pair<string, TrialCount> >, 3> wonSorted;
    for (size_t iDet = 0; iDet < trialStat["Won"].size(); ++iDet) {
      wonSorted[iDet] =
        vector< pair<string,  TrialCount> >(trialStat["Won"][iDet].begin(),
                                            trialStat["Won"][iDet].end());
    }
    for (size_t iDet = 0; iDet < wonSorted.size(); ++iDet) {
      sort(wonSorted[iDet].begin(), wonSorted[iDet].end(),
           SecondGreaterThan<string, TrialCount>());
    }
    
    double nWonTot = 0;
    for (vector< pair<string, TrialCount> >::const_iterator
         i = wonSorted[0].begin(); i != wonSorted[0].end(); ++i) {
      nWonTot += i->second;
    }
    
    os << "Trial Statistics:" << endl;
    os << "Total number of trials nTot = ";
    os << setw(widthNum) << double(nWonTot) << endl;
    os << "nWon: number of trials" << endl;
    os << "nOut: number of trials vetoed because it was outside "
                "the physical phase space, in the wrong sector, "
                "or the on-shell check failed" << endl;
    os << "nCut: number of trials vetoed because of q_had < q_cutoff" << endl;
    os << "nSHa: number of trials vetoed because of other hadronization "
                "vetoes" << endl;
    os << "nHFl: number of trials vetoed because an incoming heavy "
                "flavour could not have converted anymore" << endl;
    os << "nVet: number of trials vetoed normally (central veto step)" << endl;
    os << "nBra: number of trials which resulted in a branching" << endl;
    os << "nErr: check of completeness of statistics (should be 0)" << endl;
    os << endl;
    os << setw(widthName) << left << " ";
    os << setw(widthNum) << right << "nWon/nTot";
    os << setw(widthNum) << right << "nOut/nWon";
    os << setw(widthNum) << right << "nCut/nWon";
    os << setw(widthNum) << right << "nSHa/nWon";
    os << setw(widthNum) << right << "nHFl/nWon";
    os << setw(widthNum) << right << "nVet/nWon";
    os << setw(widthNum) << right << "nBra/nWon";
    os << setw(widthNum) << right << "nErr/nWon";
    os << endl;
    
    vector<string> cat;
    cat.push_back("Out");
    cat.push_back("Cut");
    cat.push_back("SHa");
    cat.push_back("HFl");
    cat.push_back("Vet");
    cat.push_back("Bra");
    
    
    for (vector< pair<string, TrialCount> >::iterator
         i0 = wonSorted[0].begin(); i0 != wonSorted[0].end(); ++i0) {
      string name0 = i0->first;
      os << setw(widthName) << left << name0;
      os << setw(widthNum) << right;
      os << trialStat["Won"][0][name0]/nWonTot;
      
      int nCheck0 = trialStat["Won"][0][name0];
      
      for (vector<string>::const_iterator iCat = cat.begin();
           iCat != cat.end(); ++iCat) {
        os<< setw(widthNum) << right;
        os << trialStat[*iCat][0][name0]/trialStat["Won"][0][name0];
        nCheck0 -= trialStat[*iCat][0][name0];
      }
      os << setw(widthNum) << right;
      os << nCheck0/trialStat["Won"][0][name0];
      
      os << endl;
      
      for (vector< pair<string, TrialCount> >::iterator
           i1 = wonSorted[1].begin(); i1 != wonSorted[1].end(); ++i1) {
        string name1 = i1->first;
        if (name1.find(name0) != string::npos) {
          os << "  " << setw(widthName - 2) << left << name1;
          os << setw(widthNum) << right;
          os << trialStat["Won"][1][name1]/nWonTot;
          
          int nCheck1 = trialStat["Won"][1][name1];
          
          for (vector<string>::const_iterator iCat = cat.begin();
               iCat != cat.end(); ++iCat) {
            os<< setw(widthNum) << right;
            os << trialStat[*iCat][1][name1]/trialStat["Won"][1][name1];
            nCheck1 -= trialStat[*iCat][1][name1];
          }
          os << setw(widthNum) << right;
          os << nCheck1/trialStat["Won"][1][name1];
          
          os << endl;
          
          for (vector< pair<string, TrialCount> >::iterator
               i2 = wonSorted[2].begin(); i2 != wonSorted[2].end(); ++i2) {
            string name2 = i2->first;
            if (name2.find(name1) != string::npos) {
              os << "    " << setw(widthName - 4) << left << name2;
              os << setw(widthNum) << right;
              os << trialStat["Won"][2][name2]/nWonTot;
              
              int nCheck2 = trialStat["Won"][2][name2];
              for (vector<string>::const_iterator iCat = cat.begin();
                   iCat != cat.end(); ++iCat) {
                os << setw(widthNum) << right;
                os << trialStat[*iCat][2][name2]/trialStat["Won"][2][name2];
                nCheck2 -= trialStat[*iCat][2][name2];
              }
              os << setw(widthNum) << right;
              os << nCheck2/trialStat["Won"][2][name2];
              
              os << endl;
            }
          }
        }
      }
    }
  }
  
  // Print internal and diagnostic histograms
  void VinciaShower::printHistos( ostream& os ) {
    // Print histograms in matching class
    matchingPtr->printHistos(os);
    // Now print histograms in shower class
    map<string,Hist>::iterator iH;
    for (iH=vinciaHistos.begin();iH!=vinciaHistos.end();++iH) {
      string Hname=iH->first;
      if ( vinciaHistos[Hname].getEntries() >= 1) {
        os<<Hname<<vinciaHistos[Hname]<<endl;
      }
    }
  }
  
  // Print internal and diagnostic histrograms
  void VinciaShower::writeHistos(string fileName, string lastName) {
    // Print histograms in matching class
    matchingPtr->writeHistos( fileName, lastName );
    // Now print histograms in shower class
    map<string,Hist>::const_iterator iH;
    for (iH=vinciaHistos.begin();iH!=vinciaHistos.end();++iH) {
      string Hname=iH->first;
      if ( vinciaHistos[Hname].getEntries() >= 1) {
        string file = fileName + "-Hist-" + Hname + "." + lastName;
        file = sanitizeFileName(file);
        cout << "Writing " << file << endl;
        iH->second.table(file, true);
      }
    }
  }

  
  // Wrapper function to return a specific antenna
  // inside a specific antenna set
  // 0: User, 1: Def, 2: Max, 3: Min
  Antenna* VinciaShower::getAnt(int iAnt, bool isSectorIn, int iSet) {
    if (!isSectorIn)
      return antennaSets[iSet]->getAnt(iAnt);
    else
      return antennaSetsSct[iSet]->getAnt(iAnt);
  }
  
  int VinciaShower::getNAnt(int iSet) { return antennaSets[iSet]->getNAnt(); }
  
  // Return the antenna corresponding to the proto-state
  Antenna* VinciaShower::VinciaShower::getAntProto(int iSet) const {
    if (sectorShower) {
      return antennaSetsSct[iSet]->getAntProto(evPtr);
    }
    else {
      return antennaSets[iSet]->getAntProto(evPtr);
    }
  }
  
  BeamParticle* VinciaShower::getBeamAPtr() { return evPtr->getBeamAPtr();}
  BeamParticle* VinciaShower::getBeamBPtr() { return evPtr->getBeamBPtr();}
  
  string VinciaShower::branchingStringKinematics() const {
    int iProto = evPtr->getProtoStateIndex();
    if      (evPtr->isII(iProto)) { return "II"; }
    else if (evPtr->isIF(iProto)) { return "IF"; }
    else if (evPtr->isFF(iProto)) { return "FF"; }
    else                          { return "??"; }
  }
  string VinciaShower::branchingStringType() const {
    if      (evPtr->isProtoEmission())  { return "Emit";     }
    else if (evPtr->isProtoConvertQ())  { return "ConvertQ"; }
    else if (evPtr->isProtoConvertG())  { return "ConvertG"; }
    else if (evPtr->isProtoSplitting()) { return "Split";    }
    else                                { return "????";     }
  }
  string VinciaShower::branchingStringParents() const {
    int iProto = evPtr->getProtoStateIndex();
    int idI = evPtr->getIdMot1(iProto);
    int idK = evPtr->getIdMot2(iProto);
    string sI = id2str(idI);
    string sK = id2str(idK);
    if (abs(idI) < 0 && abs(idK) > 0) {
      swap(sI, sK);
    }
    return sI+sK;
  }
  
  void VinciaShower::initTriGenMapGlob() {
  
    // The emission trials have coefficient 1.0
    // The gluon splitting and gluon conversion trials have coefficient 0.5
    // because there are two antennae which add up to the splitting function
    // IF quark conversion has coefficient 2.0 to avoid underestimates
    // far outside in the phase space
  
    // The separate scopes prevent using a previous variable due to a typo
    {
      AntDescript qqFF;
      qqFF.absColTypeI = 1;
      qqFF.absColTypeK = 1;
      qqFF.dirI        = Direction::out;
      qqFF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > qqFFvec;
      qqFFvec.push_back(make_pair(1.0, &trialSoft));
      triGenMapGlob[qqFF] = qqFFvec;
    }
    
    {
      AntDescript qgFF;
      qgFF.absColTypeI = 1;
      qgFF.absColTypeK = 2;
      qgFF.dirI        = Direction::out;
      qgFF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > qgFFvec;
      qgFFvec.push_back(make_pair(1.0, &trialSoft));
      qgFFvec.push_back(make_pair(0.5, &trialSplitK));
      triGenMapGlob[qgFF] = qgFFvec;
    }
    
    {
      AntDescript gqFF;
      gqFF.absColTypeI = 2;
      gqFF.absColTypeK = 1;
      gqFF.dirI        = Direction::out;
      gqFF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > gqFFvec;
      gqFFvec.push_back(make_pair(1.0, &trialSoft));
      gqFFvec.push_back(make_pair(0.5, &trialSplitI));
      triGenMapGlob[gqFF] = gqFFvec;
    }
    
    {
      AntDescript ggFF;
      ggFF.absColTypeI = 2;
      ggFF.absColTypeK = 2;
      ggFF.dirI        = Direction::out;
      ggFF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > ggFFvec;
      ggFFvec.push_back(make_pair(1.0, &trialSoft));
      ggFFvec.push_back(make_pair(0.5, &trialSplitI));
      ggFFvec.push_back(make_pair(0.5, &trialSplitK));
      triGenMapGlob[ggFF] = ggFFvec;
    }
    
    {
      AntDescript qqIF;
      qqIF.absColTypeI = 1;
      qqIF.absColTypeK = 1;
      qqIF.dirI        = Direction::in;
      qqIF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > qqIFvec;
      qqIFvec.push_back(make_pair(1.0, &trialSoftIF));
      qqIFvec.push_back(make_pair(2.0, &trialConvertQIF));
      triGenMapGlob[qqIF] = qqIFvec;
    }
    
    {
      AntDescript qqFI;
      qqFI.absColTypeI = 1;
      qqFI.absColTypeK = 1;
      qqFI.dirI        = Direction::out;
      qqFI.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > qqFIvec;
      qqFIvec.push_back(make_pair(1.0, &trialSoftFI));
      qqFIvec.push_back(make_pair(2.0, &trialConvertQFI));
      triGenMapGlob[qqFI] = qqFIvec;
    }
    
    {
      AntDescript qgIF;
      qgIF.absColTypeI = 1;
      qgIF.absColTypeK = 2;
      qgIF.dirI        = Direction::in;
      qgIF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > qgIFvec;
      qgIFvec.push_back(make_pair(1.0, &trialSoftIF));
      qgIFvec.push_back(make_pair(1.0, &trialCollOutIF));
      qgIFvec.push_back(make_pair(2.0, &trialConvertQIF));
      qgIFvec.push_back(make_pair(0.5, &trialSplitKIF));
      triGenMapGlob[qgIF] = qgIFvec;
    }
    
    {
      AntDescript gqFI;
      gqFI.absColTypeI = 2;
      gqFI.absColTypeK = 1;
      gqFI.dirI        = Direction::out;
      gqFI.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > gqFIvec;
      gqFIvec.push_back(make_pair(1.0, &trialSoftFI));
      gqFIvec.push_back(make_pair(0.5, &trialSplitIFI));
      gqFIvec.push_back(make_pair(1.0, &trialCollOutFI));
      gqFIvec.push_back(make_pair(2.0, &trialConvertQFI));
      triGenMapGlob[gqFI] = gqFIvec;
    }
    
    {
      AntDescript gqIF;
      gqIF.absColTypeI = 2;
      gqIF.absColTypeK = 1;
      gqIF.dirI        = Direction::in;
      gqIF.dirK        = Direction::out;
      vector<pair<double, const TrialGenerator*> > gqIFvec;
      gqIFvec.push_back(make_pair(1.0, &trialSoftIF));
      gqIFvec.push_back(make_pair(1.0, &trialCollInIF));
      gqIFvec.push_back(make_pair(0.5, &trialConvertGIF));
      triGenMapGlob[gqIF] = gqIFvec;
      
      AntDescript ggIF(gqIF);
      ggIF.absColTypeK = 2;
      vector<pair<double, const TrialGenerator*> > ggIFvec(gqIFvec);
      ggIFvec.push_back(make_pair(0.5, &trialSplitKIF));
      triGenMapGlob[ggIF] = ggIFvec;
    }
    
    {
      AntDescript qgFI;
      qgFI.absColTypeI = 1;
      qgFI.absColTypeK = 2;
      qgFI.dirI        = Direction::out;
      qgFI.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > qgFIvec;
      qgFIvec.push_back(make_pair(1.0, &trialSoftFI));
      qgFIvec.push_back(make_pair(1.0, &trialCollInFI));
      qgFIvec.push_back(make_pair(0.5, &trialConvertGFI));
      triGenMapGlob[qgFI] = qgFIvec;
      
      AntDescript ggFI(qgFI);
      ggFI.absColTypeI = 2;
      vector<pair<double, const TrialGenerator*> > ggFIvec(qgFIvec);
      ggFIvec.push_back(make_pair(0.5, &trialSplitIFI));
      triGenMapGlob[ggFI] = ggFIvec;
    }
    
    {
      AntDescript qqII;
      qqII.absColTypeI = 1;
      qqII.absColTypeK = 1;
      qqII.dirI        = Direction::in;
      qqII.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > qqIIvec;
      qqIIvec.push_back(make_pair(1.0, &trialSoftII));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQIII1));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQIII2));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQIII3));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQKII1));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQKII2));
      qqIIvec.push_back(make_pair(1.0, &trialConvertQKII3));
      triGenMapGlob[qqII] = qqIIvec;
    }
    
    {
      AntDescript qgII;
      qgII.absColTypeI = 1;
      qgII.absColTypeK = 2;
      qgII.dirI        = Direction::in;
      qgII.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > qgIIvec;
      qgIIvec.push_back(make_pair(1.0, &trialSoftII));
      qgIIvec.push_back(make_pair(1.0, &trialCollKII));
      qgIIvec.push_back(make_pair(1.0, &trialConvertQIII1));
      qgIIvec.push_back(make_pair(1.0, &trialConvertQIII2));
      qgIIvec.push_back(make_pair(1.0, &trialConvertQIII3));
      qgIIvec.push_back(make_pair(0.5, &trialConvertGKII));
      triGenMapGlob[qgII] = qgIIvec;
    }
    
    {
      AntDescript gqII;
      gqII.absColTypeI = 2;
      gqII.absColTypeK = 1;
      gqII.dirI        = Direction::in;
      gqII.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > gqIIvec;
      gqIIvec.push_back(make_pair(1.0, &trialSoftII));
      gqIIvec.push_back(make_pair(1.0, &trialCollIII));
      gqIIvec.push_back(make_pair(0.5, &trialConvertGIII));
      gqIIvec.push_back(make_pair(1.0, &trialConvertQKII1));
      gqIIvec.push_back(make_pair(1.0, &trialConvertQKII2));
      gqIIvec.push_back(make_pair(1.0, &trialConvertQKII3));
      triGenMapGlob[gqII] = gqIIvec;
    }
    
    {
      AntDescript ggII;
      ggII.absColTypeI = 2;
      ggII.absColTypeK = 2;
      ggII.dirI        = Direction::in;
      ggII.dirK        = Direction::in;
      vector<pair<double, const TrialGenerator*> > ggIIvec;
      ggIIvec.push_back(make_pair(1.0, &trialSoftII));
      ggIIvec.push_back(make_pair(1.0, &trialCollIII));
      ggIIvec.push_back(make_pair(1.0, &trialCollKII));
      ggIIvec.push_back(make_pair(0.5, &trialConvertGIII));
      ggIIvec.push_back(make_pair(0.5, &trialConvertGKII));
      triGenMapGlob[ggII] = ggIIvec;
    }
  }
  
  // Determine the trial generators associated with an antenna
  vector<pair<double, const TrialGenerator*> >
  VinciaShower::determineTrialGenerators(int iAnt) {
    AntDescript index = evPtr->getAntDescript(iAnt);
    
    // TODO: distinguish global from sector mode
    typedef map<AntDescript, vector<pair<double, const TrialGenerator*> > > t;
    t::const_iterator entry = triGenMapGlob.find(index);
    if (entry == triGenMapGlob.end()) {
      throw ExGeneral("VinciaShower::determineTrialGenerators: no list of "
                      "trial generators found for antenna");
    }
    return triGenMapGlob[index];
  }
  
  double VinciaShower::getPDFMass(int idQ) const {
    if (abs(idQ) == 6) {
      return numeric_limits<double>::infinity();
    }
    else if (abs(idQ) == 5) {
      return pdfThresholdB;
    }
    else if (abs(idQ) == 4) {
      return pdfThresholdC;
    }
    else {
      return 0.0;
    }
  }
  
  double VinciaShower::mThresholdPDF(int idQ) const {
    return getPDFMass(idQ)/pdfScaleFactor;
  }
  
  int VinciaShower::getNfAlphaSofQ2(double q2, int iVar) const {
    return alphaSptr->nFofQ2(q2, iVar);
  }
  
  int VinciaShower::getiAlphaS(int iVar) const {
    switch (iVar) {
      case 12:
        return 1;
      case 13:
        return 2;
      case 14:
        return 3;

      default:
        return 0;
    }
  }
  
  int VinciaShower::getiPDF(int iVar) const {
    switch (iVar) {
      case 12:
        return 1;
      case 13:
        return 2;
      case 14:
        return 3;
        
      default:
        return 0;
    }
  }
  
} // end namespace VINCIA
