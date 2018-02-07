#include <HypothesisSelection.hpp>

namespace hypothesis_selection{

	HypothesisSelection::HypothesisSelection(){

	}

	HypothesisSelection::~HypothesisSelection(){

	}

	void LCPSelection::selectBestPoses(scene_cfg::SceneCfg *pCfg){

		for(int ii=0; ii<pCfg->numObjects; ii++){
			pCfg->pSceneObjects[ii]->objPose = pCfg->pSceneObjects[ii]->hypotheses->bestHypothesis.first;
		}
	}
}