/* 
 * File:   bdeu_scoring_function.h
 * Author: bmmalone
 *
 * Created on October 4, 2013, 11:36 AM
 */

#include <vector>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "urlearning/base/typedefs.h"
#include "urlearning/base/bayesian_network.h"

#include "urlearning/ad_tree/ad_tree.h"
#include "urlearning/ad_tree/contingency_table_node.h"

#include "scoring_function.h"
#include "constraints.h"

#ifndef BDEU_SCORING_FUNCTION_H
#define	BDEU_SCORING_FUNCTION_H

namespace scoring {
    
    struct scratch {
        /**
         * The score calculation is for this variable.
         */
        int variable;
        /**
         * Scratch variables for calculating the most recent score across different
         * function calls.
         */
        float score;
        float l_r_i;
        float lg_ij;
        float lg_ijk;
        float a_ij;
        float a_ijk;
        
        /**
         * A cache to store all of the parent sets which were pruned because they
         * violated the constraints. We need to store these because they cannot be
         * used when checking for weak pruning.
         */
        boost::unordered_set<varset> invalidParents;
    };

    class BDeuScoringFunction : public ScoringFunction {
    public:
        BDeuScoringFunction(float ess, datastructures::BayesianNetwork &network, ADTree *adTree, Constraints *constraints, bool enableDeCamposPruning);

        ~BDeuScoringFunction() {
            // no pointers 
        }

        float calculateScore(int variable, varset parents, FloatMap &cache);

    private:
        void lg(varset parents, scratch *s);
        void calculate(ContingencyTableNode *ct, uint64_t base, uint64_t index, boost::unordered_map<uint64_t, int> &paCounts, varset variables, int previousVariable, scratch *s);
        
        /**
         * A reference to the AD-tree used for finding the sufficient statistics.
         */
        ADTree *adTree;
        /**
         * A reference to the Bayesian network (and data specifications) for the
         * dataset.
         */
        datastructures::BayesianNetwork network;
        /**
         * A list of all the constraints on valid scores for this dataset.
         */
        Constraints *constraints;
        /**
         * The equivalent sample size to use in this version of BDeu.
         */
        float ess;
        /**
         * The scratch space for score calculations of each variable.
         */
        std::vector<scratch*> scratchSpace;
        
        bool enableDeCamposPruning;
    };

}

#endif	/* BDEU_SCORING_FUNCTION_H */

