/* 
 * File:   mdl_score_calculator.h
 * Author: malone
 *
 * Created on November 23, 2012, 9:15 PM
 */

#ifndef MDL_SCORE_CALCULATOR_H
#define	MDL_SCORE_CALCULATOR_H

#include <stdint.h>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "urlearning/base/typedefs.h"
#include "urlearning/base/bayesian_network.h"

#include "urlearning/ad_tree/ad_tree.h"

#include "scoring_function.h"
#include "log_likelihood_calculator.h"
#include "constraints.h"

namespace scoring {

    class BICScoringFunction : public ScoringFunction {
    public:
        BICScoringFunction(datastructures::BayesianNetwork &network, datastructures::RecordFile &recordFile, LogLikelihoodCalculator *llc, Constraints *constraints, bool enableDeCamposPruning);

        ~BICScoringFunction() {
            // no pointers 
        }

        float calculateScore(int variable, varset parents, FloatMap &cache);

    private:
        float t(int variable, varset parents);
        
        datastructures::BayesianNetwork network;
        Constraints *constraints;
        boost::unordered_set<varset> invalidParents;
        LogLikelihoodCalculator *llc;
        

        float baseComplexityPenalty;
        bool enableDeCamposPruning;
    };

}

#endif	/* MDL_SCORE_CALCULATOR_H */

