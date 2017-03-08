/* 
 * File:   SparseParentGraph.h
 * Author: malone
 *
 * Created on August 7, 2012, 8:16 PM
 */

#ifndef SPARSEPARENTBITWISE_H
#define	SPARSEPARENTBITWISE_H

#include <vector>
#include <boost/dynamic_bitset.hpp>

#include "best_score_calculator.h"
#include "urlearning/score_cache/score_cache.h"
#include "urlearning/base/typedefs.h"

namespace bestscorecalculators {

    class SparseParentBitwise : public BestScoreCalculator {
    public:
        SparseParentBitwise(const int variable, const int variableCount);
        ~SparseParentBitwise();
        void initialize(const scoring::ScoreCache &scoreCache);

        float getBestScore() const {
            return scores[0];
        }
        
        float getScore(varset &pars);

        varset &getParents() {
            return parents[bestIndex];
        }
        float getScore(int index);
        varset &getParents(int index);

        int size() {
            return parents.size();
        }

        bitset &getUsedParents(int parent) {
            return usedParents[parent];
        }
        
        void print() {
            printf("Sparse Parent Bitwise, variable: %d, size: %d\n", variable, size());
        }

    private:
        int variableCount;
        int variable;
        int bestIndex;
        std::vector<varset> parents;
        std::vector<float> scores;
        std::vector<bitset> usedParents;
        bitset getScoreScratch;
    };

}

#endif	/* SPARSEPARENTBITWISE_H */

