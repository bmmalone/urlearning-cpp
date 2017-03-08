#include "fnml_scoring_function.h"
#include "log_likelihood_calculator.h"

scoring::fNMLScoringFunction::fNMLScoringFunction(datastructures::BayesianNetwork& network, LogLikelihoodCalculator* llc, Constraints* constraints, std::vector< std::vector< float >* >* regret, bool enableDeCamposPruning) {
    this->network = network;
    this->llc = llc;
    this->constraints = constraints;
    this->regret = regret;    
    this->enableDeCamposPruning = enableDeCamposPruning;
}


// for a particular set of parents, this remains the same across different variables *with the same cardinality*
// because the parent instantiation counts are the same
// for efficiency, this could also be integrated into the score calculation

float scoring::fNMLScoringFunction::t(int variableCardinality, boost::unordered_map<uint64_t, int> &paCounts) {
    float t = 0;
    for (auto pc : paCounts) {
        std::vector<float> *rList = regret->at(variableCardinality);
        float val = rList->at(pc.second);
        t += val;
    }

    return t;
}

float scoring::fNMLScoringFunction::calculateScore(int variable, varset parents, FloatMap &cache) {
    // check if this violates the constraints
    if (constraints != NULL && !constraints->satisfiesConstraints(variable, parents)) {
        invalidParents.insert(parents);
        return 1;
    }

    // check for pruning    
    boost::unordered_map<uint64_t, int> paCounts;
    float score = llc->calculate(variable, parents, paCounts);

    int variableCardinality = network.getCardinality(variable);
    float tVal = t(variableCardinality, paCounts);
    
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

                float s = cache[parents];

    #ifdef DEBUG
                //printf("subset s: %f, this tval: %f\n", s, tVal);
    #endif

                if (s == 0 || s + tVal > 0) {
                    return 0;
                }

                VARSET_SET(parents, x);
            }
        }
    }

    // structure penalty
    score -= tVal;

    return score;
}

