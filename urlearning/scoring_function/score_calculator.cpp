/* 
 * File:   score_calculator.cpp
 * Author: malone
 * 
 * Created on November 24, 2012, 5:46 PM
 */

#include <algorithm>
#include <math.h>

#include <boost/dynamic_bitset.hpp>
#include <limits>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "score_calculator.h"
#include "scoring_function.h"

scoring::ScoreCalculator::ScoreCalculator(scoring::ScoringFunction *scoringFunction, int maxParents, int variableCount, int runningTime, scoring::Constraints *constraints) {
    this->scoringFunction = scoringFunction;
    this->maxParents = maxParents;
    this->variableCount = variableCount;
    this->runningTime = runningTime;
    this->constraints = constraints;
}

void scoring::ScoreCalculator::timeout(const boost::system::error_code& /*e*/) {
    printf("Out of time\n");
    outOfTime = true;
}

void scoring::ScoreCalculator::calculateScores(int variable, FloatMap &cache) {
    this->outOfTime = false;
    
    //io.reset();
    //boost::asio::deadline_timer *t;
    boost::asio::io_service io_t;
    t = new boost::asio::deadline_timer(io_t);
    if (runningTime > 0) {
        printf("I am using a timer in the calculation function.\n");
        t->expires_from_now(boost::posix_time::seconds(runningTime));
        t->async_wait(boost::bind(&scoring::ScoreCalculator::timeout, this, boost::asio::placeholders::error));
        boost::thread workerThread(boost::bind(&scoring::ScoreCalculator::calculateScores_internal, this, variable, boost::ref(cache)));
        io_t.run();
        workerThread.join();
        io_t.stop();
//        t.cancel();
    } else {
        calculateScores_internal(variable, cache);
    }
}

void scoring::ScoreCalculator::calculateScores_internal(int variable, FloatMap &cache) {
    // calculate the initial score
    VARSET_NEW(empty, variableCount);
    float score = scoringFunction->calculateScore(variable, empty, cache);
    
    if (score < 1) {
        cache[empty] = score;
    }
    
    int prunedCount = 0;

    for (int layer = 1; layer <= maxParents && !outOfTime; layer++) {
#ifdef DEBUG
        printf("layer: %d, prunedCount: %d\n", layer, prunedCount);
#endif

        VARSET_NEW(variables, variableCount);
        for (int i = 0; i < layer; i++) {
            VARSET_SET(variables, i);
        }

        VARSET_NEW(max, variableCount);
        VARSET_SET(max, variableCount);
        
        while (VARSET_LESS_THAN(variables, max) && !outOfTime) {
            if (!VARSET_GET(variables, variable)) {
                score = scoringFunction->calculateScore(variable, variables, cache);
                
#ifdef VERBOSE_DEBUG
                printf("%d, %" PRIu64 ": %f\n", variable, variables, score);
#endif
                
                
                // only store the score if it was not pruned
                if (score < 0) {
                    cache[variables] = score;
                } else {
                    prunedCount++;
                }
            }

            // find the next combination
            variables = nextPermutation(variables);
        }
        
        if (!outOfTime) highestCompletedLayer = layer;
    }
    
//    io.stop();
    t->cancel();
//    io.reset();

#ifdef VERBOSE_DEBUG
    printf("Scores\n");
    for (auto it = cache.begin(); it != cache.end(); it++) {
        printf("%d, %d: %f\n", variable, (*it).first, (*it).second);
    }
#endif
}

struct compareSecond {

    bool operator()(std::pair<varset, float> lhs, std::pair<varset, float> rhs) const {
        float val = lhs.second - rhs.second;
        
        if (fabs(val) > 2 * std::numeric_limits<float>::epsilon()) {
            return val > 0;
        }
        
        return lhs.first < rhs.first;
    }
} comparator;

void scoring::ScoreCalculator::prune(FloatMap &cache) {
    std::vector< std::pair<varset, float> > pairs;
    for (auto pair = cache.begin();
            pair != cache.end();
            pair++) {
        pairs.push_back(*pair);
    }
    
    std::sort(pairs.begin(), pairs.end(), comparator);

#ifdef VERBOSE_DEBUG
    printf("Sorted Scores\n");
    for (auto it = pairs.begin(); it != pairs.end(); it++) {
        printf("%d: %f\n", (*it).first, (*it).second);
    }
#endif
    
    // keep track of the ones that have been pruned
    boost::dynamic_bitset<> prunedSets(pairs.size());
    for (int i = 0; i < pairs.size(); i++) {
        if (prunedSets.test(i)) {
            continue;
        }

        varset pi = pairs[i].first;
        
        // make sure this variable set is not in an incomplete last layer
        if (cardinality(pi) > highestCompletedLayer) {
            prunedSets.set(i);
            continue;
        }

        for (int j = i + 1; j < pairs.size(); j++) {
            if (prunedSets.test(j)) {
                continue;
            }

            // check if parents[i] is a subset of parents[j]
            varset pj = pairs[j].first;
            
            if (VARSET_IS_SUBSET_OF(pi, pj)) {
                // then we can prune pj
                prunedSets.set(j);
                cache.erase(pj);
            }
        }
    }
}
