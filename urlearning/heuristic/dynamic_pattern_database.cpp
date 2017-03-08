#include "dynamic_pattern_database.h"

#include <stdio.h>

heuristics::DynamicPatternDatabase::DynamicPatternDatabase(int variableCount, int maxSize, bool optimal) {
    this->variableCount = variableCount;
    this->maxSize = maxSize;
    this->optimal = optimal;
}

heuristics::DynamicPatternDatabase::~DynamicPatternDatabase() {
    // nothing
}

void heuristics::DynamicPatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*>& spgs, varset &ancestors, varset &scc) {
    printf("A dynamic pattern database is being created with ancestors and sccs.\n");
    printf("The heuristic does not use this information (but is still admissible).\n");
}

void heuristics::DynamicPatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*>& spgs) {
    // maps, etc.
    this->spgs = spgs;

    VARSET_CLEAR_ALL(allVariables);
    VARSET_SET_ALL(allVariables, variableCount);

    // initialize layer 0
    VARSET_NEW(empty, variableCount);
    DPDNode *start = new DPDNode(0, 0);
    previousLayer[allVariables] = start;

    // less than max size because we already generated layer 0
    for (int i = 0; i < maxSize; i++) {
        for (auto p = previousLayer.begin(); p != previousLayer.end(); p++) {
            varset key = (*p).first;
            DPDNode *node = (*p).second;
            expand(key, node);

            key = VARSET_AND(allVariables, VARSET_NOT(key));
            allLayers[key] = node;
        }

        // move the layer backward
        previousLayer = currentLayer;
        currentLayer = DPDNodeMap();
    }

    // put the last layer
    for (auto p = previousLayer.begin(); p != previousLayer.end(); p++) {
        varset key = (*p).first;
        DPDNode *node = (*p).second;

        key = VARSET_AND(allVariables, VARSET_NOT(key));
        allLayers[key] = node;
    }

    // now prune
    prune();
}

void heuristics::DynamicPatternDatabase::expand(varset subnetwork, DPDNode* node) {

    for (int leaf = 0; leaf < variableCount; leaf++) {

        if (!VARSET_GET(subnetwork, leaf)) continue;

        float bestScore = spgs[leaf]->getScore(subnetwork);
        float newG = bestScore + node->gCost;

        varset newSubnetwork = varsetClearCopy(subnetwork, leaf);
        DPDNode *oldNode = currentLayer[newSubnetwork];

        // getScore(0) returns the best score (i.e., simple heuristic value) for the leaf
        float differential = node->differential + bestScore - spgs[leaf]->getScore(0);
        if (fabs(node->differential - differential) < std::numeric_limits<float>::epsilon()) {
            toPrune.insert(newSubnetwork);
        }

        // check if this is the first time we have seen this subnetwork
        if (oldNode == NULL) {
            oldNode = new DPDNode(newG, differential);
            currentLayer[newSubnetwork] = oldNode;
            continue;
        }

        // check if the new path is  better
        if (newG < oldNode->gCost) {
            oldNode->gCost = newG;
        }

        if (differential < oldNode->differential) {
            oldNode->differential = differential;
        }
    }
}

void heuristics::DynamicPatternDatabase::prune() {

    printf("Full pattern database size: %d\n", allLayers.size());

    for (auto it = toPrune.begin(); it != toPrune.end(); it++) {

        varset pattern = *it;
        pattern = VARSET_AND(allVariables, VARSET_NOT(pattern));

        // do not remove any one-variable pattern
        if (cardinality(pattern) > 1) {
#ifdef DEBUG
            printf("Pruning pattern: %s\n", varsetToString(pattern).c_str());
#endif
            allLayers.erase(pattern);
        }
    }

    std::vector<std::pair<varset, DPDNode*> > temp(allLayers.begin(), allLayers.end());
    std::sort(temp.begin(), temp.end(), dpdCmp);

    patterns.clear();
    for (auto it = temp.begin(); it != temp.end(); it++) {
        varset pattern = (*it).first;
        DPDNode *node = (*it).second;
        std::pair<varset, float> p(pattern, node->gCost);
        patterns.push_back(p);
    }

    printf("Pruned pattern database size: %d\n", patterns.size());
}

int heuristics::DynamicPatternDatabase::size() {
    return patterns.size();
}

float heuristics::DynamicPatternDatabase::h(const varset& variables, bool &complete) {
    if (optimal) {
        return hOptimal(variables);
    } else {
        return hGreedy(variables);
    }
}

/**
 * Find the optimal h-value for this node by constructing an integer program to
 * solve the maximum weighted matching problem.
 * 
 * This implementation is quite dirty.  It creates the file in CPLEX format,
 * invokes SCIP via the std::system command (so it assumes scip is in the path),
 * redirects the output of SCIP to a file, and parses the file to get the bound.
 */
float heuristics::DynamicPatternDatabase::hOptimal(const varset& removedVariables) {

    // find the remaining variables
    VARSET_NEW(empty, variableCount);
    
    // create the temp ilp file
    FILE *out = fopen("temp.lp", "w");
    
    // first, the objective function
    fprintf(out, "Maximize\n");
    fprintf(out, "obj: ");
    
    // now the weights, only include patterns that do not cover removed variables
    std::vector<varset> validPatterns;
    for (auto it = patterns.begin(); it != patterns.end(); it++) {
        varset pattern = (*it).first;
        if (VARSET_EQUAL(VARSET_AND(pattern, removedVariables), empty)) {
            fprintf(out, "+ %f x_%d", (*it).second, validPatterns.size());
            validPatterns.push_back(pattern);
        }
    }
    fprintf(out, "\n");
    
    // now the constraints
    fprintf(out, "Subject to\n");
    
    // pick each remaining variable exactly once
    for (int i = 0; i < variableCount; i++) {
        // skip removed variables
        if (VARSET_GET(removedVariables, i)) continue;
        
        for (int j = 0; j < validPatterns.size(); j++) {
            fprintf(out, "+ a_%d_%d ", i, j);
        }
        fprintf(out, " = 1\n");
    }
    
    // and we have to pick the variables in each valid pattern
    for (int j = 0; j < validPatterns.size(); j++) {
        varset pattern = validPatterns[j];
        
        // check which variables are in the pattern
        for (int i = 0; i < variableCount; i++) {
            if (VARSET_GET(pattern, i)) {
                // so the variable is in the pattern
                fprintf(out, "- x_%d + a_%d_%d = 0\n", j, i, j);
                fprintf(out, "+ x_%d - a_%d_%d = 0\n", j, i, j);
            } else {
                // then the variable is not in this pattern
                // so a_ij is always 0
                fprintf(out, "a_%d_%d = 0\n", i, j);
            }
        }
    }
    
    // and everything is binary
    fprintf(out, "binary\n");
    for (int j = 0; j < validPatterns.size(); j++) {
        fprintf(out, "x_%d\n", j);
        
        for (int i = 0; i < variableCount; i++) {
            fprintf(out, "a_%d_%d\n", i, j);
        }
    }
    
    fclose(out);
    
    // call scip with the file
    std::string cmd = "/usr/bin/env scip -f temp.lp > temp.scip.out";
    std::system(cmd.c_str());
    
    // now parse the results to get the bound
    float h = 0;
    std::ifstream in("temp.scip.out");
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        int index = line.find("objective value");
        if (index > -1) {
            // then find the bound from the end of the line
            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
            h = atof(tokens[2].c_str());
            break;
        }
    }
    in.close();
    
    return h;
}

float heuristics::DynamicPatternDatabase::hGreedy(const varset& subnetwork) {
    float b = 0;

    VARSET_NEW(remaining, variableCount);
    VARSET_SET_ALL(remaining, variableCount);
    VARSET_CLEAR_INTERSECTION(remaining, subnetwork);

    VARSET_NEW(empty, variableCount);
    for (auto it = patterns.begin(); it != patterns.end() && !VARSET_EQUAL(remaining, empty); it++) {

        // is this pattern a subset of the remaining variables?
        varset pattern = (*it).first;
        if (VARSET_AND(remaining, pattern) == pattern) {
            b += (*it).second;
            VARSET_CLEAR_INTERSECTION(remaining, pattern);
        }
    }

    return b;
}

void heuristics::DynamicPatternDatabase::print() {
    printf("[");

    for (auto it = patterns.begin(); it != patterns.end(); it++) {
        varset pattern = (*it).first;
        float score = (*it).second;

        printf("%s\t%f\n", varsetToString(pattern).c_str(), score);
        //printf("(np.uint64(%" PRIu64 "),%f),", pattern, score);
    }
    printf("]");
}
