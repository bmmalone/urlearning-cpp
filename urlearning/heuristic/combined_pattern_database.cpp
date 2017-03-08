#include "combined_pattern_database.h"
#include "static_pattern_database.h"
#include "dynamic_pattern_database.h"

#include "heuristic_creator.h"

heuristics::CombinedPatternDatabase::CombinedPatternDatabase(const int variableCount, const int pdCount, const int maxSize, int staticCount) {
    this->variableCount = variableCount;
    this->pdCount = pdCount;
    this->maxSize = maxSize;
    this->staticCount = staticCount;
    dynamicUses = 0;
}

void heuristics::CombinedPatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*>& spgs) {
    // use the deterministic pattern database first
    if (staticCount > 0) {
        Heuristic *spd = new heuristics::StaticPatternDatabase(spgs.size(), pdCount, false);
        spd->initialize(spgs);
        spds.push_back(spd);
        staticUses.push_back(0);
    }
    
    // the rest of the static pds are random
    for (int i = 1; i < staticCount; i++) {
        Heuristic *spd = new heuristics::StaticPatternDatabase(spgs.size(), pdCount, true);
        spd->initialize(spgs);
        spds.push_back(spd);
        staticUses.push_back(0);
    }

    dpd = new heuristics::DynamicPatternDatabase(spgs.size(), maxSize, false);
    dpd->initialize(spgs);
    dynamicUses = 0;
}

float heuristics::CombinedPatternDatabase::h(const varset& variables, bool &complete) {
    float max = dpd->h(variables, complete);
    int maxIndex = -1;

    int curIndex = 0;
    for (auto spd = spds.begin(); spd != spds.end(); spd++) {
        float s = (*spd)->h(variables, complete);
        
        if (s > max) {
            max = s;
            maxIndex = curIndex;
        }
        
        if (complete) {
            return s;
        }
        
        curIndex++;
    }
    
    if (maxIndex == -1) {
        dynamicUses++;
    } else {
        staticUses[maxIndex]++;
    }
    return max;
}

void heuristics::CombinedPatternDatabase::print() {
    printf("*** Static pattern database ***\n");
    //    spd->print();

    printf("\n*** Dynamic pattern database ***\n");
    dpd->print();
}


void heuristics::CombinedPatternDatabase::printStatistics() {
    printf("uses,random_pattern_database\n");
    printf("statistics,-1,%d\n", dynamicUses);
    for (int i = 0; i < staticUses.size(); i++) {
        printf("statistics,%d,%d\n", i, staticUses[i]);
    }
}

int heuristics::CombinedPatternDatabase::size() {
    int size = dpd->size();

    for (auto spd = spds.begin(); spd != spds.end(); spd++) {
        size += (*spd)->size();
    }

    return size;
}