/* 
 * File:   scoring_function.h
 * Author: malone
 *
 * Created on November 24, 2012, 5:48 PM
 */

#ifndef SCORING_FUNCTION_H
#define	SCORING_FUNCTION_H

#include "urlearning/base/typedefs.h"
#include "urlearning/score_cache/score_cache.h"

namespace scoring {

    class ScoringFunction {
        
    public:
        virtual float calculateScore(int variable, varset parents, FloatMap &cache) = 0;
        
    };
}


#endif	/* SCORING_FUNCTION_H */

