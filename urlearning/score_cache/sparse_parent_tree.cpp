#include "sparse_parent_tree.h"
#include "score_cache.h"
#include <algorithm>
#include <boost/unordered_set.hpp>

bestscorecalculators::SparseParentTree::SparseParentTree(const int variable, const int variableCount) {
    this->variable = variable;
    this->variableCount = variableCount;
    this->temp = NULL;
}

bestscorecalculators::SparseParentTree::~SparseParentTree() {
    if (root) {
        delete root;
    }
}

varset getSmallestUnused(const boost::unordered_set<varset> &usedScores, FloatMap *scores, int variableCount) {
    int minCardinality = std::numeric_limits<int>::max();
    VARSET_NEW(minVarset, variableCount);
    VARSET_SET_ALL(minVarset, variableCount);

    for (auto iter = scores->begin(); iter != scores->end(); iter++) {
        varset v = (*iter).first;
        if (cardinality(v) < minCardinality) {
            if (usedScores.find(v) == usedScores.end()) {

                minCardinality = cardinality(v);
                minVarset = v;
            }
        }
    }
    return minVarset;
}

void bestscorecalculators::SparseParentTree::initialize(const scoring::ScoreCache& scoreCache) {
    FloatMap *scores = scoreCache.getCache(variable);
    boost::unordered_set<varset> usedScores;

    // first, the empty set
    VARSET_NEW(empty, variableCount);
    float score = scores->at(empty);
    root = new SparseTreeNode(empty, score, variableCount);

    //    roots.push_back(r);

    // find all the children and set bounds recursively
    root->addChildren(scores, variableCount, usedScores);

    // now, pick up all of the remaining scores
    VARSET_NEW(flag, variableCount);
    VARSET_SET_ALL(flag, variableCount);

    varset unused = getSmallestUnused(usedScores, scores, variableCount);

    while (unused != flag) {
        VARSET_NEW(set, variableCount);
        temp = root;
        // the path to get to this node
        // used to propagate bounds back up
        std::vector<SparseTreeNode*> path;
        // try to find a path from empty to this root
        for (int i = 0; i < variableCount; i++) {
            if (VARSET_GET(unused, i)) {
                path.push_back(temp);
                VARSET_SET(set, i);
                SparseTreeNode *child = temp->getChildByIndex(i);

                // check if this child was missing
                if (child == NULL) {
                    // then create a dummy node, and add it to temp
                    child = new SparseTreeNode(set, std::numeric_limits<float>::max(), variableCount);
                    child->setBound(std::numeric_limits<float>::max(), empty);
                    temp->addChild(child, i);
                }

                temp = child;
            }
        }

        // temp should now point to where the new root should go
        score = scores->at(unused);
        temp->setScore(score);

        // find all the children and set bounds recursively
        temp->addChildren(scores, variableCount, usedScores);

        for (auto prev = path.begin(); prev != path.end(); prev++) {
            if ((*prev)->getBound() > temp->getBound()) {
                (*prev)->setBound(temp->getBound(), temp->getBoundParents());
            }
        }

        // need to propagate the bounds back up...

        unused = getSmallestUnused(usedScores, scores, variableCount);
    }
}

float bestscorecalculators::SparseParentTree::getScoreBestFirst(const varset &Q) {

    //best = infinity, bestS = null
    float best = std::numeric_limits<float>::max();
    open.clear();

    // push(OPEN, {}, bound({})
    //    for (auto iter = roots.begin(); iter != roots.end(); iter++) {
    //        if (VARSET_AND((*iter)->getParents(), Q) == (*iter)->getParents()) {
    //            open.push_back((*iter));
    //            std::push_heap(open.begin(), open.end(), cstns);
    //        }
    //    }
    open.push_back(root);

    // while best > bound
    while (open.size() > 0) {
        // U = pop(OPEN)
        temp = open.front();
        std::pop_heap(open.begin(), open.end(), cstns);
        open.pop_back();

        if (temp->getScore() < best) {
            best = temp->getScore();
        }

        if (temp->getBound() >= best) {
            break;
        }

        // for each child of U
        for (auto succ = temp->childrenBegin(); succ != temp->childrenEnd(); succ++) {
            // if succ \in Q
            if (VARSET_AND((*succ)->getParents(), Q) == (*succ)->getParents()) {
                // push (OPEN, succ, bound(succ))
                open.push_back((*succ));
                std::push_heap(open.begin(), open.end(), cstns);
            }
        }
    }

    return best;
}

float bestscorecalculators::SparseParentTree::getScore(varset &Q) {

    //best = infinity, bestS = null
    float best = std::numeric_limits<float>::max();

    while (stack.size() > 0) {
        stack.pop();
    }

    int maxVariable = lastSetBit(Q);

    stack.push(root);

    // while best > bound
    while (stack.size() > 0) {
        // U = pop(OPEN)
        temp = stack.top();
        stack.pop();

        if (temp->getScore() < best) {
            best = temp->getScore();
            bestParents = temp->getParents();
        }

        if (temp->getBound() >= best) {
            continue;
        }

        if (VARSET_AND(Q, temp->getBoundParents()) == temp->getBoundParents()) {
            // the bound is definitely better than best because of the previous conditional
            best = temp->getBound();
            bestParents = temp->getBoundParents();
            continue;
        }

        for (int i = 0; i < temp->getChildCount(); i++) {
            int index = temp->getChildIndex(i);
            if (index > maxVariable) break;
            if (!VARSET_GET(Q, index)) continue;

            // check to see if I can just use the best thing from this branch
            stack.push(temp->getChildByIndex(index));

        }

        //        // for each child of U
        //        for (auto succ = temp->childrenBegin(); succ != temp->childrenEnd(); succ++) {
        //            // if succ \in Q
        //            if ((*succ)->getBound() >= best) continue;
        //            if (VARSET_AND((*succ)->getParents(), Q) == (*succ)->getParents()) {
        //                // push (OPEN, succ, bound(succ))
        //                stack.push((*succ));
        //            }
        //        }
    }

    return best;
}