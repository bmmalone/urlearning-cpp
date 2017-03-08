#include "sparse_parent_bitwise.h"

#include <algorithm>
#include <iostream>
#include <limits>

inline bool cmp(const std::pair<varset, float>  &p1, const std::pair<varset, float> &p2) {
    return p1.second < p2.second;
}

bestscorecalculators::SparseParentBitwise::SparseParentBitwise(const int variable, const int variableCount) {
    this->variable = variable;
    this->variableCount = variableCount;
}

bestscorecalculators::SparseParentBitwise::~SparseParentBitwise() {
//    BITSET_FREE(getScoreScratch_);
//    
//    for(bitset u : usedParents_) {
//        BITSET_FREE(u);
//    }
}

void bestscorecalculators::SparseParentBitwise::initialize(const scoring::ScoreCache& scoreCache) {
    
    // populate the score and parents arrays
#ifdef DEBUG
    printf("Creating spg\n");
#endif
    
    std::vector<std::pair<varset, float> > spg (scoreCache.getCache(variable)->begin(), scoreCache.getCache(variable)->end());
    
#ifdef DEBUG
    printf("Sorting spg\n");
#endif
    std::sort(spg.begin(), spg.end(), cmp);
    
    
#ifdef DEBUG
    printf("Pushing parents and scores\n");
#endif
    for(int i = 0; i < spg.size(); ++i) {
        parents.push_back(spg[i].first);
        scores.push_back(spg[i].second);
    }
    
    
#ifdef DEBUG
    printf("Creating used parent bitsets\n");
#endif
    for(int i = 0; i < scoreCache.getVariableCount(); i++) {
        usedParents.push_back(BITSET_NEW(parents.size()));
    }
    
    // populate the used parents arrays
    
#ifdef DEBUG
    printf("Populating used parents\n");
#endif
    for(int i = 0; i < parents.size(); i++) {
        for(int parent = 0; parent < variableCount; parent++) {
            if (VARSET_GET(parents[i], parent)) {
                BITSET_SET(usedParents[parent], i);
            }
        }
    }
    
    // now get the inverse of the possible parents sets
    // later, we will find the remaining possible parents for
    // a particular variable, X, after adding this variable, l,
    // as a leaf by doing :
    // remainingParentSets(X) & ~possibleParents(X, l)    
#ifdef DEBUG
    printf("Flipping used parents\n");
#endif
    for (int parent = 0; parent < usedParents.size(); parent++) {
        BITSET_FLIP_ALL(usedParents[parent]);
    }
    
#ifdef DEBUG
    printf("Creating scratch bitset\n");
#endif
    getScoreScratch = BITSET_NEW(parents.size());
#ifdef DEBUG
    printf("Setting scratch bitset\n");
#endif
    BITSET_SET_ALL(getScoreScratch);
}

float bestscorecalculators::SparseParentBitwise::getScore(varset &pars) {
    
    BITSET_SET_ALL(getScoreScratch);
    
    for(int p = 0; p < variableCount; p++) {
        // if pars contained p, skip it
        if(VARSET_GET(pars, p)) continue;
        
        // otherwise, remove these parents
        BITSET_AND(getScoreScratch, usedParents[p]);
    }
    
    int fsb = BITSET_FIRST_SET_BIT(getScoreScratch);
    bestIndex = fsb;
    
    if (fsb < 0) {
        return std::numeric_limits<float>::max();
    }
    
    return scores[fsb];
}

float bestscorecalculators::SparseParentBitwise::getScore(int index) {
    if (index < 0) return 0;
    return scores[index];
}

varset &bestscorecalculators::SparseParentBitwise::getParents(int index) {
    if (index < 0) {
        VARSET_NEW(ret, 0);
        return ret;
    }
    return parents[index];
}