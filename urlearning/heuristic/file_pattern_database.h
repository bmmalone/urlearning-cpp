/* 
 * File:   file_pattern_database.h
 * Author: bmmalone
 *
 * Created on May 28, 2014, 11:07 AM
 */

#ifndef FILE_PATTERN_DATABASE_H
#define	FILE_PATTERN_DATABASE_H

#include <string>
#include <vector>
#include "heuristic.h"

namespace heuristics {
    class StaticPatternDatabase;
    class DynamicPatternDatabase;
    
    class FilePatternDatabase : public Heuristic {
    public:

        FilePatternDatabase() {
        }
        FilePatternDatabase(const int variableCount, std::string filename, varset &ancestors, varset &scc);
        ~FilePatternDatabase();

        void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);

        int size();
        float h(const varset &variables, bool &complete);
        void print();
        void printStatistics();
        
        Heuristic *getHeuristic(int index) { return pds[index]; }
        int getCount() { return pds.size(); }
        
        /**
         * These are statistics on how many times each database was used.  It is assumed
         * that they will be updated correctly, so they are public.
         */
        std::vector<int> uses;
        
    private:
        heuristics::StaticPatternDatabase *getStaticPatternDatabase(std::vector<std::string> &tokens, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);
        heuristics::DynamicPatternDatabase *getDynamicPatternDatabase(std::vector<std::string> &tokens, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);
        
        std::vector<Heuristic*> pds;
    
        varset ancestors;
        varset scc;
        
        int variableCount;
        std::string filename;
    };
}


#endif	/* FILE_PATTERN_DATABASE_H */

