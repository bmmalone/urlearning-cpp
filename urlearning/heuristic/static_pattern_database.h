/* 
 * File:   static_pattern_database.h
 * Author: malone
 *
 * Created on October 22, 2012, 9:38 PM
 */

#ifndef STATIC_PATTERN_DATABASE_H
#define	STATIC_PATTERN_DATABASE_H

#include <vector>

#include "urlearning/base/typedefs.h"

#include "heuristic.h"

namespace heuristics {
class StaticPatternDatabase : public Heuristic {
public:
    StaticPatternDatabase() {}
    StaticPatternDatabase(int variableCount, int pdCount, bool isRandom);
    StaticPatternDatabase(int variableCount, int pdCount, bool isRandom, varset &ancestors, varset &scc);
    ~StaticPatternDatabase();
    
    void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);
    void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, std::vector<varset> variableSets, varset &ancestors, varset &scc);

    int size();
    float h(const varset &variables, bool &complete);
    void print() {}
    void printStatistics() {
        printf("static pattern database does not collect statistics\n");
    }
    int patternDatabaseCount;

private:
    int variableCount;
    bool isRandom;
    
    varset ancestors;
    varset scc;

    std::vector<varset> variableSets;
    std::vector< FloatMap >patternDatabases;

    void createPatternDatabase(const varset &allVariables, const varset &variableSet, const int variableSetSize, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, FloatMap &pd);
    void expand(varset &subnetwork, const float g, const varset &variableSet, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, FloatMap &currentLayer);
};

}
#endif	/* STATIC_PATTERN_DATABASE_H */

