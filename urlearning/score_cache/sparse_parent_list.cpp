#include "sparse_parent_list.h"

#include <algorithm>
#include <iostream>
#include <limits>

inline bool cmp(const std::pair<varset, float>  &p1, const std::pair<varset, float> &p2) {
    return p1.second < p2.second;
}

bestscorecalculators::SparseParentList::SparseParentList(const int variable, const int variableCount) {
    this->variable = variable;
    this->variableCount = variableCount;
}

bestscorecalculators::SparseParentList::~SparseParentList() {
    
}

void bestscorecalculators::SparseParentList::initialize(const scoring::ScoreCache& scoreCache) {
    
    // populate the score and parents arrays
#ifdef DEBUG
    printf("Creating sparse parent list\n");
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
}

float bestscorecalculators::SparseParentList::getScore(varset &pars) {
    
    for (bestIndex = 0; bestIndex < scores.size(); bestIndex++) {
        if (VARSET_AND(pars, parents[bestIndex]) == parents[bestIndex]) break;
    }
    
    if (bestIndex == scores.size()) {
        return std::numeric_limits<float>::max();
    }
    
    return scores[bestIndex];
}

float bestscorecalculators::SparseParentList::getScore(int index) {
    if (index < 0) return 0;
    return scores[index];
}


varset &bestscorecalculators::SparseParentList::getParents(int index) {
    if (index < 0) {
        VARSET_NEW(ret, 0);
        return ret;
    }
    return parents[index];
}