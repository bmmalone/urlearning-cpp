/* 
 * File:   mdl_score_calculator.cpp
 * Author: malone
 * 
 * Created on November 23, 2012, 9:15 PM
 */

#include "bic_scoring_function.h"
#include "log_likelihood_calculator.h"

scoring::BICScoringFunction::BICScoringFunction(datastructures::BayesianNetwork& network, datastructures::RecordFile &recordFile, LogLikelihoodCalculator *llc, scoring::Constraints *constraints, bool enableDeCamposPruning) {
    this->network = network;
    this->baseComplexityPenalty = log(recordFile.size()) / 2;
    this->constraints = constraints;
    
    this->llc = llc;
    this->enableDeCamposPruning = enableDeCamposPruning;
}

float scoring::BICScoringFunction::t(int variable, varset parents) {
    float penalty = network.getCardinality(variable) - 1;

    for (int pa = 0; pa < network.size(); pa++) {
        if (VARSET_GET(parents, pa)) {
            penalty *= network.getCardinality(pa);
        }
    }

    return penalty;
}

float scoring::BICScoringFunction::calculateScore(int variable, varset parents, FloatMap &cache) {
    // check if this violates the constraints
    if (constraints != NULL && !constraints->satisfiesConstraints(variable, parents)) {
        invalidParents.insert(parents);
        return 1;
    }

    // check for pruning
    float tVal = t(variable, parents);

    if (enableDeCamposPruning) {
        for (int x = 0; x < network.size(); x++) {
            if (VARSET_GET(parents, x)) {
                VARSET_CLEAR(parents, x);

                // check the constraints
                if (invalidParents.size() > 0 && invalidParents.count(parents) > 0) {
                    // we cannot say anything if we skipped this because of constraints
                    VARSET_SET(parents, x);
                    continue;
                }

                auto s = cache.find(parents);

                if (s == cache.end()) {
                    return 0;
                }

                if (s->second + tVal > 0) {
                    return 0;
                }

                VARSET_SET(parents, x);
            }
        }
    }

    
    float score = llc->calculate(variable, parents);

    // structure penalty
    score -= tVal * baseComplexityPenalty;

    return score;
}
