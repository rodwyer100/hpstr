/**
 *@file TriggerParametersExtractionMollerAnaProcessor.cxx
 *@author Tongtong, UNH
 */

#include "TriggerParametersExtractionMollerAnaProcessor.h"

#include <iostream>

#include "TF1.h"
#include "math.h"

#define ELECTRONMASS 0.000510998950 // GeV
#define PI 3.14159265358979
#define CHI2NDFTHRESHOLD 20
#define CLUSTERENERGYTHRESHOLD 0.1 // threshold of cluster energy for analyzable events
#define CLUSTERENERGYMIN 0.71 // minimum of cluster energy
#define CLUSTERENERGYMAX 1.56 // maximum of cluster energy
#define CLUSTERXMAX -10 // maximum of x index
#define CLUSTERYMIN -2 // minimum of y index
#define CLUSTERYMAX 2 // maximum of y index
#define ROTATIONANGLEAROUNDY 0.0305 // rad
#define DIFFENERGYMIN -0.35 // minimum for difference between measured and calculated energy
#define DIFFENERGYMAX 0.34 // maximum for difference between measured and calculated energy
#define DIFFTHETAMIN -0.0028 // minimum for difference between measured and calculated theta before rotation
#define DIFFTHETAMAX 0.0046 // maximum for difference between measured and calculated theta before rotation

TriggerParametersExtractionMollerAnaProcessor::TriggerParametersExtractionMollerAnaProcessor(const std::string& name, Process& process) : Processor(name,process) {

}

TriggerParametersExtractionMollerAnaProcessor::~TriggerParametersExtractionMollerAnaProcessor(){}

void TriggerParametersExtractionMollerAnaProcessor::configure(const ParameterSet& parameters) {
    std::cout << "Configuring EcalTimingAnaProcessor" <<std::endl;
    try
    {
        debug_           = parameters.getInteger("debug");
        anaName_         = parameters.getString("anaName");
        histCfgFilename_      = parameters.getString("histCfg",histCfgFilename_);
        trkColl_    = parameters.getString("trkColl");
        gtpClusColl_    = parameters.getString("gtpClusColl");
        mcColl_  = parameters.getString("mcColl",mcColl_);
        vtxColl_ = parameters.getString("vtxColl",vtxColl_);
    }
    catch (std::runtime_error& error)
    {
        std::cout << error.what() << std::endl;
    }
}

void TriggerParametersExtractionMollerAnaProcessor::initialize(TTree* tree) {
	_ah =  std::make_shared<AnaHelpers>();

    tree_= tree;
    // init histos
    histos = new TriggerParametersExtractionMollerAnaHistos(anaName_.c_str());
    histos->loadHistoConfig(histCfgFilename_);
    histos->DefineHistos();

    // init TTree
    tree_->SetBranchAddress(trkColl_.c_str() , &trks_, &btrks_);
    tree_->SetBranchAddress(gtpClusColl_.c_str() , &gtpClusters_, &bgtpClusters_);
    tree_->SetBranchAddress(mcColl_.c_str() , &mcParts_, &bmcParts_);
    tree_->SetBranchAddress(vtxColl_.c_str(), &vtxs_ , &bvtxs_);

    //Cut functions for X
    func_top_topCutX = new TF1("func_top_topCutX", "pol1", -300, 40);
    func_top_topCutX->SetParameters(top_topCutX);
    func_top_botCutX = new TF1("func_top_botCutX", "pol1", -300, 40);
    func_top_botCutX->SetParameters(top_botCutX);

    func_bot_topCutX = new TF1("func_bot_topCutX", "pol1", -300, 40);
    func_bot_topCutX->SetParameters(bot_topCutX);
    func_bot_botCutX = new TF1("func_bot_botCutX", "pol1", -300, 40);
    func_bot_botCutX->SetParameters(bot_botCutX);

    //Cut functions for Y
    func_top_topCutY = new TF1("func_top_topCutY", "pol1", 30, 90);
    func_top_topCutY->SetParameters(top_topCutY);
    func_top_botCutY = new TF1("func_top_botCutY", "pol1", 30, 90);
    func_top_botCutY->SetParameters(top_botCutY);

    func_bot_topCutY = new TF1("func_bot_topCutY", "pol1", -90, -30);
    func_bot_topCutY->SetParameters(bot_topCutY);
    func_bot_botCutY = new TF1("func_bot_botCutY", "pol1", -90, -30);
    func_bot_botCutY->SetParameters(bot_botCutY);

    //NHits dependence energy
    func_nhde = new TF1("func_nhde", "pol1", 0, 20);
    func_nhde->SetParameters(pars_nhde);

    //Upper limit for position dependent energy
    func_pde_moller = new TF1("func_pde_moller", "pol2", -22, 0);
    func_pde_moller->SetParameters(pars_pde_moller);

    // Kinematic equations
    // E vs theta
    func_E_vs_theta_before_roation = new TF1("func_E_vs_theta_before_roation", "[0]/(1 + 2*[0]/[1]*sin(x/2.)*sin(x/2.))", 0, 1);
    func_E_vs_theta_before_roation->SetParameter(0, beamE_);
    func_E_vs_theta_before_roation->SetParameter(1, ELECTRONMASS);

    // theta1 vs theta2
    func_theta1_vs_theta2_before_roation = new TF1("func_theta1_vs_theta2_before_roation", "2*asin([1]/2./[0] * 1/sin(x/2.))", 0, 1);
    func_theta1_vs_theta2_before_roation->SetParameter(0, beamE_);
    func_theta1_vs_theta2_before_roation->SetParameter(1, ELECTRONMASS);
}

bool TriggerParametersExtractionMollerAnaProcessor::process(IEvent* ievent) {
    double weight = 1.;

	int n_tracks = trks_->size();
	histos->Fill1DHisto("n_tracks_h", n_tracks, weight);

	int n_cl = gtpClusters_->size();
	histos->Fill1DHisto("n_clusters_h", n_cl, weight);

	int n_vtxs = vtxs_->size();
	histos->Fill1DHisto("n_vtxs_h", n_vtxs, weight);

	histos->Fill2DHisto("n_clusters_vs_n_tracks_hh", n_tracks, n_cl, weight);
	histos->Fill2DHisto("n_clusters_vs_n_vtxs_hh", n_vtxs, n_cl, weight);
	histos->Fill2DHisto("n_tracks_vs_n_vtxs_hh", n_vtxs, n_tracks, weight);


	std::vector<Track> tracks_top;
	std::vector<Track> tracks_bot;

	tracks_top.clear();
	tracks_bot.clear();


	for(int i = 0; i < n_tracks; i++){
		Track* track = trks_->at(i);

		int charge = track->getCharge();
		histos->Fill1DHisto("charge_tracks_h", charge, weight);

		std::vector<double> positionAtEcal = track->getPositionAtEcal();
		if(charge == -1 && !isnan(positionAtEcal[2])) {

			histos->Fill1DHisto("chi2ndf_tracks_h", track->getChi2Ndf(), weight);

			if(track->getChi2Ndf() < CHI2NDFTHRESHOLD){
				if (positionAtEcal[1] > 0) {
					tracks_top.push_back(*track);
				}
				else if (positionAtEcal[1] < 0) {
					tracks_bot.push_back(*track);
				}
			}
		}
	}

	int n_tracks_top = tracks_top.size();
	int n_tracks_bot = tracks_bot.size();

	histos->Fill1DHisto("n_tracks_top_with_chi2_cut_h", n_tracks_top, weight);
	histos->Fill1DHisto("n_tracks_bot_with_chi2_cut_h", n_tracks_bot, weight);

	for(int i = 0; i < n_tracks_top; i++) {
		Track trackTop = tracks_top.at(i);
		std::vector<double> positionAtEcalTop = trackTop.getPositionAtEcal();

		for(int j = 0; j < n_tracks_bot; j++) {
			Track trackBot = tracks_bot.at(i);
			std::vector<double> positionAtEcalBot = trackBot.getPositionAtEcal();

			histos->Fill2DHisto("xy_positionAtEcal_track_pair_hh",positionAtEcalTop[0], positionAtEcalTop[1], weight);
			histos->Fill2DHisto("xy_positionAtEcal_track_pair_hh",positionAtEcalBot[0], positionAtEcalBot[1], weight);
		}
	}

	std::vector<CalCluster> clulsters_top;
	std::vector<CalCluster> clulsters_bot;

	for(int i = 0; i < n_cl; i++){
		CalCluster* cluster = gtpClusters_->at(i);
		std::vector<double> positionCluster = cluster->getPosition();
		histos->Fill2DHisto("xy_clusters_without_cut_hh",positionCluster[0], positionCluster[1], weight);

		CalHit* seed = (CalHit*)cluster->getSeed();
		histos->Fill1DHisto("seed_energy_cluster_without_cut_h", seed->getEnergy(), weight);
		histos->Fill1DHisto("energy_cluster_without_cut_h", cluster->getEnergy(), weight);
		histos->Fill1DHisto("n_hits_cluster_without_cut_h", cluster->getNHits(), weight);
		histos->Fill2DHisto("energy_vs_n_hits_cluster_without_cut_hh", cluster->getNHits(), cluster->getEnergy(), weight);

		int ix = seed -> getCrystalIndices()[0];
		if(ix < 0) ix++;
		int iy = seed -> getCrystalIndices()[1];

		histos->Fill1DHisto("n_clusters_xAxis_without_cut_h", ix, weight);
		histos->Fill2DHisto("xy_indices_clusters_without_cut_hh",ix, iy, weight);
		histos->Fill2DHisto("energy_vs_ix_clusters_without_cut_hh", ix, cluster->getEnergy(), weight);
		histos->Fill2DHisto("energy_vs_iy_clusters_without_cut_hh", iy, cluster->getEnergy(), weight);

		if(iy > 0 ){
			clulsters_top.push_back(*cluster);
		}
		else if(iy < 0) {
			clulsters_bot.push_back(*cluster);
		}
	}

	int n_clusters_top = clulsters_top.size();
	int n_clusters_bot = clulsters_bot.size();

	std::vector<CalCluster> clulsters_top_cut;
	std::vector<CalCluster> clulsters_bot_cut;

	if( ( tracks_top.size() >= 1 && tracks_bot.size() >= 1 )){
		for(int i = 0; i < n_clusters_top; i++){
			CalCluster cluster = clulsters_top.at(i);
			std::vector<double> positionCluster = cluster.getPosition();

			for(int j = 0; j < tracks_top.size(); j++){
				Track track = tracks_top.at(j);
				std::vector<double> positionAtEcal = track.getPositionAtEcal();
				histos->Fill2DHisto("trackX_vs_ClusterX_top_hh", positionCluster[0], positionAtEcal[0], weight);
				histos->Fill2DHisto("trackY_vs_ClusterY_top_hh", positionCluster[1], positionAtEcal[1], weight);

				if (positionAtEcal[0]< func_top_topCutX->Eval(positionCluster[0])
						&& positionAtEcal[0] > func_top_botCutX->Eval(positionCluster[0])
						&& positionAtEcal[1] < func_top_topCutY->Eval(positionCluster[1])
						&& positionAtEcal[1] > func_top_botCutY->Eval(positionCluster[1])) {
					clulsters_top_cut.push_back(cluster);
					break;
				}
			}
		}

		for(int i = 0; i < n_clusters_bot; i++){
			CalCluster cluster = clulsters_bot.at(i);
			std::vector<double> positionCluster = cluster.getPosition();

			for(int j = 0; j < tracks_bot.size(); j++){
				Track track = tracks_bot.at(j);
				std::vector<double> positionAtEcal = track.getPositionAtEcal();
				histos->Fill2DHisto("trackX_vs_ClusterX_bot_hh", positionCluster[0], positionAtEcal[0], weight);
				histos->Fill2DHisto("trackY_vs_ClusterY_bot_hh", positionCluster[1], positionAtEcal[1], weight);

				if (positionAtEcal[0]< func_bot_topCutX->Eval(positionCluster[0])
						&& positionAtEcal[0] > func_bot_botCutX->Eval(positionCluster[0])
						&& positionAtEcal[1] < func_bot_topCutY->Eval(positionCluster[1])
						&& positionAtEcal[1] > func_bot_botCutY->Eval(positionCluster[1])) {
					clulsters_bot_cut.push_back(cluster);
					break;
				}
			}
		}
	}

	int n_clusters_top_cut = clulsters_top_cut.size();
	int n_clusters_bot_cut = clulsters_bot_cut.size();

	int flag = false;

	for(int i = 0; i < n_clusters_top_cut; i++){
		CalCluster cluster = clulsters_top_cut.at(i);
		if(cluster.getEnergy() >= CLUSTERENERGYTHRESHOLD) flag = true;
	}

	for(int i = 0; i < n_clusters_bot_cut; i++){
		CalCluster cluster = clulsters_bot_cut.at(i);
		if(cluster.getEnergy() >= CLUSTERENERGYTHRESHOLD) flag = true;
	}

	int flag_analyzable_event = false;
	int flag_triggered_analyzable_event = false;

	if( ( tracks_top.size() >= 1 && tracks_bot.size() >= 1 ) && (n_clusters_top_cut >=1 || n_clusters_bot_cut >= 1) && flag){
		flag_analyzable_event = true;

		for(int i = 0; i < n_clusters_top_cut; i++){
			CalCluster cluster = clulsters_top_cut.at(i);

			std::vector<double> positionCluster = cluster.getPosition();
			histos->Fill2DHisto("xy_clusters_analyzable_events_hh",positionCluster[0], positionCluster[1], weight);

			CalHit* seed = (CalHit*)cluster.getSeed();
			histos->Fill1DHisto("seed_energy_cluster_analyzable_events_h", seed->getEnergy(), weight);
			histos->Fill1DHisto("energy_cluster_analyzable_events_h", cluster.getEnergy(), weight);
			histos->Fill1DHisto("n_hits_cluster_analyzable_events_h", cluster.getNHits(), weight);
			histos->Fill2DHisto("energy_vs_n_hits_cluster_analyzable_events_hh", cluster.getNHits(), cluster.getEnergy(), weight);

			int ix = seed -> getCrystalIndices()[0];
			if(ix < 0) ix++;
			int iy = seed -> getCrystalIndices()[1];

			histos->Fill1DHisto("n_clusters_xAxis_analyzable_events_h", ix, weight);
			histos->Fill2DHisto("xy_indices_clusters_analyzable_events_hh",ix, iy, weight);
			histos->Fill2DHisto("energy_vs_ix_clusters_analyzable_events_hh",
					ix, cluster.getEnergy(), weight);
			histos->Fill2DHisto("energy_vs_iy_clusters_analyzable_events_hh",
					iy, cluster.getEnergy(), weight);

			if (cluster.getEnergy() <= CLUSTERENERGYMAX
					&& cluster.getEnergy() >= CLUSTERENERGYMIN
					&& ix <= CLUSTERXMAX && iy >= CLUSTERYMIN
					&& iy <= CLUSTERYMAX
					&& cluster.getEnergy() <= func_pde_moller->Eval(ix))
				flag_triggered_analyzable_event = true;
		}

		for(int i = 0; i < n_clusters_bot_cut; i++){
			CalCluster cluster = clulsters_bot_cut.at(i);

			std::vector<double> positionCluster = cluster.getPosition();
			histos->Fill2DHisto("xy_clusters_analyzable_events_hh",positionCluster[0], positionCluster[1], weight);

			CalHit* seed = (CalHit*)cluster.getSeed();
			histos->Fill1DHisto("seed_energy_cluster_analyzable_events_h", seed->getEnergy(), weight);
			histos->Fill1DHisto("energy_cluster_analyzable_events_h", cluster.getEnergy(), weight);
			histos->Fill1DHisto("n_hits_cluster_analyzable_events_h", cluster.getNHits(), weight);
			histos->Fill2DHisto("energy_vs_n_hits_cluster_analyzable_events_hh", cluster.getNHits(), cluster.getEnergy(), weight);

			int ix = seed -> getCrystalIndices()[0];
			if(ix < 0) ix++;
			int iy = seed -> getCrystalIndices()[1];

			histos->Fill1DHisto("n_clusters_xAxis_analyzable_events_h", ix, weight);
			histos->Fill2DHisto("xy_indices_clusters_analyzable_events_hh",ix, iy, weight);
			histos->Fill2DHisto("energy_vs_ix_clusters_analyzable_events_hh", ix, cluster.getEnergy(), weight);
			histos->Fill2DHisto("energy_vs_iy_clusters_analyzable_events_hh", iy, cluster.getEnergy(), weight);

			if(cluster.getEnergy() < CLUSTERENERGYMAX && cluster.getEnergy() > CLUSTERENERGYMIN) flag_triggered_analyzable_event = true;
		}

		for(int i = 0; i < n_tracks_top; i++) {
			Track trackTop = tracks_top.at(i);
			std::vector<double> positionAtEcalTop = trackTop.getPositionAtEcal();

			for(int j = 0; j < n_tracks_bot; j++) {
				Track trackBot = tracks_bot.at(i);
				std::vector<double> positionAtEcalBot = trackBot.getPositionAtEcal();

				histos->Fill2DHisto("xy_positionAtEcal_tracks_analyzable_events_hh",positionAtEcalTop[0], positionAtEcalTop[1], weight);
				histos->Fill2DHisto("xy_positionAtEcal_tracks_analyzable_events_hh",positionAtEcalBot[0], positionAtEcalBot[1], weight);

			}
		}
	}

	bool flag_triggered_analyzable_event_and_pass_kinematic_cuts = false;

    for(int i = 0; i < n_vtxs; i++){
        Vertex* vtx = vtxs_->at(i);

        int n_entries = vtx->getParticles()->GetEntries();
        if(n_entries != 2) {
        	std::cout << "Warning: entries of Moller vertex is not 2." << std::endl;
        	return false;
        }

        double invariant_mass = vtx->getInvMass();
		histos->Fill1DHisto("invariant_mass_vertex_h", invariant_mass, weight);


        Particle* particleTop = (Particle*)vtx->getParticles()->At(0);
        Particle* particleBot = (Particle*)vtx->getParticles()->At(1);

        std::vector<double> momTop =  particleTop->getMomentum();
        std::vector<double> momBot =  particleBot->getMomentum();

		histos->Fill2DHisto("px_vs_py_vertex_hh", momTop[0], momTop[1], weight);
		histos->Fill2DHisto("px_vs_py_vertex_hh", momBot[0], momBot[1], weight);

		double momTopX_before_beam_rotation = momTop[0] * cos(ROTATIONANGLEAROUNDY) - momTop[2] * sin(ROTATIONANGLEAROUNDY);
		double momTopZ_before_beam_rotation = momTop[0] * sin(ROTATIONANGLEAROUNDY) + momTop[2] * cos(ROTATIONANGLEAROUNDY);
		double momTopY_before_beam_rotation = momTop[1];

		double momBotX_before_beam_rotation = momBot[0] * cos(ROTATIONANGLEAROUNDY) - momBot[2] * sin(ROTATIONANGLEAROUNDY);
		double momBotZ_before_beam_rotation = momBot[0] * sin(ROTATIONANGLEAROUNDY) + momBot[2] * cos(ROTATIONANGLEAROUNDY);
		double momBotY_before_beam_rotation = momBot[1];

		TLorentzVector* lorentzVectorTop_beam_rotation = new TLorentzVector();
		lorentzVectorTop_beam_rotation->SetXYZM(momTopX_before_beam_rotation, momTopY_before_beam_rotation, momTopZ_before_beam_rotation, ELECTRONMASS);

		TLorentzVector* lorentzVectorBot_beam_rotation = new TLorentzVector();
		lorentzVectorBot_beam_rotation->SetXYZM(momBotX_before_beam_rotation, momBotY_before_beam_rotation, momBotZ_before_beam_rotation, ELECTRONMASS);

		double energy_top = lorentzVectorTop_beam_rotation->E();
		double energy_bot = lorentzVectorBot_beam_rotation->E();

		double thate_top_before_rotation = lorentzVectorTop_beam_rotation->Theta();
		double thate_bot_before_rotation = lorentzVectorBot_beam_rotation->Theta();

		double energy_calcuated_top = func_E_vs_theta_before_roation->Eval(thate_top_before_rotation);
		double energy_calcuated_bot = func_E_vs_theta_before_roation->Eval(thate_bot_before_rotation);

		double theta_bot_calculated_before_rotation = func_theta1_vs_theta2_before_roation->Eval(thate_top_before_rotation);

		double energy_diff_top = energy_top - energy_calcuated_top;
		double energy_diff_bot = energy_bot - energy_calcuated_bot;
		double theta_diff = thate_bot_before_rotation - theta_bot_calculated_before_rotation;

		histos->Fill2DHisto("energy_vs_theta_track_pair_from_vertex_before_rotation_hh",thate_top_before_rotation, energy_top, weight);
		histos->Fill2DHisto("energy_vs_theta_track_pair_from_vertex_before_rotation_hh",thate_bot_before_rotation, energy_bot, weight);

		histos->Fill2DHisto("thetaTop_vs_thetaBot_track_pair_from_vertex_before_rotation_hh",thate_bot_before_rotation, thate_top_before_rotation, weight);

		histos->Fill1DHisto("diff_E_vertex_before_rotation_h", energy_diff_top, weight);
		histos->Fill1DHisto("diff_E_vertex_before_rotation_h", energy_diff_bot, weight);

		histos->Fill1DHisto("diff_theta_vertex_before_rotation_h", theta_diff, weight);

        Track trackTop = particleTop->getTrack();
        Track trackBot = particleBot->getTrack();

        std::vector<double> positionAtEcalTop = trackTop.getPositionAtEcal();
        std::vector<double> positionAtEcalBot = trackBot.getPositionAtEcal();

        if(!isnan(positionAtEcalTop[2]) && !isnan(positionAtEcalBot[2]) ){
        	histos->Fill2DHisto("xy_positionAtEcal_vertices_hh", positionAtEcalTop[0], positionAtEcalTop[1], weight);
        	histos->Fill2DHisto("xy_positionAtEcal_vertices_hh", positionAtEcalBot[0], positionAtEcalBot[1], weight);
        }

        CalCluster clTop = particleTop->getCluster();
        CalCluster clBot = particleBot->getCluster();

    	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_vertex_h", clTop.getEnergy() - energy_top, weight);
    	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_vertex_h", clBot.getEnergy() - energy_bot, weight);

        if(flag_analyzable_event){
			histos->Fill1DHisto("invariant_mass_vertex_analyzable_events_h", invariant_mass, weight);

			histos->Fill2DHisto("px_vs_py_vertex_analyzable_events_hh", momTop[0], momTop[1], weight);
			histos->Fill2DHisto("px_vs_py_vertex_analyzable_events_hh", momBot[0], momBot[1], weight);

    		histos->Fill2DHisto("energy_vs_theta_analyzable_events_before_rotation_hh",thate_top_before_rotation, energy_top, weight);
    		histos->Fill2DHisto("energy_vs_theta_analyzable_events_before_rotation_hh",thate_bot_before_rotation, energy_bot, weight);

    		histos->Fill2DHisto("thetaTop_vs_thetaBot_analyzable_events_before_rotation_hh",thate_bot_before_rotation, thate_top_before_rotation, weight);

    		histos->Fill1DHisto("diff_E_analyzable_events_before_rotation_h", energy_diff_top, weight);
    		histos->Fill1DHisto("diff_E_analyzable_events_before_rotation_h", energy_diff_bot, weight);

    		histos->Fill1DHisto("diff_theta_analyzable_events_before_rotation_h", theta_diff, weight);

        	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_analyzable_events_h", clTop.getEnergy() - energy_top, weight);
        	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_analyzable_events_h", clBot.getEnergy() - energy_bot, weight);

        	if( (energy_diff_top < DIFFENERGYMIN || energy_diff_top > DIFFENERGYMAX )
            		|| (energy_diff_bot < DIFFENERGYMIN || energy_diff_bot > DIFFENERGYMAX)
    				|| (theta_diff < DIFFTHETAMIN || theta_diff > DIFFTHETAMAX))
        		histos->Fill1DHisto("invariant_mass_vertex_analyzable_events_out_of_kinematic_cuts_h", invariant_mass, weight);
        }

        if(flag_triggered_analyzable_event){
			histos->Fill1DHisto("invariant_mass_vertex_triggered_analyzable_events_h", invariant_mass, weight);

			histos->Fill2DHisto("px_vs_py_vertex_triggered_analyzable_events_hh", momTop[0], momTop[1], weight);
			histos->Fill2DHisto("px_vs_py_vertex_triggered_analyzable_events_hh", momBot[0], momBot[1], weight);

    		histos->Fill2DHisto("energy_vs_theta_triggered_analyzable_events_before_rotation_hh",thate_top_before_rotation, energy_top, weight);
    		histos->Fill2DHisto("energy_vs_theta_triggered_analyzable_events_before_rotation_hh",thate_bot_before_rotation, energy_bot, weight);

    		histos->Fill2DHisto("thetaTop_vs_thetaBot_triggered_analyzable_events_before_rotation_hh",thate_bot_before_rotation, thate_top_before_rotation, weight);

    		histos->Fill1DHisto("diff_E_triggered_analyzable_events_before_rotation_h", energy_diff_top, weight);
    		histos->Fill1DHisto("diff_E_triggered_analyzable_events_before_rotation_h", energy_diff_bot, weight);

    		histos->Fill1DHisto("diff_theta_triggered_analyzable_events_before_rotation_h", theta_diff, weight);

        	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_triggered_analyzable_events_h", clTop.getEnergy() - energy_top, weight);
        	histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_triggered_analyzable_events_h", clBot.getEnergy() - energy_bot, weight);
        }

        if(flag_triggered_analyzable_event && energy_diff_top > DIFFENERGYMIN && energy_diff_top < DIFFENERGYMAX
        		&& energy_diff_bot > DIFFENERGYMIN && energy_diff_bot < DIFFENERGYMAX
				&& theta_diff > DIFFTHETAMIN && theta_diff < DIFFTHETAMAX){
			histos->Fill1DHisto("invariant_mass_vertex_triggered_analyzable_events_with_kinematic_cuts_h", invariant_mass, weight);

			histos->Fill2DHisto("px_vs_py_vertex_triggered_analyzable_events_with_kinematic_cuts_hh", momTop[0], momTop[1], weight);
			histos->Fill2DHisto("px_vs_py_vertex_triggered_analyzable_events_with_kinematic_cuts_hh", momBot[0], momBot[1], weight);

			histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_triggered_analyzable_events_with_kinematic_cuts_h", clTop.getEnergy() - energy_top, weight);
			histos->Fill1DHisto("diff_energy_between_recon_clulster_and_track_energy_triggered_analyzable_events_with_kinematic_cuts_h", clBot.getEnergy() - energy_bot, weight);

			flag_triggered_analyzable_event_and_pass_kinematic_cuts = true;

        }
    }

    if(flag_triggered_analyzable_event_and_pass_kinematic_cuts == true){
		for(int i = 0; i < n_clusters_top_cut; i++){
			CalCluster cluster = clulsters_top_cut.at(i);

			std::vector<double> positionCluster = cluster.getPosition();
			histos->Fill2DHisto("xy_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh",positionCluster[0], positionCluster[1], weight);

			CalHit* seed = (CalHit*)cluster.getSeed();
			histos->Fill1DHisto("seed_energy_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", seed->getEnergy(), weight);
			histos->Fill1DHisto("energy_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", cluster.getEnergy(), weight);
			histos->Fill1DHisto("n_hits_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", cluster.getNHits(), weight);
			histos->Fill2DHisto("energy_vs_n_hits_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_hh", cluster.getNHits(), cluster.getEnergy(), weight);

			int ix = seed -> getCrystalIndices()[0];
			if(ix < 0) ix++;
			int iy = seed -> getCrystalIndices()[1];

			histos->Fill2DHisto("xy_indices_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh",ix, iy, weight);
			histos->Fill2DHisto("energy_vs_ix_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh", ix, cluster.getEnergy(), weight);
			histos->Fill2DHisto("energy_vs_iy_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh", iy, cluster.getEnergy(), weight);

		}

		for(int i = 0; i < n_clusters_bot_cut; i++){
			CalCluster cluster = clulsters_bot_cut.at(i);

			std::vector<double> positionCluster = cluster.getPosition();
			histos->Fill2DHisto("xy_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh",positionCluster[0], positionCluster[1], weight);

			CalHit* seed = (CalHit*)cluster.getSeed();
			histos->Fill1DHisto("seed_energy_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", seed->getEnergy(), weight);
			histos->Fill1DHisto("energy_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", cluster.getEnergy(), weight);
			histos->Fill1DHisto("n_hits_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_h", cluster.getNHits(), weight);
			histos->Fill2DHisto("energy_vs_n_hits_cluster_triggered_analyzable_event_and_pass_kinematic_cuts_hh", cluster.getNHits(), cluster.getEnergy(), weight);

			int ix = seed -> getCrystalIndices()[0];
			if(ix < 0) ix++;
			int iy = seed -> getCrystalIndices()[1];

			histos->Fill2DHisto("xy_indices_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh",ix, iy, weight);
			histos->Fill2DHisto("energy_vs_ix_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh", ix, cluster.getEnergy(), weight);
			histos->Fill2DHisto("energy_vs_iy_clusters_triggered_analyzable_event_and_pass_kinematic_cuts_hh", iy, cluster.getEnergy(), weight);
		}

    }

    bool flag_triggered = false;
	for(int i = 0; i < n_cl; i++){
		CalCluster* cluster = gtpClusters_->at(i);
		double energy = cluster->getEnergy();

		CalHit* seed = (CalHit*)cluster->getSeed();

		int ix = seed -> getCrystalIndices()[0];
		if(ix < 0) ix++;
		int iy = seed -> getCrystalIndices()[1];

		if (energy <= CLUSTERENERGYMAX
				&& energy >= CLUSTERENERGYMIN
				&& ix <= CLUSTERXMAX && iy >= CLUSTERYMIN
				&& iy <= CLUSTERYMAX
				&& energy <= func_pde_moller->Eval(ix))
			flag_triggered = true;
	}

	if(flag_triggered){
		histos->Fill1DHisto("n_tracks_triggered_h", n_tracks, weight);
		histos->Fill1DHisto("n_clusters_triggered_h", n_cl, weight);
		histos->Fill1DHisto("n_vtxs_triggered_h", n_vtxs, weight);

		histos->Fill2DHisto("n_clusters_vs_n_tracks_triggered_hh", n_tracks, n_cl, weight);
		histos->Fill2DHisto("n_clusters_vs_n_vtxs_triggered_hh", n_vtxs, n_cl, weight);
		histos->Fill2DHisto("n_tracks_vs_n_vtxs_triggered_hh", n_vtxs, n_tracks, weight);
	}

    return true;
}

void TriggerParametersExtractionMollerAnaProcessor::finalize() {
    histos->saveHistos(outF_, anaName_.c_str());
    delete histos;
    histos = nullptr;

}

DECLARE_PROCESSOR(TriggerParametersExtractionMollerAnaProcessor);