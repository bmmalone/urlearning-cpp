/* 
 * File:   sparse_tree_node.h
 * Author: bmmalone
 *
 * Created on August 1, 2013, 9:43 PM
 */

#ifndef SPARSE_TREE_NODE_H
#define	SPARSE_TREE_NODE_H

#include <algorithm>
#include <vector>
#include <boost/unordered_set.hpp>

#include "urlearning/base/typedefs.h"

namespace bestscorecalculators {

    class SparseTreeNode {
    public:

        SparseTreeNode(varset parents, float score, int variableCount) {
            this->parents = parents;
            this->score = score;
            this->bound = 0;
            this->lsb = lastSetBit(parents);

            for (int i = 0; i < variableCount; i++) {
                children.push_back(NULL);
            }
        }

        ~SparseTreeNode() {
            for (auto iter = children.begin(); iter != children.end(); iter++) {
                delete *iter;
            }
        }

        varset &getParents() {
            return parents;
        }

        float getScore() {
            return score;
        }

        float getBound() {
            return bound;
        }

        int getLastSetBit() {
            return lsb;
        }

        int getLastChild() {
            return lastChild;
        }
        
        varset &getBoundParents() {
            return boundParents;
        }

        void setBound(float bound, varset &boundParents) {
            this->bound = bound;
            this->boundParents = boundParents;
        }

        void setScore(float score) {
            this->score = score;
        }
        

        /**
         * Based on the score cache, add all of the present children to this node.
         * Return the bound of this node (the min of the bounds of its children,
         * or its score if it has no chidren).
         * 
         * @param scores
         * @param variableCount
         * @return 
         */
        float addChildren(FloatMap *scores, int variableCount, boost::unordered_set<varset> &usedScores) {

            bound = score;
            boundParents = parents;

            usedScores.insert(parents);

            // now, look for scores that have exactly one more bit set, after lsb
            for (int x = lsb + 1; x < variableCount; x++) {
                VARSET_SET(parents, x);

                // is this score in the cache?
                auto it = scores->find(parents);
                if (it == scores->end()) {
                    VARSET_CLEAR(parents, x);
                    continue;
                }
                float score = (*it).second;
                if (score != 0) {
                    SparseTreeNode *child = new SparseTreeNode(parents, score, variableCount);
                    //children.push_back(child);
                    children[x] = child;
                    lastChild = x;
                    childIndices.push_back(x);
                    float b = child->addChildren(scores, variableCount, usedScores);

                    if (b < bound) {
                        bound = b;
                        boundParents = child->getBoundParents();
                    }
                }

                VARSET_CLEAR(parents, x);
            }

            return bound;
        }

        void addChild(SparseTreeNode *child, int index) {
            //children.push_back(child);
            children[index] = child;

            if (index > lastChild) {
                lastChild = index;
            }

            childIndices.push_back(index);
            std::sort(childIndices.begin(), childIndices.end());
        }

        SparseTreeNode* getChildByIndex(int index) {
            return children[index];
        }

        int getSize() {
            int size = 1; // me

            for (int i = 0; i < children.size(); i++) {
                if (children[i] != NULL) {
                    size += children[i]->getSize();
                }
            }

            return size;
        }

        void print(int depth) {
            for (int i = 0; i < depth; i++) {
                printf("\t");
            }

            printVarset(parents);

            printf(": score: %f, bound: %f\n", score, bound);

            for (int i = 0; i < children.size(); i++) {
                if (children[i] != NULL) {
                    children[i]->print(depth + 1);
                }
            }
        }

        std::vector<SparseTreeNode*>::iterator childrenBegin() {
            return children.begin();
        }

        std::vector<SparseTreeNode*>::iterator childrenEnd() {
            return children.end();
        }

        SparseTreeNode *getChild(int nextIndex) {
            for (auto succ = children.begin(); succ != children.end(); succ++) {
                if (VARSET_GET((*succ)->getParents(), nextIndex)) {
                    return *succ;
                }
            }
            return NULL;
        }
        
        int getChildCount() {
            return childIndices.size();
        }
        
        int getChildIndex(int i) {
            return childIndices[i];
        }
        
        SparseTreeNode* getChildByChildIndex(int i) {
            return children[childIndices[i]];
        }

    private:
        varset parents;
        varset boundParents;
        float score;
        float bound;
        int lsb;
        int lastChild;
        std::vector<SparseTreeNode*> children;
        std::vector<int> childIndices;
    };

    struct CompareSparseTreeNodeStar {

        bool operator()(SparseTreeNode *a, SparseTreeNode * b) {

            float diff = a->getBound() - b->getBound();
            //        if (fabs(diff) < std::numeric_limits<float>::epsilon()) {
            //            return (b->getScore() - a->getScore()) > 0;
            //        }

            return (diff > 0);
        }
    };

}

#endif	/* SPARSE_TREE_NODE_H */

