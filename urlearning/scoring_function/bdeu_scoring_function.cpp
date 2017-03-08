#include "bdeu_scoring_function.h"
#include "urlearning/ad_tree/contingency_table_node.h"

#include <math.h>

scoring::BDeuScoringFunction::BDeuScoringFunction(float ess, datastructures::BayesianNetwork& network, ADTree *adTree, Constraints *constraints, bool enableDeCamposPruning) {
    this->network = network;
    this->adTree = adTree;
    this->constraints = constraints;
    this->ess = ess;
    this->enableDeCamposPruning = enableDeCamposPruning;

    for (int x = 0; x < network.size(); x++) {
        scratch *s = new scratch();
        s->variable = x;
        s->l_r_i = log(network.getCardinality(x));
        scratchSpace.push_back(s);
    }
}

void scoring::BDeuScoringFunction::lg(varset parents, scratch* s) {
    int r = 1;
    for (int pa = 0; pa < network.size(); pa++) {
        if (VARSET_GET(parents, pa)) {
            r *= network.getCardinality(pa);
        }
    }

    s->a_ij = ess / r;
    s->lg_ij = lgamma(s->a_ij);

    r *= network.getCardinality(s->variable);
    s->a_ijk = ess / r;
    s->lg_ijk = lgamma(s->a_ijk);
}

float scoring::BDeuScoringFunction::calculateScore(int variable, varset parents, FloatMap& cache) {
    scratch *s = scratchSpace[variable];

    // check if this violates the constraints
    if (constraints != NULL && !constraints->satisfiesConstraints(variable, parents)) {
        s->invalidParents.insert(parents);
        return 1;
    }

    if (enableDeCamposPruning) {
        for (int x = 0; x < network.size(); x++) {
            if (VARSET_GET(parents, x)) {
                VARSET_CLEAR(parents, x);

                // check the constraints
                if (s->invalidParents.find(parents) != s->invalidParents.end()) {
                    // we cannot say anything if we skipped this because of constraints
                    VARSET_SET(parents, x);
                    continue;
                }

                auto s = cache.find(parents);

                if (s == cache.end()) {
                    return 0;
                }

                VARSET_SET(parents, x);
            }
        }
    }

    lg(parents, s);

    VARSET_COPY(parents, variables);
    VARSET_SET(variables, variable);

    s->score = 0;

    ContingencyTableNode *ct = adTree->makeContab(variables);

    boost::unordered_map<uint64_t, int> paCounts;
    calculate(ct, 1, 0, paCounts, variables, -1, s);

    // check constraints (Theorem 9 from de Campos and Ji '11)
    // only bother if the alpha bound is okay
    // check if can prune
    if (enableDeCamposPruning) {
        if (s->a_ij <= 0.8349) {

            float bound = (float) (-1.0 * ct->leafCount * s->l_r_i);

            // check each subset
            for (int x = 0; x < network.size(); x++) {
                if (VARSET_GET(parents, x)) {
                    VARSET_CLEAR(parents, x);

                    // check the constraints
                    if (s->invalidParents.find(parents) != s->invalidParents.end()) {
                        // we cannot say anything if we skipped this because of constraints
                        VARSET_SET(parents, x);
                        continue;
                    }

                    float s = cache[parents];

                    // if the score is larger (better) than the bound, then we can prune
                    if (s > bound) {
                        delete ct;
                        return 0;
                    }

                    VARSET_SET(parents, x);
                }
            }
        }
    }

    delete ct;

    for (auto pc : paCounts) {
        s->score += s->lg_ij;
        s->score -= lgamma(s->a_ij + pc.second);
    }

    return s->score;
}

void scoring::BDeuScoringFunction::calculate(ContingencyTableNode* ct, uint64_t base, uint64_t index, boost::unordered_map<uint64_t, int>& paCounts, varset variables, int previousVariable, scratch *s) {

    // if this is a leaf in the AD-tree
    if (ct->isLeaf()) {
        // update the instantiation count of this set of parents
        int count = paCounts[index];
        count += ct->getValue();

        if (count > 0) {
            paCounts[index] = count;

            // update the score for this variable, parent instantiation
            float temp = lgamma(s->a_ijk + ct->getValue());
            s->score -= s->lg_ijk;
            s->score += temp;
        }
        return;
    }

    // which actual variable are we looking at
    int thisVariable = previousVariable + 1;
    for (; thisVariable < network.size(); thisVariable++) {
        if (VARSET_GET(variables, thisVariable)) break;
    }

    // update the base and index if this is part of the parent set
    uint64_t nextBase = base;
    if (thisVariable != s->variable) {
        nextBase *= network.getCardinality(thisVariable);
    }

    // recurse
    for (int k = 0; k < network.getCardinality(thisVariable); k++) {
        ContingencyTableNode *child = ct->getChild(k);
        if (child != NULL) {
            long newIndex = index;
            if (thisVariable != s->variable) {
                newIndex += base*k;
            }
            calculate(child, nextBase, newIndex, paCounts, variables, thisVariable, s);
        }
    }

}
