/* 
 * File:   sparse_parent_tree.h
 * Author: bmmalone
 *
 * Created on August 1, 2013, 9:41 PM
 */

#ifndef SPARSE_PARENT_TREE_H
#define	SPARSE_PARENT_TREE_H

#include <vector>
#include <stack>
#include <stdexcept>

#include "urlearning/base/typedefs.h"
#include "best_score_calculator.h"
#include "score_cache.h"
#include "sparse_tree_node.h"

namespace bestscorecalculators {

    class SparseParentTree : public BestScoreCalculator {
    public:
        SparseParentTree(const int variable, const int variableCount);
        ~SparseParentTree();
        void initialize(const scoring::ScoreCache &scoreCache);

        float getBestScore() const {
            return root->getScore();
        }
        float getScoreBestFirst(const varset &pars);
        float getScore(varset &pars);

        varset &getParents() {
            return bestParents;
        }

        int size() {
            return root->getSize();
        }

        void print() {
            printf("Sparse Parent Tree, variable: %d\n", variable);
            root->print(1);
        }

        float getScore(int index) {
            throw std::runtime_error("Sparse parent trees do not support retrieving scores by index.");
        }
        
        varset &getParents(int index) {
            throw std::runtime_error("Sparse parent lists do not support retrieving parents by index.");
        }

    private:
        int variableCount;
        int variable;
        SparseTreeNode* root;
        std::vector<int> childIndices;
        varset bestParents;
        std::vector<SparseTreeNode*> open;
        std::stack<SparseTreeNode*> stack;
        CompareSparseTreeNodeStar cstns;
        SparseTreeNode *temp;
    };

}


#endif	/* SPARSE_PARENT_TREE_H */

