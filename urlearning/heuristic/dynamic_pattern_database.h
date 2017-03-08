/* 
 * File:   dynamic_pattern_database.h
 * Author: bmmalone
 *
 * Created on August 29, 2013, 8:57 AM
 */

#ifndef DYNAMIC_PATTERN_DATABASE_H
#define	DYNAMIC_PATTERN_DATABASE_H

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <vector>

#include "urlearning/base/typedefs.h"
#include "heuristic.h"

namespace heuristics {


    // <editor-fold defaultstate="collapsed" desc="Nested Node Class">

    /**
     * This class stores information about a subnetwork while constructing the pattern database.
     */
    class DPDNode {
    public:
        float gCost;
        float differential;

        DPDNode(float g, float d) : gCost(g), differential(d) {
        }
    };

    inline bool dpdCmp(const std::pair<varset, DPDNode*> &p1, const std::pair<varset, DPDNode*> &p2) {
        return p1.second->differential > p2.second->differential;
    }

    // </editor-fold>

    typedef boost::unordered_map<varset, DPDNode*> DPDNodeMap;

    class DynamicPatternDatabase : public Heuristic {
    public:

        DynamicPatternDatabase() {
        }
        DynamicPatternDatabase(int variableCount, int maxSize, bool optimal);
        ~DynamicPatternDatabase();

        void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs);
        void initialize(std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, varset &ancestors, varset &scc);

        int size();
        float h(const varset &variables, bool &complete);
        void print();
        void printStatistics() {
            printf("Dynamic pattern database does not collect statistics\n");
        }

    public:
        float hGreedy(const varset &variables);
        virtual float hOptimal(const varset &variables);

        void expand(varset subnetwork, DPDNode *node);
        void prune();

        /**
         * The total number of variables in the dataset.
         */
        int variableCount;

        /**
         * The size of the largest patterns (inclusive).
         */
        int maxSize;

        /**
         * Whether to use the greedy or IP to find heuristic values.
         */
        bool optimal;

        /**
         * A representation in which all of the variables are present.
         */
        varset allVariables;

        /**
         * The pattern subsets, ordered by differential.
         */
        std::vector<std::pair<varset, float> > patterns;

        /**
         * Data structures to answer BestScore queries.
         */
        std::vector<bestscorecalculators::BestScoreCalculator*> spgs;

        /**
         * A list of patterns to prune because some combination of smaller patterns
         * has a better differential.
         */
        boost::unordered_set<varset> toPrune;

        /**
         * Hash tables to perform the BFS.
         */
        DPDNodeMap previousLayer;
        DPDNodeMap currentLayer;

        /**
         * All previous layers of the search.  We need to store the node data
         * structures so we can sort things later.
         */
        DPDNodeMap allLayers;

    };

}



#endif	/* DYNAMIC_PATTERN_DATABASE_H */

