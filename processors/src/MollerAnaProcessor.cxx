/**
 *@file mollerAnaProcessor.cxx
 *@author Tongtong, UNH
 */

#include "../include/MollerAnaProcessor.h"

#include <iostream>

#include "TF1.h"
#include "math.h"

#define ELECTRONMASS 0.000510998950 // GeV
#define PI 3.14159265358979
#define ROTATIONANGLEAROUNDY 0.0305 // rad
#define MOMSCALE 1.761/2.09

MollerAnaProcessor::MollerAnaProcessor(const std::string& name, Process& process) : Processor(name,process) {

}

MollerAnaProcessor::~MollerAnaProcessor(){}

void MollerAnaProcessor::configure(const ParameterSet& parameters) {
    std::cout << "Configuring mollerAnaProcessor" <<std::endl;
    try
    {
        debug_           = parameters.getInteger("debug");
        anaName_         = parameters.getString("anaName");

        trkColl_    = parameters.getString("trkColl");
        vtxColl_ = parameters.getString("vtxColl",vtxColl_);

        selectionCfg_   = parameters.getString("vtxSelectionjson",selectionCfg_);
        histCfgFilename_      = parameters.getString("histCfg",histCfgFilename_);

        beamE_  = parameters.getDouble("beamE",beamE_);
    }
    catch (std::runtime_error& error)
    {
        std::cout << error.what() << std::endl;
    }

}

void MollerAnaProcessor::initialize(TTree* tree) {
	_ah =  std::make_shared<AnaHelpers>();

    vtxSelector  = std::make_shared<BaseSelector>(anaName_+"_"+"vtxSelection",selectionCfg_);
    vtxSelector->setDebug(debug_);
    vtxSelector->LoadSelection();

    // init histos
    histos = new MollerAnaHistos(anaName_.c_str());
    histos->loadHistoConfig(histCfgFilename_);
    histos->DefineHistos();

    // init TTree
    tree_= tree;
    tree_->SetBranchAddress(tsColl_.c_str(), &tsData_ , &btsData_);
    tree_->SetBranchAddress(trkColl_.c_str() , &trks_, &btrks_);
    tree_->SetBranchAddress(vtxColl_.c_str(), &vtxs_ , &bvtxs_);
}

bool MollerAnaProcessor::process(IEvent* ievent) {
    double weight = 1.;


	std::vector<Track> tracks_pos_top;
	std::vector<Track> tracks_pos_bot;
	std::vector<Track> tracks_neg_top;
	std::vector<Track> tracks_neg_bot;

	tracks_pos_top.clear();
	tracks_pos_bot.clear();
	tracks_neg_top.clear();
	tracks_neg_bot.clear();

	for(int i = 0; i < trks_->size(); i++){
		Track* track = trks_->at(i);

		int charge = track->getCharge();
		double py = track->getMomentum()[1];

		if (charge == 1 && py > 0){
			tracks_pos_top.push_back(*track);
			histos->Fill1DHisto("chi2ndf_pos_top_h", track->getChi2Ndf(), weight);
			histos->Fill1DHisto("nHits_pos_top_h", track->getTrackerHitCount(), weight);
			histos->Fill1DHisto("p_pos_top_h", track->getP(), weight);
		}
		else if (charge == 1 && py < 0){
			tracks_pos_bot.push_back(*track);
			histos->Fill1DHisto("chi2ndf_pos_bot_h", track->getChi2Ndf(), weight);
			histos->Fill1DHisto("nHits_pos_bot_h", track->getTrackerHitCount(), weight);
			histos->Fill1DHisto("p_pos_bot_h", track->getP(), weight);
		}
		else if (charge == -1 && py > 0){
			tracks_neg_top.push_back(*track);
			histos->Fill1DHisto("chi2ndf_neg_top_h", track->getChi2Ndf(), weight);
			histos->Fill1DHisto("nHits_neg_top_h", track->getTrackerHitCount(), weight);
			histos->Fill1DHisto("p_neg_top_h", track->getP(), weight);
		}
		else if (charge == -1 && py < 0){
			tracks_neg_bot.push_back(*track);
			histos->Fill1DHisto("chi2ndf_neg_bot_h", track->getChi2Ndf(), weight);
			histos->Fill1DHisto("nHits_neg_bot_h", track->getTrackerHitCount(), weight);
			histos->Fill1DHisto("p_neg_bot_h", track->getP(), weight);
		}
	}


	int num_tracks_pos_top = tracks_pos_top.size();
	int num_tracks_pos_bot = tracks_pos_bot.size();
	int num_tracks_neg_top = tracks_neg_top.size();
	int num_tracks_neg_bot = tracks_neg_bot.size();


	histos->Fill1DHisto("num_tracks_pos_top_h", num_tracks_pos_top, weight);
	histos->Fill1DHisto("num_tracks_pos_bot_h", num_tracks_pos_bot, weight);
	histos->Fill1DHisto("num_tracks_neg_top_h", num_tracks_neg_top, weight);
	histos->Fill1DHisto("num_tracks_neg_bot_h", num_tracks_neg_bot, weight);

	vtxSelector->getCutFlowHisto()->Fill(0.,weight);


    if (!vtxSelector->passCutEq("num_tracks_pos_top_eq", num_tracks_pos_top,weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutEq("num_tracks_pos_bot_eq", num_tracks_pos_bot,weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutEq("num_tracks_neg_top_eq", num_tracks_neg_top,weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutEq("num_tracks_neg_bot_eq", num_tracks_neg_bot,weight)){
        vtxSelector->clearSelector();
    	return true;
    }


    Track track_neg_top = tracks_neg_top[0];
    Track track_neg_bot = tracks_neg_bot[0];

    if (!vtxSelector->passCutGt("num_hits_tracks_neg_top_gt", track_neg_top.getTrackerHitCount(),weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutGt("num_hits_tracks_neg_bot_gt", track_neg_bot.getTrackerHitCount(),weight)){
        vtxSelector->clearSelector();
    	return true;
    }


    double time_neg_top = track_neg_top.getTrackTime();
    double time_neg_bot = track_neg_bot.getTrackTime();
	histos->Fill1DHisto("time_tracks_neg_top_with_numTrakcs_numHits_cuts_h", time_neg_top, weight);
	histos->Fill1DHisto("time_tracks_neg_bot_with_numTrakcs_numHits_cuts_h", time_neg_bot, weight);
	histos->Fill2DHisto("time_tracks_neg_top_vs_bot_with_numTrakcs_numHits_cuts_hh", time_neg_top, time_neg_bot, weight);

    double time_diff = time_neg_top - time_neg_bot;
	histos->Fill1DHisto("time_diff_with_numTrakcs_numHits_cuts_h", time_diff, weight);

    if (!vtxSelector->passCutLt("time_diff_lt", fabs(time_diff),weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    std::vector<double> mom_neg_top = track_neg_top.getMomentum();
    std::vector<double> mom_neg_bot = track_neg_bot.getMomentum();

    TLorentzVector* vect_neg_top = new TLorentzVector();
    vect_neg_top->SetXYZM(mom_neg_top[0], mom_neg_top[1], mom_neg_top[2], ELECTRONMASS);
    TLorentzVector* vect_neg_bot = new TLorentzVector();
    vect_neg_bot->SetXYZM(mom_neg_bot[0] * MOMSCALE, mom_neg_bot[1] * MOMSCALE, mom_neg_bot[2] * MOMSCALE, ELECTRONMASS);


    double p_neg_top = vect_neg_top->P();
    double p_neg_bot = vect_neg_bot->P();
    double pSum = p_neg_top + p_neg_bot;
    double pDiff = p_neg_top - p_neg_bot;
    double im = (*vect_neg_top + *vect_neg_bot).M();

	histos->Fill1DHisto("p_tracks_neg_top_with_numTrakcs_numHits_timeDiff_cuts_h", p_neg_top, weight);
	histos->Fill1DHisto("p_tracks_neg_bot_with_numTrakcs_numHits_timeDiff_cuts_h", p_neg_bot, weight);
	histos->Fill1DHisto("pDiff_with_numTrakcs_numHits_timeDiff_cuts_h", pDiff, weight);

	histos->Fill1DHisto("pSum_with_numTrakcs_numHits_timeDiff_cuts_h", pSum, weight);
	histos->Fill1DHisto("im_with_numTrakcs_numHits_timeDiff_cuts_h", im, weight);
	histos->Fill2DHisto("im_vs_pSum_with_numTrakcs_numHits_timeDiff_cuts_hh", pSum, im, weight);

    if (!vtxSelector->passCutLt("pTop_lt", p_neg_top, weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutLt("pBot_lt", p_neg_bot, weight)){
        vtxSelector->clearSelector();
    	return true;
    }

    if (!vtxSelector->passCutLt("pDiff_lt", fabs(pDiff), weight)){
        vtxSelector->clearSelector();
    	return true;
    }

	histos->Fill1DHisto("pSum_with_numTrakcs_numHits_timeDiff_p_pDiff_cuts_h", pSum, weight);
	histos->Fill1DHisto("im_with_numTrakcs_numHits_timeDiff_p_pDiff_cuts_h", im, weight);
	histos->Fill2DHisto("im_vs_pSum_with_numTrakcs_numHits_timeDiff_p_pDiff_cuts_hh", pSum, im, weight);

    if (!vtxSelector->passCutGt("pSum_gt", pSum, weight)){
        vtxSelector->clearSelector();
    	return true;
    }

	histos->Fill1DHisto("im_with_numTrakcs_numHits_timeDiff_p_pDiff_pSum_cuts_h", im, weight);

    vtxSelector->clearSelector();
    return true;
}

void MollerAnaProcessor::finalize() {
    outF_->cd();
    histos->saveHistos(outF_, anaName_.c_str());
    delete histos;
    histos = nullptr;
    vtxSelector->getCutFlowHisto()->Write();

    outF_->Close();

}

DECLARE_PROCESSOR(MollerAnaProcessor);
