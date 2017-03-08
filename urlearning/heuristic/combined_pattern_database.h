/* 
 * File:   combined_pattern_database.h
 * Author: bmmalone
 *
 * Created on August 30, 2013, 10:00 AM
 */

#ifndef COMBINED_PATTERN_DATABASE_H
#define	COMBINED_PATTERN_DATABASE_H

#include <vector>

#include "heuristic.h"

namespace heuristics {
    
    class StaticPatternDatabase;
    class DynamicPatternDatabase;

    class CombinedPatternDatabase : public Heuristic {
    public:

        CombinedPatternDatabase() {
        }
        CombinedPatternDatabase(const int variableCount, const int pdCount, const int maxSize, int staticCount);
        ~CombinedPatternDatabase();

        void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);

        int size();
        float h(const varset &variables, bool &complete);
        void print();
        void printStatistics();
        
        /**
         * These are statistics on how many times each database was used.  It is assumed
         * that they will be updated correctly, so they are public.
         */
        std::vector<int> staticUses;
        int dynamicUses;
        
    private:
        std::vector<Heuristic*> spds;
        Heuristic *dpd;
        
        int variableCount;
        int pdCount;
        int maxSize;
        int staticCount;
    };
}


#endif	/* COMBINED_PATTERN_DATABASE_H */

