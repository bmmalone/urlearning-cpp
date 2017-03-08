/* 
 * File:   static_pattern_database.cpp
 * Author: malone
 * 
 * Created on October 22, 2012, 9:38 PM
 */

#include <iostream>
#include <ostream>

#include <math.h>

#include "static_pattern_database.h"

heuristics::StaticPatternDatabase::StaticPatternDatabase(int variableCount, int patternDatabaseCount, bool isRandom) {
    this->variableCount = variableCount;
    this->patternDatabaseCount = patternDatabaseCount;
    this->isRandom = isRandom;
    VARSET_CLEAR_ALL(ancestors);
    VARSET_SET_ALL(scc, variableCount);
}

heuristics::StaticPatternDatabase::StaticPatternDatabase(int variableCount, int patternDatabaseCount, bool isRandom, varset &ancestors, varset &scc) {
    this->variableCount = variableCount;
    this->patternDatabaseCount = patternDatabaseCount;
    this->isRandom = isRandom;
    this->ancestors = ancestors;
    this->scc = scc;
}

heuristics::StaticPatternDatabase::~StaticPatternDatabase() {
    
}

inline int getRandomVariable(varset remainingVariables, int remainingCount, int variableCount) {
    int r = rand() % remainingCount;

    int counter = 0;
    for (int x = 0; x < variableCount; x++) {
        if (VARSET_GET(remainingVariables, x)) {
            if (counter == r) {
                return x;
            }
            counter++;
        }
    }

    return -1;
}


void heuristics::StaticPatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, std::vector<varset> variableSets, varset &ancestors, varset &scc) {
    this->variableCount = spgs.size();
    this->variableSets = variableSets;
    this->isRandom = false;
    this->patternDatabaseCount = variableSets.size();
    this->ancestors = ancestors;
    this->scc = scc;
    
    VARSET_NEW(allVariables, variableCount);
    allVariables = VARSET_OR(allVariables, scc);
    //VARSET_SET_ALL(allVariables, variableCount);
    
    for (int pd_i = 0; pd_i < patternDatabaseCount; ++pd_i) {
        int variableSetSize;
        variableSetSize = cardinality(variableSets[pd_i]);
        
#ifdef DEBUG
        printf("Creating static pattern database: %s\n", varsetToString(variableSets[pd_i]).c_str());
#endif

        FloatMap patternDatabase(pow(2, variableSetSize));
        init_map(patternDatabase);
        patternDatabases.push_back(patternDatabase);

        createPatternDatabase(allVariables, variableSets[pd_i], variableSetSize, spgs, patternDatabases[pd_i]);
    }
}

void heuristics::StaticPatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs) {
    
    // create the variable set bitmasks
    int x = 0;

    VARSET_NEW(allVariables, variableCount);
    allVariables = VARSET_OR(allVariables, scc);
    //VARSET_SET_ALL(allVariables, variableCount);

    VARSET_NEW(remainingVariables, variableCount);
    remainingVariables = VARSET_OR(remainingVariables, scc);
    //VARSET_SET_ALL(remainingVariables, variableCount);

    int remainingCount = cardinality(scc);
    int var = VARSET_FIND_FIRST_SET(scc);
    
    int patternDatabaseSize = ceil(static_cast<float> (remainingCount) / patternDatabaseCount);

    
    for (int pd_i = 0; pd_i < patternDatabaseCount; ++pd_i) {
        VARSET_NEW(empty, variableCount);
        variableSets.push_back(empty);

        int variableSetSize;

        if (isRandom) {
            // create the variable set randomly
            for (variableSetSize = 0; variableSetSize < patternDatabaseSize && remainingCount > 0; variableSetSize++, remainingCount--) {
                x = getRandomVariable(remainingVariables, remainingCount, variableCount);
                VARSET_SET(variableSets[pd_i], x);
                VARSET_CLEAR(remainingVariables, x);
            }
        } else {
            
            for (variableSetSize = 0; variableSetSize < patternDatabaseSize && x < remainingCount; variableSetSize++) {
                VARSET_SET(variableSets[pd_i], var);
                var = VARSET_FIND_NEXT_SET(scc, var);
                ++x;
            }
        }

#ifdef DEBUG
        printf("Creating pattern database: %s\n", varsetToString(variableSets[pd_i]).c_str());
#endif

        FloatMap patternDatabase(pow(2, variableSetSize));
        init_map(patternDatabase);
        patternDatabases.push_back(patternDatabase);

        createPatternDatabase(allVariables, variableSets[pd_i], variableSetSize, spgs, patternDatabases[pd_i]);
    }
}

int heuristics::StaticPatternDatabase::size() {
    int size = 0;
    for (int pd_i = 0; pd_i < patternDatabaseCount; pd_i++) {
        size += patternDatabases[pd_i].size();
    }
    return size;
}

float heuristics::StaticPatternDatabase::h(const varset &variables, bool &complete) {
    float h = 0;

    VARSET_NEW(mask, variableCount);
    VARSET_SET_ALL(mask, variableCount);

    VARSET_COPY(VARSET_NOT(variables), remaining);

    // use the mask to make sure "remaining" only has valid variables left
    remaining = VARSET_AND(remaining, mask);

    for (int pd_i = 0; pd_i < patternDatabaseCount; pd_i++) {
        varset vs = VARSET_AND(variableSets[pd_i], remaining);
        
        if (vs == remaining) {
            complete = true;
            return patternDatabases[pd_i][vs];
        }

        h += patternDatabases[pd_i][vs];
    }

    return h;
}

void heuristics::StaticPatternDatabase::createPatternDatabase(const varset &allVariables, const varset &variableSet, const int variableSetSize, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, FloatMap &patternDatabase) {
    FloatMap previousLayer;
    init_map(previousLayer);

    previousLayer[allVariables] = 0;

    // create the pattern database by performing a reverse bfs
    for (int layer = 0; layer <= variableSetSize; layer++) {
        FloatMap currentLayer;
        init_map(currentLayer);

        for (FloatMap::iterator it = previousLayer.begin();
                it != previousLayer.end();
                ++it) {

            varset key = it->first;
            float value = it->second;

            // expand this subnetwork
            expand(key, value, variableSet, spgs, currentLayer);

            // add (the inverse of) these variables to the pd
            varset pattern = VARSET_AND(variableSet, VARSET_NOT(key));
            
            // also remove the ancestors
            pattern = VARSET_AND(pattern, VARSET_NOT(ancestors));
            patternDatabase[pattern] = value;
        }

        previousLayer = currentLayer;
    }

    // add the last layer to the pd (this should only be one node...)
    for (FloatMap::iterator it = previousLayer.begin();
            it != previousLayer.end();
            ++it) {

        varset key = it->first;
        float value = it->second;

        patternDatabase[variableSet & ~key] = value;
    }
}

void heuristics::StaticPatternDatabase::expand(varset &subnetwork, const float g, const varset &variableSet, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, FloatMap &currentLayer) {
    // for each remaining leaf
    for (int leaf = 0; leaf < variableCount; leaf++) {

        // is it still remaining and one of my variables?
        if (!(VARSET_GET(subnetwork, leaf)) || !VARSET_GET(variableSet, leaf)) continue;

        // find the best parents
        VARSET_NEW(parentChoices, variableCount);
        parentChoices = VARSET_OR(subnetwork, ancestors);
        
#ifdef DEBUG
        //printf("Looking for parents for %d, choices: %s\n", leaf, varsetToString(parentChoices).c_str());
#endif
        
        float newG = spgs[leaf]->getScore(parentChoices) + g;

        // duplicate detection
        varset new_subnetwork = varsetClearCopy(subnetwork, leaf);
        float oldG = currentLayer[new_subnetwork];

        if (oldG == 0 || newG < oldG) {
            currentLayer[new_subnetwork] = newG;
        }
    }
}
