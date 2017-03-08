/* 
 * File:   best_score_creator.h
 * Author: bmmalone
 *
 * Created on August 30, 2013, 10:18 AM
 */

#ifndef BEST_SCORE_CREATOR_H
#define	BEST_SCORE_CREATOR_H

#include "score_cache.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "best_score_calculator.h"
#include "sparse_parent_bitwise.h"
#include "sparse_parent_list.h"
#include "sparse_parent_tree.h"

#include "score_cache.h"

namespace bestscorecalculators {
    
    static std::string bestScoreCalculatorString = "The data structure to use for BestScore calculations. [\"list\", \"tree\", \"bitwise\"]";

    inline std::vector<BestScoreCalculator*> create(std::string type, scoring::ScoreCache &cache) {
        std::vector<bestscorecalculators::BestScoreCalculator*> spgs;
        for (int i = 0; i < cache.getVariableCount(); i++) {
            bestscorecalculators::BestScoreCalculator *spg;
            if (type == "tree") {
                spg = new bestscorecalculators::SparseParentTree(i, cache.getVariableCount());
            } else if (type == "bitwise") {
                spg = new bestscorecalculators::SparseParentBitwise(i, cache.getVariableCount());
            } else if (type == "list") {
                spg = new bestscorecalculators::SparseParentList(i, cache.getVariableCount());
            } else {
                throw std::runtime_error("Invalid BestScore calculator type: '" + type + "'.  Valid options are 'tree', 'bitwise' and 'list'.");
            }

            spg->initialize(cache);
            spgs.push_back(spg);
        }
        
        return spgs;
    }
}


#endif	/* BEST_SCORE_CREATOR_H */

