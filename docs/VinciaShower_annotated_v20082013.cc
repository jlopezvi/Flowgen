// VinciaShower.cc is a part of the VINCIA plug-in to the PYTHIA 8 generator.
// VINCIA is licenced under the GNU GPL version 2.
// Copyright (C) 2013 P. Skands.
// Please respect the MCnet Guidelines for Event Generator Authors and Users,
//               http://www.montecarlonet.org/GUIDELINES

// Function definitions (not found in the header) for the VinciaShower class,
// as follows:
// 1) inherited member functions from the Pythia 8 TimeShower classs
// 2) non-inherited member functions for the VinciaShower class
// 3) member functions for the Branchelemental class

// Include the Vincia headers
#include "VinciaShower.h"

using namespace Pythia8;

namespace Vincia {
  
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
  
  void VinciaShower::init( BeamParticle* beamAPtrIn,
                          BeamParticle* beamBPtrIn) {
    
    // Check if already initialized
    if (isInit) return;
    
    // Verbose level and version number
    verbose            = settingsPtr->mode("Vincia:verbose");
    version            = settingsPtr->parm("Vincia:versionNumber");
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::init","begin --------------");
    }
    
    // Store input pointers for future use.
    beamAPtr     = beamAPtrIn;
    beamBPtr     = beamBPtrIn;
    
    // Default evolution parameters
    evolutionType      = settingsPtr->mode("Vincia:evolutionType");
    // Decide whether to force strong (evolution) ordering or not
    orderingMode      = settingsPtr->mode("Vincia:orderingMode");
    // Number of active quark flavours
    nGluonToQuark      = settingsPtr->mode("Vincia:nGluonToQuark");
    // Normalization factors for types 1 and 2 evolution variables
    pTnormalization    = settingsPtr->parm("Vincia:pTnormalization");
    mDnormalization    = settingsPtr->parm("Vincia:mDnormalization");
    normalizeInputPT   = settingsPtr->flag("Vincia:normalizeInputPT");
    
    // Decide whether to impose sector ordering or not
    sectorShower       = settingsPtr->flag("Vincia:sectorShower");
    useSectorTerms     = settingsPtr->flag("Vincia:useSectorTerms");
    if ( !settingsPtr->flag("Vincia:useCollinearTerms") && sectorShower) {
      printOut("VS::init","Note - useCollinearTerms = off"
               ", so also forcing useSectorTerms = off");
      useSectorTerms = false;
    }
    // global flag for polarization
    helicityShower=settingsPtr->flag("Vincia:helicityShower");
    
    // local flag for polarization and mass. Default values
    isMassless = true;
    usePolarizationNow = helicityShower;
    
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
    
    // Flag whether to use scale() associated with particles or not
    useScale               = settingsPtr->flag("Vincia:useCreationScales");
    // Enhancement factor on scale choice
    pTmaxFudge  = settingsPtr->parm("Vincia:pTmaxFudge");
    
    // Perturbative cutoff
    cutoffType         = settingsPtr->mode("Vincia:cutoffType");
    if (cutoffType == -1) {
      printOut("VS::init",
               "Note - using Pythia to set hadronization cutoff");
      // TODO: rescale by pTnormalization, mDnormalization
      cutoffScale = 2.0 * settingsPtr->parm("TimeShower:pTmin");
      settingsPtr->parm("Vincia:cutoffScale",cutoffScale);
    } else {
      if (cutoffType == 0) cutoffType = evolutionType;
      if (evolutionType == 3 && cutoffType != evolutionType) {
        printOut("VS::init",
                 "Warning - forced cutoffType = 1 to regulate energy ordering");
        cutoffType = 1;
      }
      cutoffScale        = settingsPtr->parm("Vincia:cutoffScale");
    }
    
    // Initialize Trial Generators
    // Note: additional trial generators can be added and initialized by
    // adding them in the header and adding an additional push_back here.
    vector<TrialGenerator*> trialGenerators;
    trialGenerators.push_back(&trialSoft);
    trialGenerators.push_back(&trialCollI);
    trialGenerators.push_back(&trialCollK);
    trialGenerators.push_back(&trialSplitI);
    trialGenerators.push_back(&trialSplitK);
    for (int iGen=0; iGen < int(trialGenerators.size()); ++iGen) {
      trialGenerators[iGen]->initPtr(rndmPtr,settingsPtr);
      double evNormFactor = 1.0;
      if (evolutionType == 1) evNormFactor = pTnormalization;
      else if (evolutionType == 2) evNormFactor = mDnormalization;
      trialGenerators[iGen]->init(evolutionType,evNormFactor);
    }
    
    // Uncertainty Bands
    uncertaintyBands    = settingsPtr->flag("Vincia:uncertaintyBands");
    uncertaintyBandsKmu = settingsPtr->parm("Vincia:uncertaintyBandsKmu");
    
    // Uncertainty Bands not yet implemented for sector showers
    if (sectorShower && uncertaintyBands) {
      uncertaintyBands = false;
      settingsPtr->flag("Vincia:uncertaintyBands",false);
      if (verbose >= 2) printOut("VS::init","uncertainty bands "
                                 "are switched off for sector showers.");
    }
    
    // Number of uncertainty variations (including USR)
    // Hardcoded here, no sense to make user specifiable, since user
    // can see a table of which variations are being performed, and can
    // then just choose the weight sets he or she wants to consider.
    nWeights = 1;
    allowReweighting = settingsPtr->flag("Vincia:allowReweighting");
    if (uncertaintyBands) nWeights = 12;
    weightLimit = settingsPtr->parm("Vincia:uncertaintyLimit");
    // Resize weight and Paccept arrays to the relevant number of elements
    weightSave.resize(nWeights);
    weightSumSave.resize(nWeights);
    weightSum2Save.resize(nWeights);
    weightOld.resize(nWeights);
    weightMaxSave.resize(nWeights);
    for (int iVar=0; iVar<nWeights; iVar++) weightMaxSave[iVar]=0.0;
    Paccept.resize(nWeights);
    
    // Statistics
    nTrialsSum = 0;
    nTrialsVeto = 0;
    nTrialsHull = 0;
    nTrialsHullMassive = 0;
    nTrialsCutM = 0;
    nTrialsCutQ = 0;
    
    // Hyperjet compatibility mode on/off
    hyperjet           = settingsPtr->flag("Vincia:hyperjet");
    
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
    matchingRegShape   = settingsPtr->mode("Vincia:matchingRegShape");
    matchingRegType    = settingsPtr->mode("Vincia:matchingRegType");
    matchingScaleIsAbs = settingsPtr->flag("Vincia:matchingRegScaleIsAbsolute");
    matchingScale      = settingsPtr->parm("Vincia:matchingRegScale");
    matchingScaleRatio = settingsPtr->parm("Vincia:matchingRegScaleRatio");
    MESav = -1;
    MENew = 0.0;
    
    // Initialize headroom factors (optimized dynamically later)
    setHeadRoomEmit(1.5);
    setHeadRoomSplit(2.0);
    
    // Internal Histograms
    if (verbose >= 2) initVinciaHistos();
    
    // Mass windows
    // Note: PYTHIA's quark masses ~ constituent masses for light quarks.
    mt          = particleDataPtr->m0(6);
    mb          = particleDataPtr->m0(5);
    mtb         = sqrt(mt*mb);
    mc          = particleDataPtr->m0(4);
    ms          = particleDataPtr->m0(3);
    
    // alphaS
    alphaSvalue        = settingsPtr->parm("Vincia:alphaSvalue");
    alphaSorder        = settingsPtr->mode("Vincia:alphaSorder");
    alphaSmode         = settingsPtr->mode("Vincia:alphaSmode");
    alphaScmw          = settingsPtr->flag("Vincia:alphaScmw");
    alphaSkMu          = settingsPtr->parm("Vincia:alphaSkMu");
    alphaSmax          = settingsPtr->parm("Vincia:alphaSmax");
    alphaSmuMin        = settingsPtr->parm("Vincia:alphaSmuMin");
    alphaScmwDef       = settingsPtr->flagDefault("Vincia:alphaScmw");
    alphaSkMuDef       = settingsPtr->parmDefault("Vincia:alphaSkMu");
    
    // Initialize MSbar alphaS
    alphaSptr->init(alphaSvalue, alphaSorder);
    
    // Lambda for 5, 4 and 3 flavours: MSbar
    Lambda3flav        = alphaSptr->Lambda3();
    Lambda4flav        = alphaSptr->Lambda4();
    Lambda5flav        = alphaSptr->Lambda5();
    Lambda5flav2       = pow2(Lambda5flav);
    Lambda4flav2       = pow2(Lambda4flav);
    Lambda3flav2       = pow2(Lambda3flav);
    // Compute 6-flavor Lambda as well
    Lambda6flav        = Lambda5flav * pow(Lambda5flav/mt, 2./21.);
    Lambda6flav2       = pow2(Lambda6flav);
    
    // Check freezeout scale
    if (alphaSorder >= 1) {
      // Make sure smallest ren scale > Lambda3 * facCMW / alphaSkMu
      double kRen = alphaSkMu;
      if (alphaScmw) kRen /= alphaSptr->facCMW(3);
      alphaSmuMin = max(alphaSmuMin,1.1*Lambda3flav/kRen);
    }
    alphaSmax   = min(alphaSmax,alphaSptr->alphaS(pow2(alphaSmuMin)));
    
    // Hadronization scale > LambdaQCD
    cutoffScale = max(cutoffScale,1.1*Lambda3flav);
    
    // Smallest evolution scale > LambdaQCD
    QminAll     = 1.1*Lambda3flav;
    
    // MR 5.11.2010: initialise
    // nNegativeWeight, nNonunityWeight and nTotWeights to 0
    nNegativeWeight		   = 0;
    nNonunityWeight		   = 0;
    nTotWeights			   = 0;
    
    // Initialize negative-weight and non-unity warning flags
    reweightingOccurred = false;
    setWeightAll(0.0);
    resetWeightSums(0.0);
    reweightMaxPSaveP      = 0.0;
    reweightMaxPSaveQ      = 0.0;
    reweightMaxQSaveP      = 0.0;
    reweightMaxQSaveQ      = 0.0;
    
    // Initialize VinClu, VinCom, and MadGraph interface objects,
    vinCluPtr->init();
    vinComPtr->init();
    // Check MG interface and initialize matching class
    isMGOn = settingsPtr->flag("Vincia:MGInterface");
    if (isMGOn) {
      isMGOn = matchingPtr->init(nWeights);
      if (!matching && usePolarizationNow) isMGOn = mgInterfacePtr->init();
      settingsPtr->flag("Vincia:MGInterface",isMGOn);
      if (isMGOn && verbose >= 2) {
        printOut("VinciaPlugin()","MGInterface switched on.");
      }
    }
    
    // Initialize USR antennae. Not done before to make sure we catch if
    // something was changed in Settings before.
    antennaSets[0]->init(false);
    antennaSetsSct[0]->init(true);
    
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
  
  // Top-level routine to do a full time-like shower in resonance decay.
  
  int VinciaShower::shower(int iBeg, int iEnd, Event& event, double pTmax,
                           int nBranchMax) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::shower","begin --------------");
    }
    
    // Add new system, automatically with two empty beam slots.
    int iSys = partonSystemsPtr->addSys();
    
    // Verbose output
    if (verbose >= 8)
      printOut("VS::shower","preparing to shower. System no. "
               +num2str(iSys));
    
    // Loop over allowed range to find all final-state particles.
    Vec4 pSum;
    for (int i = iBeg; i <= iEnd; ++i) if (event[i].isFinal()) {
      partonSystemsPtr->addOut( iSys, i);
      pSum += event[i].p();
    }
    partonSystemsPtr->setSHat( iSys, pSum.m2Calc() );
    
    // Let prepare routine do the setup.
    prepare(iSys, event, false);
    
    // Begin evolution down in pT from hard pT scale.
    int nBranch = 0;
    do {
      double pTtimes = pTnext(event, pTmax, 0.);
      
      // Do a final-state emission (if allowed).
      if (pTtimes > 0.) {
        if (branch(event)) ++nBranch;
        pTmax = pTtimes;
      }
      
      // Keep on evolving until nothing is left to be done.
      else pTmax = 0.;
    } while (pTmax > 0. && (nBranchMax <= 0 || nBranch < nBranchMax));
    
    // Return number of emissions that were performed.
    return nBranch;
    
  }
  
  //*********
  
  // Prepare system for evolution.
  
  void VinciaShower::prepare(int iSys, Event& event, bool limitPTmaxIn,
                             double /*pTfirstTrialIn*/) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::prepare","begin --------------");
    }
    
    //If verbose, list event before showering
    if (verbose >= 4) {
      if (verbose >= 6 || iSys == 0) event.list();
      printOut("VS::prepare","preparing system no. "
               +num2str(iSys)+" for showering. Size = "
               +num2str(partonSystemsPtr->sizeOut(iSys))+
               " limitPTmax = "+bool2str(limitPTmaxIn));
    }
    
    // Get generated event weight from Pythia.
    // Problem: for e+e- the first one is 1
    //          for dijets, the first one is 0.
    //          how to tell the first time we are being called for an event?
    if (iSys <= 1) {
      setWeightAll(infoPtr->weight());
      for (int iVar=0; iVar<nVariations(); iVar++) {
        weightOld[iVar] = 0.0;
      }
    }
    
    // Reset dipole list for first interaction and for resonance decays.
    int iInA = partonSystemsPtr->getInA(iSys);
    if (iSys == 0 || iInA == 0) {
      branchelementals.resize(0);
      winners.resize(0);
    }
    
    // Vector to find total invariant mass of system
    Vec4 pSum;
    
    // Find and save all FS colors and anticolors
    map<int,int> indexOfAcol;
    map<int,int> indexOfCol;
    
    // Loop over event record. Find indices of FS particles with color lines
    // Make new copies so any modifications by VINCIA
    // (eg polarization assignment) are clearly recorded
    int sizeSystem = partonSystemsPtr->sizeOut(iSys);
    int iAncestor  = 0;
    vector<int> iPartons;
    for (int i = 0; i < sizeSystem; ++i) {
      int i1 = partonSystemsPtr->getOut( iSys, i);
      // Skip if not present in final state
      if ( i1 <= 0 || !event[i1].isFinal()) continue;
      int col= event[i1].col();
      int acol= event[i1].acol();
      // Check if all the partons have a single common mother
      if (iAncestor >= 0) {
        int iMot1 = event[i1].mother1();
        int iMot2 = event[i1].mother2();
        if (iMot2 == 0) iMot2 = iMot1;
        if (iAncestor == 0) iAncestor = iMot1;
        // If more than one ancestor found, set iAncestor = -1
        if (iMot1 != iAncestor || iMot2 != iMot1) iAncestor = -1;
      }
      // Sum total momentum
      pSum += event[i1].p();
      // Make new copy and store begin and end of new parton list
      // i1 = event.copy(i1,event[i1].status());
      iPartons.push_back(i1);
      // Store position in color arrays
      if (col != 0) indexOfCol[col]=i1;
      if (acol != 0) indexOfAcol[acol]=i1;
    }
    
    // Sanity check: If common ancestor found, check that it is a resonance
    if (iAncestor > 0 && ! event[iAncestor].isResonance() ) iAncestor=-1;
    isResonance = (iAncestor > 0);
    
    // Check if 1->n or 2->n
    int nIn = 0;
    if (isResonance) nIn = 1;
    // 2->n not implemented yet
    
    // Store total invariant mass and set initial unordering scale
    mSystem  = m(pSum);
    Qrestart = mSystem;
    Q2UnOrdOld      = -1.;
    mD2UnOrdOld     = -1.;
    if (verbose >= 7) {
      printOut("VS::prepare",
               "Total mass of showering system = "+num2str(mSystem));
    }
    
    // Polarization: check if VINCIA should (attempt to) assign helicities
    usePolarizationNow = helicityShower && nIn >= 1;
    if (usePolarizationNow && iPartons.size() >= 1) {
      // Check whether event is already polarized
      bool isPolarized = true;
      for (int i=0; i<int(iPartons.size()); ++i)
        if (event[i].pol() == 9) isPolarized = false;
      
      // If not, see if we can polarize it
      if (!isPolarized) {
        // Create tmp list of partons to be saved if polarization succeeds
        vector<Particle*> particlesTmp;
        particlesTmp.push_back(&event[iAncestor]);
        for (int i=0; i<int(iPartons.size()); ++i)
          particlesTmp.push_back(new Particle(event[iPartons[i]]));
        usePolarizationNow = matchingPtr->polarize(nIn,particlesTmp);
        
        // Create new (polarized) copies of original parton system
        if (usePolarizationNow) {
          for (int i=0; i<int(iPartons.size()); ++i) {
            int iOld = iPartons[i];
            // Create new polarized copy
            int iNew = event.copy(iOld,51);
            event[iNew].pol(particlesTmp[i+1]->pol());
            // Update the list of systems.
            partonSystemsPtr->replace(iSys, iOld, iNew);
            // Also update VINCIA's list of partons and color maps
            iPartons[i] = iNew;
            int col     = event[iNew].col();
            int acol    = event[iNew].acol();
            if (col != 0) indexOfCol[col]=iNew;
            if (acol != 0) indexOfAcol[acol]=iNew;
          }
        }
      }
    }
    
    // Now loop over colored particles to create branchelementals (=antennae)
    for (map<int,int>::iterator it=indexOfCol.begin();
         it != indexOfCol.end(); ++it) {
      int col  = it->first;
      int i1   = it->second;
      int i2   = indexOfAcol[col];
      if (col == 0 || i1 == 0 || i2 == 0) continue;
      if (verbose >= 7) {
        printOut("VS::prepare",
                 "Creating antenna between "+num2str(i1,4)+", "
                 +num2str(i2,4)+": col = "+num2str(col,5)
                 +" ; scales = "+num2str(event[i1].scale())
                 +", "+num2str(event[i2].scale()));
      }
      
      // Store trial QCD antenna and add trial generators depending on type
      Branchelemental trial(event,i1,i2);
      resetTrialGenerators(&trial);
      
      // Save system and ancestor (for use in matching below)
      trial.system=iSys;
      trial.iAncestor=iAncestor;
      
      // Determine starting scale
      double mDip = trial.m();
      if ( ! useScale ) {
        if ( !isResonance ) {
          Q2UnOrdOld  = mDip;
          mD2UnOrdOld = mDip;
          if (orderingMode >= 2) trial.unOrdering = true;
        }
      } else {
        double scale = min(trial.old1.scale(),trial.old2.scale());
        // If ancestor has color, use current scale as unordering scale
        if (!isResonance) {
          Q2UnOrdOld  = scale;
          mD2UnOrdOld = scale;
          if (orderingMode >= 2) trial.unOrdering = true;
        }
      }
      
      // Find neigbor on i1 side
      if (event[i1].acol() != 0) {
        int colL = event[i1].acol();
        trial.iL = indexOfCol[colL];
        // Find next-to-nearest neigbor on i1 side
        if (trial.iL != 0 && event[trial.iL].acol() != 0) {
          int colLL = event[trial.iL].acol();
          trial.iLL = indexOfCol[colLL];
        }
      }
      
      // Find neigbor on i2 side
      if (event[i2].col() != 0) {
        int colR = event[i2].col();
        trial.iR = indexOfAcol[colR];
        // Find next-to-nearest neigbor on i1 side
        if (trial.iR != 0 && event[trial.iR].col() != 0) {
          int colRR = event[trial.iR].col();
          trial.iRR = indexOfAcol[colRR];
        }
      }
      branchelementals.push_back(trial);
      
    }
    
    // Count up number of gluons and quark pairs (to compute QCD order)
    nG = 0.;
    nQQ = 0.;
    for (int i=0; i<int(branchelementals.size()); ++i) {
      if (branchelementals[i].old1.colType() == 2) ++nG;
      else if (branchelementals[i].old1.colType() == 1) ++nQQ;
    }
    
    // Sanity check
    if (branchelementals.size() <= 0) {
      if (verbose >= 5) printOut("VS::prepare",
                                 "did not find any antennae: exiting.");
      return;
    }
    
    //If verbose output, list dipoles
    if (verbose >= 5) list();
    
  }
  
  //*********
  
  // Select next pT in downwards evolution of the existing dipoles.
  
  double VinciaShower::pTnext(Event& event, double pTevolBegAll,
                              double pTevolEndAll, bool /*isFirstTrial*/) {
    
    // Translate input scales to VINCIA normalization (if asked for)
    // Denote VINCIA scales by "q", PYTHIA ones by "pTevol".
    double qOld = pTevolBegAll;
    double qEndAll = pTevolEndAll;
    double evNormFactor = 1.0;
    if ( evolutionType == 1 ) evNormFactor = pTnormalization;
    else if ( evolutionType == 2 ) evNormFactor = mDnormalization;
    if (normalizeInputPT)  {
      // Vincia pT is quadratic -> take square root
      if (evolutionType == 1) {
        qOld    *= sqrt(evNormFactor);
        qEndAll *= sqrt(evNormFactor);
      }
      // Vincia mD is linear -> no square root
      else if (evolutionType == 2) {
        qOld    *= evNormFactor;
        qEndAll *= evNormFactor;
      }
    }
    // End scale: (qEndAll is input parameter, qCutoff from hadronization,
    //             qMinAll is internal Vincia parameter)
    qEndAll = max(QminAll,qEndAll);
    
    // Initialize winner scale and trial index
    double qWin = 0.;
    
    // Verbose output
    if (verbose >= 4) {
      printOut("VS::pTnext","(re)starting evolution at scale Q = "
               +num2str(qOld));
      if (verbose >= 6)  list();
    }
    
    // Sanity checks
    if (branchelementals.size() <= 0) return 0.;
    if (!settingsPtr->flag("Vincia:FSR")) return 0.;
    
    //-------------------------------------------------------------------
    // Begin main loop over dipole-antennae
    for (int iAnt = 0; iAnt < int(branchelementals.size()); ++iAnt) {
      
      // Shorthand for current dipole-antenna
      Branchelemental* trialPtr=&branchelementals[iAnt];
      
      // Verbose output
      if (verbose >= 6) {
        printOut("VS::pTnext",
                 "* processing antenna no"+num2str(iAnt,4)
                 +" col = "+num2str(trialPtr->old1.col())
                 +" nTrialGens = "+num2str(trialPtr->nTrialGenerators(),2));
      }
      
      // Generate new trial branchings, starting from qOld
      double qBegin = qOld;
      // For unordered antennae, instead start from mass (or from scale of last
      // failed trial, which is stored in Qrestart)
      if (trialPtr->unOrdering) qBegin = Qrestart;
      
      // All evolution normalizations are limited by invariant mass of dipole
      double sAnt    = branchelementals[iAnt].s();
      double mAnt    = branchelementals[iAnt].m();
      qBegin = min(qBegin,mAnt);
      
      // Lowest evolution boundary: impose hadronization scale.
      double qEnd = qEndAll;
      // Translate the cutoff scale to a lower bound on the evolution variable
      double qCutoff = cutoffScale;
      if (cutoffType == 0 || cutoffType == evolutionType) {
        qCutoff = sqrt(evNormFactor) * cutoffScale;
      }
      // mD evolution with a cutoff in pT: larger mD hull
      else if (cutoffType == 1 && evolutionType == 2) {
        if (4*pow2(cutoffScale) > sAnt) qCutoff = qBegin;
        else
          qCutoff = sqrt( evNormFactor * sAnt
                         * (1. - sqrt(1. - 4.*pow2(cutoffScale)/sAnt)));
      }
      // pT evolution with a cutoff in mD: larger pT hull
      else if (cutoffType == 2 && evolutionType == 1) {
        qCutoff = sqrt( evNormFactor * pow(cutoffScale,4.) / sAnt);
      }
      qEnd = max(qEndAll, qCutoff);
      // Check if any phase space still open
      if (qBegin <= qEnd || trialPtr->nTrialGenerators() == 0) continue;
      
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
      
      // Loop over trial functions. Find and save max trial scale.
      double qTrialMax    = 0.0;
      int    indxMax      = -1;
      for (int indx = 0; indx < int(trialPtr->nTrialGenerators()); ++indx) {
        
        // Pointer to trial generator for this trial:
        TrialGenerator* trialGenPtr = trialPtr->trialGenPtrSav[indx];
        string trialGenName         = trialGenPtr->name();
        int iAntPhys                = trialPtr->getPhysIndex(indx);
        double qTrial               = qBegin;
        
        // Look up if there is a saved value to be reused.
        if (trialPtr->hasTrial(indx)) {
          qTrial = trialPtr->getTrialScale(indx);
          
          // Verbose output
          if (verbose >= 9) {
            cout<<"   Trial "<<indx<<" "<<trialGenName
            <<" Retrieving saved qTrial = "<<qTrial<<endl;
          }
        }
        
        // Generate new q value
        else {
          
          // MR 12.7.2010: normalize by factor
          //    Ecm / sqrt( lambda( Ecm^2, m_parent1^2, m_parent2^2 ) )
          // with Lambda the Kallen function, to account for the
          // normalisation to the two-particle phase space volume
          double psMassCorr = 1.0;
          double s1 = trialPtr->old1.m2();
          double s2 = trialPtr->old2.m2();
          if (s1 > TINY || s2 > TINY) {
            psMassCorr = sAnt/sqrt(pow2(sAnt - s1 - s2) - 4*s1*s2);
            if (verbose>=8 && psMassCorr > 1) {
              printOut("VinciaShower::pTnext", "massive phase space volume "
                       "factor is "+ num2str( psMassCorr ) );
            }
          }
          
          // Sector showers: improve efficiency by reducing max trial scale
          // for gluon emissions if we already know sector vetos will kill
          // region above qT2sectorMin
          if (sectorShower && (iAntPhys == iQQemit || iAntPhys == iQGemit
                               || iAntPhys == iGGemit))
            qTrial = min(qTrial,sqrt(qT2sectorMin));
          
          // Impose evolution windows
          bool acceptRegion = false;
          int iRegion = getRegion(qTrial);
          
          while (!acceptRegion) {
            
            // Set overestimated Z range for trial generation
            double qMinNow = getQmin(iRegion);
            double yMinNow = pow2(qMinNow)/sAnt;
            double zMinNow = trialGenPtr->getZmin(yMinNow);
            double zMaxNow = trialGenPtr->getZmax(yMinNow);
            
            // Set color and headRoom factors for trial (default = CA)
            // TODO: give headroomfactor an iRegion dependence
            double colFac      = CA;
            int nF             = getNf(iRegion);
            double headRoomFac = getHeadRoomFactorEmit(nG,nQQ);
            int colType1       = event[trialPtr->iOld1].colType();
            int colType2       = event[trialPtr->iOld2].colType();
            if (colType1 == 1 && colType2 == -1) {
              // For qq->qgqbar, allow to use CF as color factor
              colFac = max(CF,getAnt(iQQemit)->getChargeFactor());
            }
            else if (iAntPhys == iQGsplit || iAntPhys == iGGsplit) {
              // For splittings, use nF*TR as color factor.
              colFac = min(nF,nGluonToQuark)*TR;
              // Global headRoomFactor is half as big as sector
              headRoomFac = getHeadRoomFactorSplit(nG,nQQ);
              if (!sectorShower) {
                headRoomFac *= 0.5;
                // Also fold in Pari factor in Global normalization
                double sN = sAnt;
                if ( !trialPtr->getIsSwapped(indx) ) {
                  if ( event[trialPtr->iR].colType() == -1 )
                    sN = m2(event[trialPtr->iOld2],event[trialPtr->iR]);
                } else {
                  if ( event[trialPtr->iL].colType() == 1 )
                    sN = m2(event[trialPtr->iOld1],event[trialPtr->iL]);
                }
                headRoomFac *= 2.*sN / (sAnt + sN);
              }
            }
            
            // Normalization factor (headRoom * phaseSpaceMassCorrection)
            double normFac = headRoomFac * psMassCorr;
            
            // Verbose output
            if (verbose >= 9) {
              cout<<"   Trial "<<indx<<" "<<trialGenName
              <<": Generating new qTrial starting from "<<qTrial
              <<" (in evolution window "<<iRegion<<" with qMin = "
              <<qMinNow<<"; nF = "<<nF<<")"<<endl;
            }
            
            // Sanity check for zero branching probability
            if (colFac < TINY || headRoomFac < TINY) {
              qTrial = 0.0;
              trialPtr->saveTrial(indx,qTrial,0.,0.,0.,0.);
            }
            
            // 1) AlphaS running inside trial integral
            // Only used for pT-evolution with muR propto pT
            // and for mQQ-evolution with muR propto mQQ
            else if (alphaSorder >= 1 && evolutionType == 1 && alphaSmode == 1) {
              
              // One-loop beta function (two-loop imposed by veto, below)
              double b0 = (33. - 2.*nF) / (12. * M_PI);
              
              // Effective renormalization-scale prefactor.
              double kR = alphaSkMu;
              double lambdaQCD = getLambda(nF);
              if (alphaScmw || alphaScmwDef) {
                lambdaQCD *= alphaSptr->facCMW(nF);
              }
              
              // Generate new q value, with alphaS running inside trial integral
              double q2trial = trialGenPtr->genQ2(pow2(qTrial),
                                                  sAnt,zMinNow,zMaxNow,colFac,
                                                  b0,kR,lambdaQCD,normFac);
              qTrial = sqrt(q2trial);
              
              // Save trial information
              double evNormNow = evNormFactor;
              if (iAntPhys == iQGsplit || iAntPhys == iGGsplit) evNormNow = 1.0;
              double muEff    = kR*qTrial/sqrt(evNormNow)/lambdaQCD;
              double alphaEff = 1./b0/log(pow2(muEff));
              trialPtr->saveTrial(indx,qTrial,zMinNow,zMaxNow,colFac,
                                  alphaEff,headRoomFac);
              
            }
            
            // 2) AlphaS outside trial integral
            else {
              
              // 0) constant alphaS
              double facAlphaS = alphaSvalue;
              
              // 1) running with muR ~ mDipole
              if (alphaSmode == 0 && alphaSorder >= 1) {
                double kR = alphaSkMu;
                if (alphaScmw || alphaScmwDef) kR /= alphaSptr->facCMW(nF);
                facAlphaS = alphaSptr->alphaS(pow2(kR)*sAnt);
              }
              
              // 2) running with muR not proportional to evolution variable
              else if (alphaSorder >= 1) {
                facAlphaS = alphaSmax;
                // Special case for mD evolution with muR proportional to pT
                if (evolutionType == 2 && alphaSmode == 1) {
                  // Lowest pT scale that can be reached in this mD window
                  double pT2min = pow(qMinNow,4.)/sAnt/pow2(evNormFactor);
                  double kR = alphaSkMu;
                  if (alphaScmw || alphaScmwDef) kR /= alphaSptr->facCMW(nF);
                  double mu2 = max(pow2(alphaSmuMin),pow2(kR)*pT2min);
                  facAlphaS = alphaSptr->alphaS(mu2);
                  facAlphaS = min(alphaSmax,facAlphaS);
                  // Special for 2-loop running below 1 GeV: extra headroom
                  if (alphaSorder >= 2 && mu2 < 1.) facAlphaS *= 1.1;
                }
              }
              
              // Generate new q value, with alphaS outside trial integral
              double q2trial = trialGenPtr->genQ2(pow2(qTrial),
                                                  sAnt,zMinNow,zMaxNow,colFac,
                                                  facAlphaS,normFac);
              qTrial = sqrt(q2trial);
              // Save trial information
              trialPtr->saveTrial(indx,qTrial,zMinNow,zMaxNow,colFac,
                                  facAlphaS,headRoomFac);
              
              // End: if alphaS outside trial integral
            }
            
            // Check evolution window boundaries
            if (qTrial > qMinNow) {
              acceptRegion = true;
            } else if (iRegion == 0) {
              acceptRegion = true;
              qTrial       = 0.0;
            } else {
              qTrial = qMinNow;
              iRegion--;
            }
            
            // End: impose evolution windows
          }
          
          // End: if has saved trial
        }
        
        if (qTrial > qTrialMax) {
          qTrialMax = qTrial;
          indxMax   = indx;
        }
        
        // End: loop over trial generators for this antenna
      }
      
      // Did I win?
      if (qTrialMax >= qWin || qWin <= 0.0) {
        winnerPtr = trialPtr;
        qWin      = qTrialMax;
        indxWin   = indxMax;
      }
      
      // Verbose output
      if (verbose >= 7) {
        printOut("VS::pTnext",": qTrialMax   = "+num2str(qTrialMax)
                 +(indxMax >= 0 ?
                   " ("+trialPtr->trialGenPtrSav[indxMax]->name()+")" :"")
                 +(iAnt+1 == int(branchelementals.size()) ?
                   "; final qWin = ":"; current qWin = ")
                 +num2str(qWin));
      }
      
    }
    // End of main loop over dipole-antennae
    //-------------------------------------------------------------------
    
    // A) If non-zero branching scale found: continue
    if (qWin > qEndAll) {
      
      // If unordered type, update current restart scale for unordering
      if (winnerPtr->unOrdering) Qrestart = qWin;
      
      // Verbose output
      if (verbose >= 5) {
        if (verbose >= 7)  list();
        printOut("VS::pTnext","=== Winner at scale qWin = "
                 +num2str(qWin)
                 +" trial type "+winnerPtr->trialGenPtrSav[indxWin]->name()
                 +"   col = "+num2str(winnerPtr->old1.col(),5));
      }
      
    }
    
    // B) Test for end-of-shower condition
    else {
      
      qWin = 0.0;
      
      // Verbose output
      if (verbose >= 5) {
        printOut("VS::pTnext","=== All trials now below cutoff "
                 "qEndAll = "+num2str(qEndAll,3)+".");
        if (verbose >= 6) {
          printOut("VS::pTnext","Final configuration was:");
          event.list();
        }
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
          // Add weiht correction to cumulative sum
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
    // End of pTnext: finalize and return to PYTHIA
    
    // Return nonvanishing value if branching found
    // Translate back to PYTHIA normalization --> factor evNormFactor
    double pTevolWin = qWin;
    if (normalizeInputPT) {
      // Vincia pT is quadratic -> take square root
      if (evolutionType == 1) {
        pTevolWin /= sqrt(evNormFactor);
      }
      // Vincia mD is linear -> no square root
      else if (evolutionType == 2) {
        pTevolWin /= evNormFactor;
      }
    }
    // If unordered branching won, return input scale - this should make
    // the overall evolution still proceed in an ordered way.
    return min(pTevolBegAll,max(0.0,pTevolWin));
    
  }
  
  //*********
  
  bool VinciaShower::branch(Event& event, bool isInterleaved) {
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::branch","begin --------------");
    }
    
    // The winning branchelemental and trial type are stored in winner
    // and indxWin respectively. Extract current QE and sAnt scales
    double qNew   = winnerPtr->scaleSav[indxWin];
    double Q2new  = pow2(qNew);
    double sAnt   = winnerPtr->s();
    iSysSel = winnerPtr->system;
    
    // Consistency check 1
    if ( sAnt < TINY || Q2new < TINY) {
      if (verbose >= 3) {
        printOut("VS::branch",
                 "Warning: s ="+num2str(sAnt)+"  Q2 = "+num2str(Q2new));
      }
      winnerPtr->nHadr++;
    }
    
    // Verbose output
    if (verbose >= 6) {
      printOut("VS::branch","* Processing Branching at scale Q = "
               +num2str(qNew)+"   isInterleaved = "+bool2str(isInterleaved));
    }
    
    //------------------------------------------------------------------------
    // GENERATE FULL TRIAL KINEMATICS and check whether it is accepted
    // v.1024: delegated this step to a separate function, acceptTrial()
    //         (eg, with the intention of making "pseudoshowers" possible)
    if (not acceptTrial(event)) return false;
    
    //------------------------------------------------------------------------
    
    // Flag to tell whether a general update of antenna particles is necessary
    bool updateMothers;
    
    // Put new particles into the event record.
    int iOld1 = winnerPtr->iOld1;
    int iOld2 = winnerPtr->iOld2;
    int iNew1 = event.append(winnerPtr->new1);
    int iNew2 = event.append(winnerPtr->new2);
    int iNew3 = event.append(winnerPtr->new3);
    
    // Update event pointers if necessary
    event.restorePtrs();
    
    // Check if we went from polarized to unpolarized state
    // If so, depolarize parton state
    // (A more complete alternative here would be to create depolarized
    // copies of all partons and then update everything, but deemed
    // unnecessary for now.)
    if (event[iOld1].pol() != 9 && event[iOld2].pol() != 9 &&
        (event[iNew1].pol() == 9 || event[iNew2].pol() == 9 ||
         event[iNew3].pol() == 9)) {
          if (verbose >= 5)
            printOut("VS::branch","Depolarizing parton state");
          // Depolarize parton state (except the branching mothers,
          // which will be replaced by unpolarized daughters)
          int sizeSystem = partonSystemsPtr->sizeOut(iSysSel);
          for (int i = 0; i < sizeSystem; ++i) {
            int i1 = partonSystemsPtr->getOut( iSysSel, i);
            // Skip if not present in final state
            if ( i1 <= 0 || !event[i1].isFinal()) continue;
            // Skip if mother parton to be replaced by daughter
            if ( i1 == winnerPtr->iOld1 || i1 == winnerPtr->iOld2) continue;
            // Else depolarize
            if ( event[i1].pol() != 9 ) event[i1].pol(9);
            // Tell that antenna mothers should be updated in update loop below
            updateMothers = true;
          }
          // Also make sure all daughters are depolarized as well
          winnerPtr->new1.pol(9);
          winnerPtr->new2.pol(9);
          winnerPtr->new3.pol(9);
        }
    
    // Mark original dipole partons as branched and set daughters/mothers.
    event[iOld1].statusNeg();
    event[iOld1].daughters(iNew1, iNew3);
    event[iOld2].statusNeg();
    event[iOld2].daughters(iNew1, iNew3);
    
    // Finally update the list of systems.
    partonSystemsPtr->replace(iSysSel, iOld1, iNew1);
    partonSystemsPtr->addOut(iSysSel, iNew2);
    partonSystemsPtr->replace(iSysSel, iOld2, iNew3);
    
    // Update number of gluons and quark pairs
    if (winnerPtr->new2.id() == 21) ++nG;
    else {
      nG -= 1;
      ++nQQ;
    }
    
    // Store old neighbours
    int iL  = winnerPtr->iL;
    int iLL = winnerPtr->iLL;
    int iR  = winnerPtr->iR;
    int iRR = winnerPtr->iRR;
    
    // Update old and add new dipoles
    if (verbose >= 5) {
      printOut("VS::branch",
               "=== Branching Accepted. Updating dipole-antenna(e)");
    }
    if (winnerPtr->new1.colType() > 0) {
      // Normal case, where iNew1-iNew2 has an LC color connection
      winnerPtr->reset(event,iNew1,iNew2);
      winnerPtr->old1 = event[iNew1];
      winnerPtr->old2 = event[iNew2];
      if (orderingMode == 0 || orderingMode >= 2)
        winnerPtr->unOrdering = true;
      
      // Update trial generators
      resetTrialGenerators(winnerPtr);
      
      // Update neighbours
      winnerPtr->iL=iL;
      winnerPtr->iLL=iLL;
      winnerPtr->iR=iNew3;
      winnerPtr->iRR=iR;
      
      // If second parton is octet, insert new antenna (iNew2,iNew3)
      if (event[iNew2].colType() > 0) {
        
        // Define new dipole-antenna
        Branchelemental trial(event,iNew2,iNew3);
        trial.system = iSysSel;
        trial.old1   = event[iNew2];
        trial.old2   = event[iNew3];
        if (orderingMode == 0 || orderingMode >= 2) trial.unOrdering = true;
        
        // Set trial generators
        resetTrialGenerators(&trial);
        
        // Set tri- and quadrupole neighbors
        trial.iL     = iNew1;
        trial.iLL    = iL;
        trial.iR     = iR;
        trial.iRR    = iRR;
        
        // Keep family name for matching
        trial.iAncestor=winnerPtr->iAncestor;
        
        // Add dipole-antenna to list, insert after iNew1-Inew2 to keep order
        for (int iAnt=0; iAnt<int(branchelementals.size()); iAnt++) {
          if (&branchelementals[iAnt] == winnerPtr) {
            branchelementals.insert(branchelementals.begin()+iAnt+1,trial);
            break;
          }
        }
        
      }
      
    } else {
      
      // Special case for qbar q-x final state, where only second pair is dipole
      winnerPtr->reset(event,iNew2,iNew3);
      winnerPtr->old1        = event[iNew2];
      winnerPtr->old2        = event[iNew3];
      if (orderingMode == 0 || orderingMode >= 2)
        winnerPtr->unOrdering = true;
      
      // Update trial generators
      resetTrialGenerators(winnerPtr);
      
      // Update neighbors
      winnerPtr->iL  = iNew1;
      winnerPtr->iLL = iL;
      winnerPtr->iR  = iR;
      winnerPtr->iRR = iRR;
    }
    
    // Update neighbor pointers in other dipoles and unordering scales
    Qrestart     = mSystem;
    Q2UnOrdOld   = pow2(mSystem);
    mD2UnOrdOld  = pow2(mSystem);
    for (int iAnt = 0; iAnt < int(branchelementals.size()); ++iAnt) {
      
      // Shorthand
      Branchelemental* antPtr = &branchelementals[iAnt];
      
      // Update mother partons in case a depolarization took place
      if (updateMothers) {
        antPtr->old1 = event[antPtr->iOld1];
        antPtr->old2 = event[antPtr->iOld2];
      }
      
      // If unordering, restart everyone from CM
      if (orderingMode >= 2 || orderingMode == 0) antPtr->unOrdering = true;
      
      // Global showers with smooth ordering:
      if (!sectorShower && orderingMode >= 2) {
        // Update current Q scale for unordering
        // (antennae already updated, so can use iOld values)
        int i1 = antPtr->iOld1;
        int i2 = antPtr->iOld2;
        // For antennae ending on an antiquark, check if there is an iR (quark)
        // -> compute g->qq splitting measure (invariant mass)
        if (event[i2].colType() == -1 && antPtr->iR != 0) {
          Q2UnOrdOld = min(Q2UnOrdOld,m2(event[i2],event[antPtr->iR]));
          mD2UnOrdOld = min(mD2UnOrdOld,m2(event[i2],event[antPtr->iR]));
        }
        // Else for antennae ending on a gluon, compute emission measure
        else if (event[i2].colType() == 2 && antPtr->iR != 0) {
          Q2UnOrdOld = min(Q2UnOrdOld, resolutionPtr->Q2E(event[i1],event[i2],
                                                          event[antPtr->iR]));
          mD2UnOrdOld =
          min(mD2UnOrdOld, min(m2(event[i1],event[i2]),
                               m2(event[i2],event[antPtr->iR])));
        }
      }
      
      // The post-branching branchelementals
      if (antPtr->iOld1 == iNew1 || antPtr->iOld1 == iNew2) {
        // No update necessary for this pair (handled above),
        continue;
      }
      
      // Next-to-next-to-nearest neighbour update
      if (antPtr->iLL == iOld2) branchelementals[iAnt].iLL = iNew3;
      if (antPtr->iRR == iOld1) branchelementals[iAnt].iRR = iNew1;
      
      // Next-to-nearest neighbour update
      if (antPtr->iL  == iOld2) {
        branchelementals[iAnt].iL    = iNew3;
        branchelementals[iAnt].iLL   = iNew2;
      }
      if (antPtr->iR == iOld1) {
        branchelementals[iAnt].iR  = iNew1;
        branchelementals[iAnt].iRR = iNew2;
      }
      
      // Nearest neighbour update
      if (antPtr->iOld1 == iOld2) {
        branchelementals[iAnt].renewTrial();
        branchelementals[iAnt].iOld1       = iNew3;
        branchelementals[iAnt].old1        = event[iNew3];
        // Update neighbors
        branchelementals[iAnt].iL          = iNew2;
        branchelementals[iAnt].iLL         = iNew1;
        // Update trial generators
        resetTrialGenerators(&branchelementals[iAnt]);
      }
      if (antPtr->iOld2 == iOld1) {
        branchelementals[iAnt].renewTrial();
        branchelementals[iAnt].iOld2       = iNew1;
        branchelementals[iAnt].old2        = event[iNew1];
        // Recompute stored invariant mass
        branchelementals[iAnt].mCalc();
        branchelementals[iAnt].iR          = iNew2;
        branchelementals[iAnt].iRR         = iNew3;
        // Update trial generators
        resetTrialGenerators(&branchelementals[iAnt]);
      }
      
      // Also force to recompute stored invariant masses (eg if a recoil
      // changed a parent momentum)
      branchelementals[iAnt].mCalc();
      
    }
    
    if (verbose >= 6) {
      event.list();
      list();
    }
    
    // Done.
    return true;
    
  }
  
  //*********
  
  // Update list of branchelementals after ISR radiation
  
  void VinciaShower::update( int iSys, Event& event) {
    if (verbose >= 1)
      printErr("VS::update","system "+num2str(iSys)
               +" (function not implemented)");
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
      printOut("VS::initVinciaHistos","begin --------------");
    }
    
    vinciaHistos.clear();
    
    // Don't book diagnostics histograms if verbose output not switched on.
    if (verbose <= 1) return;
    
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
    
    // Histos for pTevol scales
    nBin   = 100;
    binWid = 0.1;
    wid    = nBin * binWid;
    xMax   = 0.;
    xMin   = xMax - wid;
    Hist HqLn("Ln(q2/sSystem) distribution (USR)",nBin,xMin,xMax);
    for (int nGtmp = 0; nGtmp <= 9; ++nGtmp) {
      for (int nQQtmp = 0; nQQtmp <= 5; ++nQQtmp) {
        string state = num2str(2*nQQtmp,1)+"q"+num2str(nGtmp,1)+"g";
        string hisTitle="Ln(q2trial/sSystem):" + state;
        vinciaHistos[hisTitle]=HqLn;
        hisTitle="Ln(q2/sSystem):" + state;
        vinciaHistos[hisTitle]=HqLn;
      }
    }
    
  }
  
  //*********
  
  // Add trial functions to a Branchelemental
  void VinciaShower::resetTrialGenerators(Branchelemental* trial) {
    
    // Reset
    trial->clearTrialGenerators();
    
    // Parent ID
    int id1      = trial->old1.id();
    int id2      = trial->old2.id();
    int colType1 = trial->old1.colType();
    int colType2 = trial->old2.colType();
    // (Treat color sextets as triplets here)
    if (abs(colType1) >= 3) colType1 = colType1 % 2;
    if (abs(colType2) >= 3) colType2 = colType2 % 2;
    
    // Gluon emission: soft-eikonal trial terms
    int iAntPhys = -1;
    bool isOn = false;
    // QQbar -> QGQbar
    if ( colType1 == 1 && colType2 == -1 ) {
      iAntPhys = iQQemit;
      isOn     = settingsPtr->flag("Vincia:QQemit");
      if (isOn) trial->addTrialGenerator(iAntPhys, false, &trialSoft);
    }
    // QG -> QGG
    else if ( colType1 == 1 && colType2 == 2 ) {
      iAntPhys = iQGemit;
      isOn     = settingsPtr->flag("Vincia:QGemit");
      if (isOn) {
        trial->addTrialGenerator(iAntPhys, false, &trialSoft);
        if (sectorShower)
          trial->addTrialGenerator(iAntPhys, false, &trialCollK);
      }
    }
    // GQbar -> GGQbar (physical antenna is obtained by swap of QG -> QGG)
    else if ( colType1 == 2 && colType2 == -1 ) {
      iAntPhys = iQGemit;
      isOn     = settingsPtr->flag("Vincia:QGemit");
      if (isOn) {
        trial->addTrialGenerator(iAntPhys, true, &trialSoft);
        if (sectorShower)
          trial->addTrialGenerator(iAntPhys, true, &trialCollI);
      }
    }
    // GG -> GGG
    else if (colType1 == 2 && colType2 == 2) {
      iAntPhys = iGGemit;
      isOn     = settingsPtr->flag("Vincia:GGemit");
      if (isOn) {
        trial->addTrialGenerator(iAntPhys, false, &trialSoft);
        if (sectorShower) {
          trial->addTrialGenerator(iAntPhys, false, &trialCollI);
          trial->addTrialGenerator(iAntPhys, false, &trialCollK);
        }
      }
    }
    
    // XG -> XQbarQ
    if (id2 == 21) {
      if (colType1 == 2) {
        iAntPhys = iGGsplit;
        isOn = nGluonToQuark >= 1 && settingsPtr->flag("Vincia:GGsplit");
      } else {
        iAntPhys = iQGsplit;
        isOn = nGluonToQuark >= 1 && settingsPtr->flag("Vincia:QGsplit");
      }
      if (isOn) trial->addTrialGenerator(iAntPhys, false, &trialSplitK);
    }
    // GX -> QbarQX (physical antenna obtained from swap of XG -> XQbarQ)
    if (id1 == 21) {
      if (colType2 == 2) {
        iAntPhys = iGGsplit;
        isOn = nGluonToQuark >= 1 && settingsPtr->flag("Vincia:GGsplit");
      } else {
        iAntPhys = iQGsplit;
        isOn = nGluonToQuark >= 1 && settingsPtr->flag("Vincia:QGsplit");
      }
      if (isOn) trial->addTrialGenerator(iAntPhys, true, &trialSplitI);
    }
  }
  
  // ******
  //
  // acceptTrial : main function to accept or reject a trial branching
  //               assumes branching scale trialPtr->qNew already generated
  // Steps:
  //     *) Generate second branching invariant, given qNew
  //        Construct new partons, compute basic (LL) accept probability,
  //        and construct kinematics in LAB frame
  //     *) Optional: check sector ordering (if doing sector showers)
  //     *) Optional: check alternative hadronization vetos
  //     *) Optional: apply beyond-LL (beyond 2->3) improvements
  //     *) Set mother and color-flow information
  //     *) Optional: match to fixed-order QCD matrix elements (from MadGraph)
  //     *) Optional: post-processing of uncertainty variations
  //     *) Accept or Reject Trial Emission
  //
  //        Return true if trial accepted, false otherwise
  
  bool VinciaShower::acceptTrial(Event& event) {
    
    //************************************************************************
    // Basic info about trial function and scale
    //************************************************************************
    
    int iTrial     = winnerPtr->getTrialIndex();
    int iAntPhys   = winnerPtr->getPhysIndex(iTrial);
    bool isSwapped = winnerPtr->getIsSwapped(iTrial);
    
    double qNew  = winnerPtr->getTrialScale(iTrial);
    double mAnt  = winnerPtr->m();
    double sAnt  = winnerPtr->s();
    
    //$ Mark this trial as "used", will need to generate a new one.
    winnerPtr->renewTrial(iTrial);
    
    //$1 Count up total number of trials
    ++nTrialsSum;
    
    //************************************************************************
    //$ Generate zeta variable, then work out y12, y23
    //************************************************************************
    
    double y12, y23;
    bool pass = winnerPtr->genTrialInvariants(y12, y23, iTrial);   //$
    //$1 [trial invariants are not in the good region of P.S.]    
    if (! pass) {   //$
      if (verbose >= 6) printOut("VS::acceptTrial","failed "
                                 "genTrialInvariants, returning.");
      //$1 Count up local and global failure rate due to hull
      winnerPtr->nHull++;
      ++nTrialsHull;
      // Return failure
      return false;
    }
    
    //************************************************************************
    //$ Compute spin-summed trial antenna sum for these variables
    //************************************************************************
    
    // Sum over individual trial terms
    double antTrialSum    = 0.;
    for (int iSum = 0; iSum < int(winnerPtr->nTrialGenerators()); ++iSum) {  //$
      // Only include terms that correspond to the current physical antenna
      if (winnerPtr->getPhysIndex(iSum) != iAntPhys ||
          winnerPtr->getIsSwapped(iTrial) != isSwapped ) continue;
      // Evaluate trial term
      double antTrial(0.);
      antTrial += winnerPtr->trialGenPtrSav[iSum]->aTrial(y12,y23);
      // HeadRoom and Color factors
      double headRoom = winnerPtr->getHeadRoomFactor(iSum);
      double colFac   = winnerPtr->getColFac(iSum);
      // (color factor for splitings is just nF, which is not included in
      // physical antenna normalization nor in matrix elements)
      if (iAntPhys == iQGsplit || iAntPhys == iGGsplit) colFac = 1.;
      antTrial *= colFac * headRoom;
      antTrialSum += antTrial;
    }
    
    //************************************************************************
    //$ Set flavors to the particles in the winner branchelemental
    //************************************************************************
    
    // Gluon emission: inherit parent flavors and add middle gluon
    if (iAntPhys == iQQemit || iAntPhys == iQGemit || iAntPhys == iGGemit) {
      winnerPtr->new1.id(winnerPtr->old1.id());
      winnerPtr->new2.id(21);
      winnerPtr->new3.id(winnerPtr->old2.id());
    }
    
    // Gluon splitting:
    else if ( iAntPhys == iQGsplit || iAntPhys == iGGsplit) {
      
      // Set flavor of splitting
      double nF = min(nGluonToQuark, getNf(qNew));
      int splitFlavor = int(rndmPtr->flat() * nF) + 1;
      
      // QG->QQbar'Q' and GG->GQbarQ
      if ( !isSwapped ) {
        winnerPtr->new1.id(winnerPtr->old1.id());
        winnerPtr->new2.id(-splitFlavor);
        winnerPtr->new3.id( splitFlavor);
      }
      // GQbar->Qbar'Q'Qbar and GG->QbarQG
      else {
        winnerPtr->new1.id(-splitFlavor);
        winnerPtr->new2.id( splitFlavor);
        winnerPtr->new3.id(winnerPtr->old2.id());
      }
      
    }
    
    //************************************************************************
    //$ Treatment of Masses
    //************************************************************************
    
    // Masses
    double mu1(0.), mu2(0.), mu3(0.);
    if ( vinComPtr->isMassive(winnerPtr->new1.id()) )
      mu1 = winnerPtr->new1.m0()/mAnt;
    if ( vinComPtr->isMassive(winnerPtr->new2.id()) )
      mu2 = winnerPtr->new2.m0()/mAnt;
    if ( vinComPtr->isMassive(winnerPtr->new3.id()) )
      mu3 = winnerPtr->new3.m0()/mAnt;
    
    // For gluon splitting to quarks, translate from sab = (pa+pb)^2 (the
    // evolution variable) to sab~ = (pa+pb)^2-ma^2-mb^2 definitions
    if ( (iAntPhys == iQGsplit || iAntPhys == iGGsplit)
        && (mu1 > 0.0 || mu2 > 0.0 || mu3 > 0.0) ) {
      y12 += - pow2(mu1) - pow2(mu2);
      y23 += - pow2(mu2) - pow2(mu3);
    }
    
    if ( mu1 > 0. || mu2 > 0. || mu3 > 0.)
      isMassless = false;
    else
      isMassless = true;
    
    //$ Check massive phase-space boundaries
    if ( !isMassless ) {
      double y13 = 1. - y12 - y23 - pow2(mu1) - pow2(mu2) - pow2(mu3);
      double gDetDimless = gDet(y12, y23, y13, mu1, mu2, mu3);
      if (gDetDimless <= 0.) {
        winnerPtr->nHull++;
        ++nTrialsHullMassive;
        return false;
      }
    }
    
    //************************************************************************
    // Check hadronization veto(s) 1
    // Note: here only check branching invariants. Invariants with respect
    // to neighbors can optionally be checked after the branching kinematics
    // have been fully defined, below.
    //************************************************************************
    
    // Require all color-connected invariants at least above lowest
    // physical meson mass
    // (consistent with length > thickness assumption of string model)
    
    //$1 <parallel> branching invariant 12
    if ( winnerPtr->new1.col() == winnerPtr->new2.acol() ) {
      int id1 = abs(winnerPtr->new1.id());
      if (id1 == 21 || id1 <= 2) id1 = 1;
      int id2 = abs(winnerPtr->new2.id());
      if (id2 == 21 || id1 <= 2) id2 = 1;
      int idMes = max(id1,id2)*100 + min(id1,id2)*10 + 1;
      // Special for ssbar, allow to form eta rather than eta'.
      if (idMes == 331) idMes = 221;
      double m2mes = pow2(particleDataPtr->m0(idMes));
      // Translate from dot product to invariant mass
      double m2    = (y12 + pow2(mu1) + pow2(mu2)) * sAnt;
      // Verbose test
      if (verbose >= 9) {
        cout<<" Had Test (12): m2("<<winnerPtr->new1.id()<<","
        <<winnerPtr->new2.id()<<") = "<<m2
        <<" vs m2("<<idMes<<") = "<<m2mes<<endl;
      }
      //$2 Hadronization test
      if (m2 < 1.01*m2mes) {
        if (verbose >= 5) printOut("VS::acceptTrial",
                                   "=== Branching Vetoed. m2 < 1.01*m2mes.");
        winnerPtr->nHadr++;
        ++nTrialsCutM;
        return false;
      }
    }
    
    //$1 <parallel> branching invariant 23
    if ( winnerPtr->new2.col() == winnerPtr->new3.acol() ) {
      int id1 = abs(winnerPtr->new2.id());
      if (id1 == 21) id1 = 1;
      int id2 = abs(winnerPtr->new3.id());
      if (id2 == 21) id2 = 1;
      int idMes = max(id1,id2)*100 + min(id1,id2)*10 + 1;
      double m2mes = pow2(particleDataPtr->m0(idMes));
      // Translate from dot product to invariant mass
      double m2    = (y23 + pow2(mu2) + pow2(mu3)) * sAnt;
      // Verbose test
      if (verbose >= 9) {
        cout<<" Had Test (23): m2("<<winnerPtr->new2.id()<<","
        <<winnerPtr->new3.id()<<") = "<<m2
        <<" vs m2("<<idMes<<") = "<<m2mes<<endl;
      }
      //$2 Hadronization test
      if (m2 < 1.01*m2mes) {
        if (verbose >= 5) printOut("VS::acceptTrial",
                                   "=== Branching Vetoed. m2 < 1.01*m2mes.");
        winnerPtr->nHadr++;
        ++nTrialsCutM;
        return false;
      }
    }
    
    //$1 <parallel> Check hadronization cutoff: Ariadne pT or alternatives
    double q2forHad = y12 * y23 * sAnt;
    // Alternative 1: Cutoff in Dipole dot product
    if (cutoffType == 2) q2forHad = min(y12,y23) * sAnt;
    // Alternative 2: Cutoff in Pythia pTevol
    else if (cutoffType == -1)
      q2forHad = resolutionPtr->Q2E(winnerPtr->new1, winnerPtr->new2,
                                    winnerPtr->new3,-1);
    //$2 Check cutoffScale
    if (q2forHad < pow2(cutoffScale)) {
      winnerPtr->nHadr++;
      ++nTrialsCutQ;
      return false;
    }
    
    //************************************************************************
    //$ Construct flag for helicity of mother partons
    //************************************************************************
    
    double hA = winnerPtr->old1.pol();
    double hB = winnerPtr->old2.pol();
    bool isPolarized = usePolarizationNow && (hA != 9 && hB != 9);
    
    // Only use polarized antennae for massless particles
    isPolarized = isPolarized && isMassless;
    
    //************************************************************************
    //$ If physical antenna function is mirror of current trial, translate to
    //$ swapped invariants for antenna-function evaluation
    //************************************************************************
    
    double hAant  = hA;
    double hBant  = hB;
    double y12ant = y12;
    double y23ant = y23;
    double mu1ant = mu1;
    double mu2ant = mu2;
    double mu3ant = mu3;
    if ( isSwapped ) {
      hAant  = hB;
      hBant  = hA;
      y12ant = y23;
      y23ant = y12;
      mu1ant = mu3;
      mu3ant = mu1;
    }
    
    //************************************************************************
    //$ <keepFormat>
    //$ Compute spin-summed physical antennae (spin selection below)
    //$ Store result in vector representing uncertainty variations
    //$ iVar = 0 : User settings
    //$        1 : Vincia defaults
    //$        2 : AlphaS-Hi (with def ants)
    //$        3 : AlphaS-Lo (with def ants)
    //$        4 : Ant-Hi    (with def alphaS)
    //$        5 : Ant-Lo    (with def alphaS)
    //$        6 : NLO-Hi    (with def ants & alphaS)
    //$        7 : NLO-Lo    (with def ants & alphaS)
    //$        8 : Ord-Var1  -"-
    //$        9 : Ord-Var2  -"-
    //$       10 : NLC-Hi    -"-
    //$       11 : NLC-Lo    -"-
    //$ Global shower: Pimp * Pari * ant-global
    //$ Global matched: Pimp * Pari * PME * ant-global
    //$ Sector shower: ant-sector
    //$ Sector matched: ME(n+1)/ME(n)
    //************************************************************************
    
    // Define pointer to Global or Sector antennae, depending on shower setting
    Antenna* antUsrPtr = getAnt(iAntPhys,sectorShower,0); //$
    Antenna* antDefPtr = getAnt(iAntPhys,sectorShower,1);
    Antenna* antMaxPtr = getAnt(iAntPhys,sectorShower,2);
    Antenna* antMinPtr = getAnt(iAntPhys,sectorShower,3);
    
    // Compute spin-summed antenna function
    if (verbose >= 8) printOut("VS::acceptTrial","Evaluating antenna function");
    
    vector<double> helSum;
    helSum.resize(nVariations());
    //$1 [Unpolarized case: ignore parent helicities]
    if (!isPolarized) { 
      helSum[0] = antUsrPtr->antennaFunction(y12ant,y23ant,mu1ant,mu2ant,mu3ant,
                                             9,9,9,9,9); //$
      if (uncertaintyBands) {
        helSum[1] = antDefPtr->antennaFunction(y12ant,y23ant,mu1ant,mu2ant,
                                               mu3ant,9,9,9,9,9); //$
        // Use DEF antennae as basis for uncertainty variations
        for (int iVar=2; iVar<nVariations(); ++iVar)
          helSum[iVar] = helSum[1];
        helSum[4] = antMaxPtr->antennaFunction(y12ant,y23ant,mu1ant,mu2ant,
                                               mu3ant,9,9,9,9,9); //$
        helSum[5] = antMinPtr->antennaFunction(y12ant,y23ant,mu1ant,mu2ant,
                                               mu3ant,9,9,9,9,9); //$
      }
    }
    //$1 [Polarized case: use parent helicities (but restrict to massless)]
    else {
      helSum[0] = antUsrPtr->antennaFunction(y12ant, y23ant, 0., 0., 0.,
                                             hAant, hBant, 9, 9, 9);
      if (uncertaintyBands) {
        helSum[1] = antDefPtr->antennaFunction(y12ant, y23ant, 0., 0., 0.,
                                               hAant, hBant, 9, 9, 9);
        // Use DEF antennae as basis for uncertainty variations
        for (int iVar=2; iVar<nVariations(); ++iVar)
          helSum[iVar] = helSum[1];
        helSum[4] = antMaxPtr->antennaFunction(y12ant, y23ant,0., 0., 0.,
                                               hAant, hBant, 9, 9, 9);
        helSum[5] = antMinPtr->antennaFunction(y12ant, y23ant, 0., 0., 0.,
                                               hAant, hBant, 9, 9, 9);
      }
    }
    
    // Verbose output
    if (verbose >= 8)
      printOut("VS::acceptTrial"," helSum  = "+num2str(helSum[0]/sAnt));
    
    //$1 Apply color (charge) factor
    vector<double> antPhys;
    for (int iVar=0; iVar<nVariations(); ++iVar) {
      double chargeFactor   = antDefPtr->getChargeFactor();
      if (iVar == 0) chargeFactor = antUsrPtr->getChargeFactor();
      else if (iVar == 4) chargeFactor = antMaxPtr->getChargeFactor();
      else if (iVar == 5) chargeFactor = antMinPtr->getChargeFactor();
      else if (iVar == 10 && winnerPtr->new2.id() == 21) chargeFactor = CA;
      else if (iVar == 11 && winnerPtr->new2.id() == 21) chargeFactor = CF;
      antPhys.push_back(chargeFactor * helSum[iVar]);
    }
    
    //***********************************************************************
    //$1 Compute Pimp and Pari modifications (only applied to global showers)
    //***********************************************************************
    
    //$2 <parallel>    Pimp (allows suppressed unordered branchings)
    double Pimp = 1.0;
    if (!sectorShower && orderingMode >= 2 && winnerPtr->unOrdering) {
      double q2new  = pow2(qNew);
      Pimp = 1.0/(1.0 + q2new/Q2UnOrdOld);
      for (int iVar = 0; iVar < nVariations(); ++iVar) {
        // Stronger ordering
        if (iVar == 8) {
          double PimpStronger = 1.0/(1.0 + pow2(q2new)/pow2(Q2UnOrdOld));
          antPhys[iVar] *= PimpStronger;
        }
        // mD-ordering (with unit normalization and using mqq for g->qq)
        else if (iVar == 9) {
          int id2 = winnerPtr->new2.id();
          double mD2new = (id2 == 21 || id2 == 22) ? sAnt*min(y12,y23) : q2new;
          double PimpMD =  1.0/(1.0 + mD2new/mD2UnOrdOld);
          antPhys[iVar] *= PimpMD;
        }
        else {
          antPhys[iVar] *= Pimp;
        }
      }
    }
    
    //$2 <parallel>    Pari  (ARIADNE factor for g->qq splittings)
    //$    Only for global showers, when neighbor is a final-state quark
    double Pari = 1.0;
    double sP = sAnt;
    if (!sectorShower && (iAntPhys == iQGsplit || iAntPhys == iGGsplit)
        && settingsPtr->flag("Vincia:GluonSplittingCorrection") ) {
      
      // Find initial particles in dipole branching.
      int iOld1 = winnerPtr->iOld1;
      int iOld2 = winnerPtr->iOld2;
      iSysSel = winnerPtr->system;
      
      // Impose ARIADNE factor for g->qqbar.
      //       Pari = 2/(1+sN/sP)    (sN = s(neighbor), sP = s(parent))
      // If s(neighbor) << s(parent) -> splitting suppressed (screened?)
      // If s(neighbor) >> s(parent) -> splitting enhanced (up to factor 2)
      
      double sN;
      int iN;
      // Which side of dipole-antenna is splitting?
      if (event[iOld1].id() != 21 || winnerPtr->new1.id() == 21) {
        iN = winnerPtr->iR;
        // Only apply if neighbor exists, is a quark, and is in final state
        if (iN != 0 && event[iN].id() != 21 && event[iN].isFinal() ) {
          sN = m2(event[iOld2],event[iN]);
          Pari = 2.0*sN/(sP + sN);
        }
      } else {
        iN = winnerPtr->iL;
        // Only apply if neighbor exists, is a quark, and is in final state
        if (iN != 0 && event[iN].id() != 21 && event[iN].isFinal() ) {
          sN = m2(event[iN],event[iOld1]);
          Pari = 2.0*sN/(sP + sN);
        }
      }
      // Check for negative Pari factor (normally should not happen!)
      if (Pari < 0.0) {
        if (verbose >= 1)
          printErr("VS::acceptTrial",
                   "Pari factor = "+num2str(Pari)+" < 0");
        Pari = 0.0;
      }
      // Apply Pari factor
      for (int iVar = 0; iVar < nVariations(); ++iVar)
        antPhys[iVar] *= Pari;
    }
    
    if (verbose >= 8)
      printOut("VS::acceptTrial","Pimp = "+num2str(Pimp)+
               " * Pari = "+num2str(Pari)+" -> antPhys = "+num2str(antPhys[0]));
    
    //************************************************************************
    //$ HYPERJET: special mode for processing events from HEJ
    //$ => matching to soft-resummed matrix elements
    //$ Ignore otherwise
    //************************************************************************
    
    if (hyperjet && winnerPtr->new2.id() == 21 && winnerPtr->system == 0) {
      // If running in hyper-jet mode, subtract soft Eikonal for gluon emission
      // for emissions off the hard interaction
      if (verbose >= 8)
        printOut("VS::acceptTrial","HyperJet mode: Subtracting Eikonal");
      antPhys[0] -= 2*(1.-y12-y23)/(y12*y23);
    }
    
    //***********************************************************************
    //$ Effiency step: Pure shower accept/reject, before doing full kinematics
    //$ <note> (only if no uncertainty bands and only for global showers;
    //$  for sector showers, the full kinematics are needed to know if the branching
    //$  will be vetoed, thus the accept probability can actually be crazy at
    //$  this point if we are in the wrong sector. For uncertainty bands, the
    //$  correct reweighting of the uncertainty variations has not yet been
    //$  worked out in the presence of an intermediate accept/reject here.)
    //$ <note>: matching corrections still to be applied, done below
    //***********************************************************************
    
    // Compute basic accept probability : Physical/Trial
    for (int iVar = 0; iVar < nVariations(); ++iVar)
      Paccept[iVar] = antPhys[iVar]/antTrialSum;
    
    // Verbose output
    if (verbose >= 8) {
      cout<<"   AntTrial/s = "<<setprecision(6)<<antTrialSum/sAnt<<endl;
      cout<<"   AntPhys/s  = "<<setprecision(6)<<antPhys[0]/sAnt
      <<"      ( Pimp = "<<Pimp<<" ; Pari = "<<Pari<<" )"<<endl;
      cout<<"   Paccept    = "<<setprecision(6)<<antPhys[0]/antTrialSum<<endl;
    }
    
    // If we are already beyond matched orders, do pure shower accept/reject
    // (else wait until matching corrections have been included)
    // (so far only works when uncertaintybands are switched off)
    if (nG + 2*nQQ - 2 > matchingLO && !uncertaintyBands && !sectorShower) {
      if ((Paccept[0] > 1.1 ) && verbose >= 1) {
        cout <<setprecision(3)
        <<" Paccept(shower) = "<<Paccept[0]<<"  at  q = "<<qNew
        <<"  mAnt = "<<sqrt(sAnt)<<"  Pimp = "<<Pimp << "  Pari = "<<Pari
        <<"  id(ijk) = "<<winnerPtr->new1.id()<<" "<<winnerPtr->new2.id()
        <<" "<<winnerPtr->new3.id()<<"  y(ij,jk) = "<<y12<<" "<<y23<<endl;
      }
      if (rndmPtr->flat() > Paccept[0]) {
        // TODO: do Sudakov reweighting for uncertainty bands
        winnerPtr->nVeto++;
        ++nTrialsVeto;
        return false;
      }
      // Reset accept probability to unity (Paccept[0] -> 1)
      for (int iVar = 0; iVar < nVariations(); ++iVar) {
        Paccept[iVar] /= Paccept[0];
      }
    }
    
    //***********************************************************************
    //$ Generate full kinematics for this trial branching
    //***********************************************************************
    
    //$1 Generate random (uniform) phi angle
    double phi = 2 * M_PI * rndmPtr->flat();
    
    //$1 Generate branching kinematics, starting from dipole-antenna parents
    vector<Vec4> pOld, pNew;
    pOld.push_back(winnerPtr->old1.p());
    pOld.push_back(winnerPtr->old2.p());
    int kineMapType  = antUsrPtr->getKineMapType(); //$
    vinCluPtr->map2to3(pNew, pOld, kineMapType, y12, y23, phi, mu1, mu2, mu3); //$
    
    // Save momenta
    winnerPtr->new1.p(pNew[0]);
    winnerPtr->new2.p(pNew[1]);
    winnerPtr->new3.p(pNew[2]);
    
    //------------------------------------------------------------------------
    //$ *)Optional: check sector ordering (if doing sector showers)
    if (sectorShower) {
      if (!sectorAccept(event,winnerPtr)) {  //$
        if (verbose >= 5) {
          printOut("VS::acceptTrial",
                   "Branching Vetoed. Failed sector veto.");
        }
        winnerPtr->nHull++;
        return false;
      } else {
        if (verbose >= 7) {
          printOut("VS::acceptTrial",
                   "Trial passed sector veto. Continuing...");
        }
      }
    }
    
    //************************************************************************
    //$ Check hadronization veto(s) 2
    //$ <note> here only check neighbor invariants. Branching invariants were
    //$ already checked above.
    //$ <note> for neighbors, we are only checking invariant mass > pion mass.
    //$ Could in principle be made more elaborate by checking cutoffScale
    //$ as well, given sufficient reason to do so.
    //************************************************************************
    
    // Require all neighbor dot products at least above pion mass
    // (consistent with length > thickness assumption of string model)
    double m2min = sAnt;
    double m2pi  = pow2(particleDataPtr->m0(111));
    if (winnerPtr->iL != 0)
      m2min = m2(event[winnerPtr->iL],winnerPtr->new1);
    if (winnerPtr->iR != 0)
      m2min = min(m2min,m2(event[winnerPtr->iR],winnerPtr->new3));
    if (m2min < 1.05*m2pi) {
      if (verbose >= 5) printOut("VS::acceptTrial","=== Branching Vetoed."
                                 " m2neighbor < 1.05*m2pi.");
      winnerPtr->nHadr++;
      ++nTrialsCutM;
      return false;
    }
    
    //***********************************************************************
    //$ Choose helicities for daughter particles
    //$ (so far only for massless polarized shower)
    //***********************************************************************
    
    if ( isPolarized ) {
      
      // Sum over final-state helicities
      // (Slightly inefficient to first evaluate sum and then reevaluate
      // individual terms below, but perhaps acceptable for increased
      // clarity. Some optimization is attempted by starting with the
      // dominant (MHV) contributions below. Further optimization could
      // be achieved by implementing some form of caching of the individual
      // contributions.)
      
      // Generate random number
      double randHel = rndmPtr->flat() * helSum[0];
      
      // Select helicity
      int hi(0), hj(0), hk(0);
      double aHel(0.);
      for (hi = hAant; abs(hi) <= 1; hi -= 2*hAant) {
        for (hk = hBant; abs(hk) <= 1; hk -= 2*hBant) {
          for (hj = hAant; abs(hj) <= 1; hj -= 2*hAant) {
            aHel = antUsrPtr->antennaFunction(y12ant,y23ant,0.,0.,0.,
                                              hAant,hBant,hi,hj,hk);
            randHel -= aHel;
            if (verbose >= 8) {
              cout<<"VS: antPhys("<<int(hAant)<<int(hBant)<<">"<<hi<<hj<<hk
              <<") = "<<aHel/sAnt
              <<", m(ar,rb) = "<<sqrt(y12ant*sAnt)<<", "<<sqrt(y23ant*sAnt)
              <<", isSwapped = "<<bool2str(isSwapped)
              <<"; sum = "<<helSum[0]/sAnt<<endl;
            }
            if (randHel < 0.) break;
          }
          if (randHel < 0.) break;
        }
        if (randHel < 0.) break;
      }
      
      // Uncertainty variations (would have picked these helicities with
      // a different probability if using different antennae).
      if (uncertaintyBands) {
        double PhelUsr = aHel / helSum[0];
        double PhelDef =
        antDefPtr->antennaFunction(y12ant,y23ant,0.,0.,0.,
                                   hAant,hBant,hi,hj,hk) / helSum[1];
        for (int iVar = 1; iVar < nVariations(); ++iVar) {
          if (iVar == 4)
            Paccept[4] *= antMaxPtr->antennaFunction(y12ant,y23ant,0.,0.,0.,
                                                     hAant,hBant,hi,hj,hk)
            / helSum[4] / PhelUsr;
          else if (iVar == 5)
            Paccept[5] *= antMinPtr->antennaFunction(y12ant,y23ant,0.,0.,0.,
                                                     hAant,hBant,hi,hj,hk)
            / helSum[5] / PhelUsr;
          else
            Paccept[iVar] *= PhelDef / PhelUsr;
        }
        
      }
      
      if (verbose >= 7)
        cout<<"VS: selected "<<int(hAant)<<" "<<int(hBant)<<" -> "
        <<hi<<" "<<hj<<" "<<hk<<", isSwapped = "<<bool2str(isSwapped)<<endl;
      
      // Assign helicities (taking swapped invariants into account)
      if (!isSwapped) {
        //cout<<" Selected : " <<hA<<" "<<hB<<" > "<<hi<<" "<<hj<<" "<<hk<<endl;
        winnerPtr->new1.pol(hi);
        winnerPtr->new2.pol(hj);
        winnerPtr->new3.pol(hk);
      }
      else {
        //cout<<" Selected : " <<hB<<" "<<hA<<" > "<<hk<<" "<<hj<<" "<<hi<<endl;
        winnerPtr->new1.pol(hk);
        winnerPtr->new2.pol(hj);
        winnerPtr->new3.pol(hi);
      }
      
    }
    // If not polarized
    else {
      usePolarizationNow = false;
      winnerPtr->new1.pol(9);
      winnerPtr->new2.pol(9);
      winnerPtr->new3.pol(9);
    }
    
    //***********************************************************
    //$ Set mother and color-flow information in the event record
    //************************************************************
    
    // Set mothers
    winnerPtr->new1.mothers(winnerPtr->iOld1,winnerPtr->iOld2);
    winnerPtr->new2.mothers(winnerPtr->iOld1,winnerPtr->iOld2);
    winnerPtr->new3.mothers(winnerPtr->iOld1,winnerPtr->iOld2);
    
    // Set colour flow, deleting branched dipoles and adding new ones.
    winnerPtr->new1.acol(winnerPtr->old1.acol());
    winnerPtr->new1.col(0);
    winnerPtr->new2.acol(0);
    winnerPtr->new2.col(0);
    winnerPtr->new3.acol(0);
    winnerPtr->new3.col(winnerPtr->old2.col());
    if (winnerPtr->new1.colType() > 0) {
      int col=event.nextColTag();
      winnerPtr->new1.col(col);
      winnerPtr->new2.acol(col);
    }
    if (winnerPtr->new2.colType() > 0) {
      int col=event.nextColTag();
      winnerPtr->new2.col(col);
      winnerPtr->new3.acol(col);
    }
    
    //$ Set masses in the event record
    //$ <note> MR 17.7.2010: there is a problem with particles which are
    //$ off-shell by more than TINY, set the masses based on ID and
    //$ check off-shellness here
    //$ <note> PS 23.2.2011: changed denominator in offshell to sP instead of qNew
    //$ and modified linear to squared dependence, since that's really what
    //$ the numerical precision is driven by
    winnerPtr->new1.m( (vinComPtr->isMassive(winnerPtr->new1.id()) ?
                        winnerPtr->new1.m0() : 0.0) );
    winnerPtr->new2.m( (vinComPtr->isMassive(winnerPtr->new2.id()) ?
                        winnerPtr->new2.m0() : 0.0) );
    winnerPtr->new3.m( (vinComPtr->isMassive(winnerPtr->new3.id()) ?
                        winnerPtr->new3.m0() : 0.0) );
    double offsh1 = abs( winnerPtr->new1.m2() - winnerPtr->new1.p().m2Calc() );
    double offsh2 = abs( winnerPtr->new2.m2() - winnerPtr->new2.p().m2Calc() );
    double offsh3 = abs( winnerPtr->new3.m2() - winnerPtr->new3.p().m2Calc() );
    // Relative to branching scale (TINY should only measure ratios)
    offsh1 /= sP;
    offsh2 /= sP;
    offsh3 /= sP;
    //$1 check off-shellness
    if (offsh1 > TINY || offsh2 > TINY || offsh3 > TINY) {
      if (verbose >= 3) {
        printOut("VS::acceptTrial",
                 "new particle failed off-shell test");
        cout << "p1 =	" << winnerPtr->new1.p();
        cout << "p2 =	" << winnerPtr->new2.p();
        cout << "p3 =	" << winnerPtr->new3.p();
        cout << setprecision(16);
        cout << "TINY		= " << TINY << endl;
        cout << "offshell1	= " << offsh1 << endl;
        cout << "offshell2	= " << offsh2 << endl;
        cout << "offshell3	= " << offsh3 << endl;
      }
      winnerPtr->nHull++;
      ++nTrialsHull;
      //$1
      return false;
    }
    
    //------------------------------------------------------------------------
    // *) AlphaS
    //
    // Impose default choice. Can differ slighly from trial even when running
    // inside trial integral, due to flavor thresholds. Here, alphaS(mu) is
    // returned directly, with the number of flavors active at mu, whereas
    // the number of flavors in the trial integral is controlled by the value
    // of the trial scale.
    double alphaTrial = winnerPtr->getAlphaTrial(iTrial);
    // Default is pT for gluon emission and mQQ for gluon splitting
    double yMuDef     = y12 * y23;
    if (winnerPtr->new2.colType() == -1)
      yMuDef = y23 + pow2(mu2) + pow2(mu3);
    else if (winnerPtr->new2.colType() == 1)
      yMuDef = y12 + pow2(mu1) + pow2(mu2);
    // User choice:
    double yMuUsr      = yMuDef;
    if (alphaSmode == 0) yMuUsr = 1.0;
    yMuUsr *= pow2(alphaSkMu);
    yMuDef *= pow2(alphaSkMuDef);
    // All variations use same nF as default for CMW computation
    double nF = getNf( sqrt(yMuDef * sAnt) );
    double facCMWdef  = (alphaScmwDef) ? alphaSptr->facCMW(nF) : 1.0;
    double facCMWusr  = (alphaScmw) ? facCMWdef : 1.0;
    double mu2min     = pow2(alphaSmuMin);
    double mu2usr     = max(mu2min, yMuUsr * sAnt / pow2(facCMWusr));
    double alphaSusr  = min(alphaSmax, alphaSptr->alphaS(mu2usr));
    // Sanity check (allow small > 1 due to wrongly guessed number of flavors
    // for trial generation)
    if (verbose >= 1 && alphaSusr > 1.1*alphaTrial) {
      cout<<" alphaS reweighting problem : aStrial = "<<alphaTrial<<" aSusr = "<<alphaSusr<<" pT = "<<sqrt(yMuDef * sAnt)<<" muUsr = "<<sqrt(mu2usr)<<endl;
    }
    // Reweight central accept probability by alphaSusr / alphaTrial
    Paccept[0] *= alphaSusr / alphaTrial;
    // Do uncertainty variations
    if (uncertaintyBands) {
      double mu2def     = max(mu2min, yMuDef * sAnt / pow2(facCMWdef));
      double alphaSdef  = min(alphaSmax, alphaSptr->alphaS(mu2def));
      // Up/down variatons
      double kMuVar2   = pow2(settingsPtr->parm("Vincia:uncertaintyBandsKmu"));
      double mu2hi     = max(mu2min, yMuDef * sAnt * kMuVar2 / pow2(facCMWdef));
      double mu2lo     = max(mu2min, yMuDef * sAnt / kMuVar2 / pow2(facCMWdef));
      double alphaShi  = min(alphaSmax, alphaSptr->alphaS(mu2lo));
      double alphaSlo  = min(alphaSmax, alphaSptr->alphaS(mu2hi));
      // Reweight uncertainty accept probabilities by alphaVar / alphaTrial
      for (int iVar = 1; iVar < nVariations(); ++iVar) {
        if (iVar == 1) Paccept[iVar] *= alphaSdef/alphaTrial;
        else if (iVar == 2) Paccept[iVar] *= alphaShi/alphaTrial;
        else if (iVar == 3) Paccept[iVar] *= alphaSlo/alphaTrial;
        else Paccept[iVar] *= alphaSdef/alphaTrial;
      }
      
      // NLO K-factor variation
      // (largely degenerate with scale variation at LO*LL, but could in
      // principle behave differently at (N)LO*(N)LL)
      double mu2nlo    = max(mu2min, sAnt);
      double alphaSnlo = min(alphaSmax, alphaSptr->alphaS(mu2nlo));
      Paccept[6]  *= (1.+alphaSnlo);
      Paccept[7]  /= (1.+alphaSnlo);
    }
    
    
    //************************************************************************
    // Matching: Which order are we currently at?
    // Update and save color-ordered list of particles
    //************************************************************************
    
    int nGnow  = nG;
    int nQQnow = nQQ;
    if (winnerPtr->new2.id() == 21) ++nGnow;
    else {
      ++nQQnow;
      nGnow -= 1;
    }
    int nQCDnow   = nGnow + 2*nQQnow -2;
    bool doMatch  = (nQCDnow <= matchingLO);
    
    // Matching: check if state comes from a resonance decay
    int iAncestor = winnerPtr->iAncestor;
    if (iAncestor <= 0) doMatch = false;
    
    // Matching: create ID and parton lists
    vector<int> idPartons, idPartonsOrg;
    vector<Particle*> partonPtrs, partonPtrsOrg;
    if (doMatch) {
      // First store ancestor
      idPartons.push_back(event[iAncestor].id());
      partonPtrs.push_back(&event[iAncestor]);
      partonPtrsOrg.push_back(&event[iAncestor]);
      // Then store pre- and post-branching systems
      for (int iAnt=0;iAnt<int(branchelementals.size());++iAnt) {
        Branchelemental* prevAntPtr=&branchelementals[iAnt];
        if (prevAntPtr->iOld1 == winnerPtr->iOld1 &&
            prevAntPtr->iOld2 == winnerPtr->iOld2) {
          idPartons.push_back(winnerPtr->new1.id());
          idPartons.push_back(winnerPtr->new2.id());
          idPartons.push_back(winnerPtr->new3.id());
          partonPtrsOrg.push_back(&winnerPtr->old1);
          partonPtrsOrg.push_back(&winnerPtr->old2);
          partonPtrs.push_back(&winnerPtr->new1);
          partonPtrs.push_back(&winnerPtr->new2);
          partonPtrs.push_back(&winnerPtr->new3);
        } else {
          if (prevAntPtr->iOld1 != winnerPtr->iOld2) {
            idPartons.push_back(event[prevAntPtr->iOld1].id());
            partonPtrs.push_back(&event[prevAntPtr->iOld1]);
            partonPtrsOrg.push_back(&event[prevAntPtr->iOld1]);
          }
          if (event[prevAntPtr->iOld2].colType() != 2) {
            idPartons.push_back(event[prevAntPtr->iOld2].id());
            partonPtrs.push_back(&event[prevAntPtr->iOld2]);
            partonPtrsOrg.push_back(&event[prevAntPtr->iOld2]);
          }
        }
      }
      
      // Matching: check if ME exists for this state
      doMatch = matchingPtr->isMatched(1,idPartons,usePolarizationNow);
    }
    
    //------------------------------------------------------------------------
    // *) Match to fixed-order QCD matrix elements (from MadGraph)
    
    // Default is not to impose any matching scale
    bool imposeMatchingScale = false;
    double q2match(0.);
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
      if (matchingRegType == 1) q2damp = y12 * y23 * sAnt;
      else if (matchingRegType == 2) q2damp = min(y12,y23) * sAnt;
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
    
    // Get matching coefficients
    string HPME = "none";
    if (doMatch) {
      
      //getPME computes the matching factor -> PME
      if (matchingPtr->getPME(1,partonPtrs,PME)) {
        
        for (int iVar=0; iVar < nVariations(); ++iVar)
          Paccept[iVar] *= 1.0 + DeltaDamp * (PME[iVar] - 1.);
        
        // Do NLO matching
        if (matchingNLO >= nQCDnow + 1) {
          vector<double> V1;
          if ( matchingPtr->getV1(1,partonPtrs,V1) ) {
            if (verbose >= 8) printOut("VS::acceptTrial","Applying NLO Correction"
                                       " = "+num2str(1.+V1[0]));
            // Apply NLO correction, for each variation separately
            for (int iVar = 0; iVar < nVariations(); ++iVar)
              Paccept[iVar] *= max(0., 1. + V1[iVar]);
          } else {
            if (verbose >= 5) printOut("VS::acceptTrial","Failed to compute V1");
          }
        }
        
      }
      
      // Global showers: compute PME from sum over histories (using getPME)
      if (!sectorShower) {
        if ((Paccept[0] > 1.0 + TINY) && verbose >= 3) {
          cout <<" order = "<<nQCDnow<<"  Bare PME = "<<PME[0]<<" at qNew = "<<qNew<<" mDamp = "<<sqrt(q2damp)<<" mDamp/mMatch = 1/"<<sqrt(q2match/q2damp)<<" Pimp = "<<Pimp << " Pari = "<<Pari<<" DeltaDamp = "<<DeltaDamp<<endl;
          cout <<"       ijk = "<<winnerPtr->new1.id()<<" "<<winnerPtr->new2.id()<<" "
          <<winnerPtr->new3.id()
          <<"  mij = "<<sqrt(m2(winnerPtr->new1.p()+winnerPtr->new2.p()))
          <<" mjk = "<<sqrt(m2(winnerPtr->new2.p()+winnerPtr->new3.p()))
          <<" mijk = "<<sqrt(sP);
          cout <<"      yij = "<< m2(winnerPtr->new1.p()+winnerPtr->new2.p())/sP
          <<" yjk = "<<m2(winnerPtr->new2.p()+winnerPtr->new3.p())/sP<<endl;
          
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
    
    // Limit uncertainty to factor 2.0 for first branching, to
    // capture possibly large finite term differences, then level off at
    // 20% asymptotically, to capture multiple-emission uncertainties.
    if (uncertaintyBands) {
      double limit = 2.0 - min(0.8,0.05*(nQCDnow));
      for  (int iVar=1; iVar<nVariations(); iVar++) {
        if (Paccept[iVar]>limit*Paccept[0]) Paccept[iVar] = limit * Paccept[0];
        if (Paccept[iVar]<Paccept[0]/limit) Paccept[iVar] = Paccept[0] / limit;
      }
    }
    
    //------------------------------------------------------------------------
    // *) Accept or Reject Trial Emission
    
    // Verbose output
    if (verbose >= 8) {
      cout<<" Paccept  = "<<setprecision(6)<<Paccept[0]<<endl;
    }
    
    // Accept/Reject Trial
    double ran = rndmPtr->flat();
    if (ran > Paccept[0]) {
      if (verbose >= 5) {
        printOut("VS::branch","=== Branching Vetoed. wPhys/wTrial = "
                 +num2str(Paccept[0]));
      }
      // Check if branching would have happened if we had been using
      // another radiation function
      if (uncertaintyBands) {
        for  (int iVar=1; iVar<nVariations(); iVar++) {
          double Pacc = Paccept[0];
          double Pvar = Paccept[iVar];
          double rw = (1. - Pvar)/(1. - Pacc);
          scaleWeight(rw,iVar);
        }
      }
      // For each failed trial, attempt to (slowly) optimize headroomfactors
      if (nQCDnow <= matchingLO && winnerPtr->nVeto >= 1) {
        if (nGnow > nG) {
          double headRoomFactor = getHeadRoomFactorEmit(nG,nQQ);
          if (headRoomFactor > 0.5)
            setHeadRoomEmit(0.999*headRoomFactor, nG, nQQ);
        } else {
          double headRoomFactor = getHeadRoomFactorSplit(nG,nQQ);
          if (headRoomFactor > 0.5)
            setHeadRoomSplit(0.999*headRoomFactor, nG, nQQ);
        }
      }
      // Count up number of vetoed branchings
      winnerPtr->nVeto++;
      ++nTrialsVeto;
      return false;
    } else if (Paccept[0] > 1.0+TINY) {
      if ( (verbose >= 2 && Paccept[0] > 1.1)
          || (verbose >= 1 && Paccept[0] > 1.5) ) {
        printOut("VS::branch",
                 "===  wPhys/wTrial = "+num2str(Paccept[0],6)
                 +" at QCD order "+num2str(nQCDnow,3)
                 +", "+winnerPtr->old1.name()+winnerPtr->old2.name()
                 +"->"+winnerPtr->new1.name()+winnerPtr->new2.name()
                 +winnerPtr->new3.name()
                 +" ("+winnerPtr->trialGenPtrSav[iTrial]->name()+")"
                 +", QE = "+num2str(qNew)
                 +", mIK = "+num2str(sqrt(sP),6));
        if (doMatch && verbose >= 8) cout<<" PME = "<<PME[0]<<endl;
        if (verbose >= 5) winnerPtr->list();
        if ((Paccept[0] > 1.0 && verbose >= 5) || (Paccept[0] > 2.0 && verbose >= 4) || (Paccept[0] > 10.0 && verbose >= 3)) {
          cout<<" Momentum Listing: "<<endl;
          for (int ip=0; ip<int(partonPtrs.size())-1; ip++) {
            cout << " id"<<ip<<" = "<<num2str(partonPtrs[ip+1]->id(),7)
            << " p"<<" = " << partonPtrs[ip+1]->p();
          }
        }
      }
      
      // Max reweight by factor 10
      if (allowReweighting) scaleWeight(min(10.,Paccept[0]),0);
      
      // Automatically increase headroom factor when violations encountered,
      // up to a maximum of a factor 5.
      if (nQCDnow <= matchingLO) {
        if (nGnow > nG) {
          double headRoomFactor = getHeadRoomFactorEmit(nG,nQQ);
          setHeadRoomEmit(min(1.2,Paccept[0])*headRoomFactor, nG, nQQ);
        } else {
          double headRoomFactor = getHeadRoomFactorSplit(nG,nQQ);
          setHeadRoomSplit(min(1.2,Paccept[0])*headRoomFactor, nG, nQQ);
        }
      }
      
    }
    
    // For accepted branching, rescale uncertainty event weights
    if (uncertaintyBands) {
      for (int iVar=1; iVar<nVariations(); iVar++) {
        scaleWeight(Paccept[iVar]/Paccept[0],iVar);
      }
    }
    
    // Fill diagnostics histos?
    if (verbose >= 2 && doMatch) {
      string state = num2str(2*nQQnow,1) + "q" + num2str(nGnow,1) + "g";
      string HPacc = "Log10(ME/AntPhys):" + state;
      vinciaHistos[HPacc].fill(log10(max(TINY,PME[0])));
      string HqPhys = "Ln(q2/sSystem):" + state;
      vinciaHistos[HqPhys].fill(log(max(TINY,pow2(qNew/mSystem))));
    }
    
    //------------------------------------------------------------------------
    // *) Store winner in list of winners
    
    // Update saved matrix element (sector Ordering only)
    if (sectorShower) MESav = MENew;
    
    // Store vector of winners
    winners.push_back(*winnerPtr);
    
    return true;
  }
  
  
  //*********
  
  /* Check whether radiation falls into sector
   Inputs:
   event : event record before branching
   trial : proposed trial branching
   Output:
   sectorAccept : false if branching should be vetoed
   */
  
  bool VinciaShower::sectorAccept(Event& event,Branchelemental* trialPtr) {
    
    // Only do check if evolution mode is not global
    if ( ! sectorShower ) return true;
    
    // Verbose output
    if (verbose >= 7) {
      printOut("VS::sectorAccept","begin --------------");
    }
    
    // Check if this sector has smallest sector measure
    int sectorMeasureType = 10;
    if (trialPtr->new2.id() < 0) sectorMeasureType = 8;
    else if (trialPtr->new2.id() != 21) sectorMeasureType = 9;
    double QS2 = resolutionPtr->Q2E(trialPtr->new1,trialPtr->new2,trialPtr->new3,
                                    sectorMeasureType);
    // Now compute those of alternative clusterings (= competing sectors)
    double QS2Min = pow2(mSystem);
    for (int iAnt = 0; iAnt < int(branchelementals.size()); ++iAnt) {
      int ia = branchelementals[iAnt].iOld1;
      int ir = branchelementals[iAnt].iOld2;
      int ib = branchelementals[iAnt].iR;
      // skip if no right-hand neighbor
      if (ib == 0) continue;
      // Compute sector resolution function QS2 for this triplet
      Vec4 pa = event[ia].p();
      Vec4 pr = event[ir].p();
      Vec4 pb = event[ib].p();
      //int idpa = event[ia].id();
      int idpr = event[ir].id();
      //int idpb = event[ib].id();
      
      // use new particles if neigboring pts affected by recoils
      if (ir == trialPtr->iOld1) {
        pr = trialPtr->new1.p();
        pb = trialPtr->new2.p();
        idpr = trialPtr->new1.id();
        //idpb = trialPtr->new2.id();
      } else if (ib == trialPtr->iOld1) {
        pb = trialPtr->new1.p();
        //idpb = trialPtr->new1.id();
      } //it is not "else if" to account for cyclic topologies
      if (ia == trialPtr->iOld2) {
        pa = trialPtr->new3.p();
        //idpa = trialPtr->new3.id();
      } else if (ir == trialPtr->iOld2) {
        pa = trialPtr->new2.p();
        pr = trialPtr->new3.p();
        //idpa = trialPtr->new2.id();
        idpr = trialPtr->new3.id();
      }
      
      // we use David's prescription for quarks. We rely on color ordering:
      // antiquarks of splitted pairs are always before the corresponding
      // quark
      // TODO: make sure types 8 and 9 also make sense for massive quarks
      sectorMeasureType = 10;
      if (idpr < 0) sectorMeasureType = 8;
      else if (idpr != 21) sectorMeasureType = 9;
      QS2Min = min(QS2Min,resolutionPtr->Q2E(pa,pr,pb,sectorMeasureType));
      
      if (verbose >= 8) {
        printOut("sectorAccept","testing sector no "+num2str(iAnt)+" QS2 = "
                 +num2str(QS2)+" vs QS2MinNow = "+num2str(QS2Min));
      }
      if (QS2 > QS2Min) return false;
    }
    
    // Return
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
    <<bool2str(settingsPtr->flag("Vincia"),9) <<"\n |\n";
    if (settingsPtr->flag("Vincia")) {
      cout << " | Matching:                          = ";
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
      << num2str(settingsPtr->mode("Vincia:evolutionType"),9)<<"\n";
      cout << " |                 orderingMode       = "
      << num2str(settingsPtr->mode("Vincia:orderingMode"),9)<<"\n";
      cout << " |                 helicityShower     = "
      << bool2str(settingsPtr->flag("Vincia:helicityShower"),9)<<"\n";
      cout << " |                 sectorShower       = "
      << bool2str( settingsPtr->flag("Vincia:sectorShower"),9)<<"\n";
      cout << " |                 nGluonToQuark      = "
      << num2str(nGluonToQuark,9)<<"\n";
      if (verbose >= 2) {
        cout << " |                 isMassiveS         = "
        << bool2str(vinComPtr->isMassive(3),9)<<"\n";
        cout << " |                 isMassiveC         = "
        << bool2str(vinComPtr->isMassive(4),9)<<"\n";
        cout << " |                 isMassiveB         = "
        << bool2str(vinComPtr->isMassive(5),9)<<"\n";
      }
      cout << " |                 QminAll            = "
      << num2str(QminAll,9)<<"\n";
      cout << " |\n | Alpha_s:        alphaS(mZ)|MSbar   = "
      << num2str(alphaSvalue,9)<<"\n"
      << " |                 order              = "
      << num2str(alphaSorder,9)<<"\n";
      if (alphaSorder >= 1) {
        cout << " |                 LambdaQCD[nF]      = "
        << num2str(Lambda3flav,9)<<"[3] "<<num2str(Lambda4flav,7)<<"[4] "
        << num2str(Lambda5flav,7)<<"[5] "<<num2str(Lambda6flav,7)<<"[6]\n";
        cout << " |                 mode               = "
        << num2str(settingsPtr->mode("Vincia:alphaSmode"),9)<<"\n";
        cout << " |                 CMW rescaling      = "
        << bool2str(alphaScmw,9)<<"\n";
        cout << " |                 kMu                = "
        << num2str(alphaSkMu,9)<<"\n";
        cout << " |                 alphaSmax          = "
        << num2str(alphaSmax,9)<<"\n";
      }
      cout << " |\n"
      << " | IR Reg.:        cutoffType         = "
      << setw(12)<<num2str(settingsPtr->mode("Vincia:cutoffType"),9)<<(settingsPtr->mode("Vincia:cutoffType") == -1 ? " (Pythia 8 pTevol)" :" ")<<"\n"
      << " |                 cutoffScale        = "
      <<setw(12)<<num2str(cutoffScale,9)<<"\n |\n";
      
      if ( settingsPtr->word("Vincia:antennaFile") != "none") {
        cout<<" | Antennae:       antennaFile        = "
        <<settingsPtr->word("Vincia:antennaFile")<<"\n |\n";
      }
      
      if (verbose >= 2) {
        for (int iAnt=0;iAnt<=4;++iAnt) {
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
            ( settingsPtr->flag("Vincia:useCollinearTerms") ? "on":"off")
            <<"\n";
            cout<<" |   useFiniteTerms     = "<<setw(6)<<
            ( settingsPtr->flag("Vincia:useFiniteTerms") ? "on":"off")<<"\n";
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
  
  // Print the list of dipoles.
  
  void VinciaShower::list(ostream& os) const {
    // Loop over dipole list and print it.
    for (int iAnt = 0; iAnt < int(branchelementals.size()); ++iAnt)
      if (branchelementals.size() == 1) branchelementals[iAnt].list(os,true,true);
      else if ( iAnt == 0 ) branchelementals[iAnt].list(os,true,false);
      else if ( iAnt == int(branchelementals.size())-1 ) branchelementals[iAnt].list(os,false,true);
      else branchelementals[iAnt].list(os);
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
    // TMP
    if (verbose >= 2) {
      os << " | Trial Veto Rates:   Hull = "<<nTrialsHull*1.0/nTrialsSum<<"   Massive "<<nTrialsHullMassive*1.0/nTrialsSum<<"   Had M+Q = "<<nTrialsCutM*1.0/nTrialsSum<<" + "<<nTrialsCutQ*1.0/nTrialsSum<<"   Veto = "<<nTrialsVeto*1.0/nTrialsSum<<"  |"<<endl;
    }
    os << " |                                                                        "
    <<setw(40)<<" "<<" |\n";
    os << " *-------  End VINCIA Statistics ----------------------------------------------------------------------------------*\n\n";
  }
  
  //**********************************************************************
  
  // Branchelemental class member functions
  
  //*********
  // Simple print utility, showing the contents of the Branchelemental
  
  void Branchelemental::list(ostream& os, bool header, bool footer) const { 
    if (header) {
      // Header.
      os << "\n --------  VINCIA Dipole-Antenna Listing  ------------------------------"
      << "------------------------------------------------------------ \n \n" 
      << "  sys       antenna type   pols    col    L aunts   parents  R uncles"
      << "         m  Ord  Saved Trial Swap    Qtrial "
      << " nVeto nHull nHadr\n"
      << fixed << setprecision(3);
    }
    os << setw(5) << system<<" ";
    os << setw(9) << old1.name();
    os << setw(9) << old2.name()<<"  ";
    os << setw(2) << int(old1.pol()) <<" "<<setw(2)<< int(old2.pol())<<" ";
    os << setw(6) << old1.col()<<" ";
    os << setw(5) << iLL << setw(5) << iL;
    os << setw(5) << iOld1 << setw(5) << iOld2;
    os << setw(5) << iR << setw(5) << iRR;
    os << num2str(m(),10);
    os << setw(5) << (unOrdering ? "n" : "y");
    int iWin = getTrialIndex();
    if (nTrialGenerators() >= 1 && iWin >= 0) {
      if (hasSavedTrial[iWin]) {
        os<< setw(13)<< trialGenPtrSav[iWin]->name();
        os << setw(5) << (isSwappedSav[iWin] ? "y" : "n");
        os<< num2str(scaleSav[iWin],10);
      }
    } else {
      os<<setw(13)<<"-";
      os<<setw(15)<<" ";
    }
    os << " "<<setw(6)<<nVeto;   
    os << " "<<setw(5)<<nHull;   
    os << " "<<setw(5)<<nHadr;   
    os << endl;   
    if (footer) {
      os << "\n --------  End VINCIA VinciaShower Dipole-Antenna Listing  ------------"
      << "-------------------------------------------------------------\n";
    }
  }
  
} // end namespace VINCIA
