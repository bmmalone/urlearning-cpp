/* 
 * File:   ad_tree.cpp
 * Author: malone
 * 
 * Created on November 22, 2012, 9:21 PM
 */

#include <string.h>

#include "ad_tree.h"
#include "contingency_table_node.h"

scoring::ADTree::ADTree(int rMin) {
    this->rMin = rMin;
}

void scoring::ADTree::initialize(datastructures::BayesianNetwork &network, datastructures::RecordFile &recordFile) {
    this->network = network;
    this->recordCount = recordFile.size();
    VARSET_NEW(empty, network.size());
    zero = empty;

    consistentRecords = network.getConsistentRecords(recordFile);
}

void scoring::ADTree::createTree() {
    bitset countIndices(recordCount);
    BITSET_SET_ALL(countIndices);
    VARSET_NEW(empty, network.size());
    root = makeADTree(0, countIndices, 0, empty);
}

scoring::ADNode* scoring::ADTree::makeADTree(int i, bitset &recordNums, int depth, varset variables) {

    // since this is index i, there are (variableCount - i) remaining variables.
    // therefore, it will have that many children
    ADNode *adn = new ADNode(network.size() - i, BITSET_COUNT(recordNums));

    // check if we should just use a leaf list
    if (adn->getCount() < rMin) {
        BITSET_COPY(recordNums, leafList);
        adn->setLeafList(leafList);
        return adn;
    }

    // for each of the remaining variables
    for (int j = i; j < network.size(); j++) {
        // create a vary node
        varset newVariables = VARSET_SET(variables, j);
        VaryNode *child = makeVaryNode(j, recordNums, depth, newVariables);
        adn->setChild(j - i, child);
    }

    return adn;
}

scoring::VaryNode* scoring::ADTree::makeVaryNode(int i, bitset &recordNums, int depth, varset variables) {
    // this node will have variableCardinalities[i] children
    VaryNode *vn = new VaryNode(network.getCardinality(i));

    // split into ChildNums
    std::vector< bitset > childNums;

    int mcv = -1;
    int mcvCount = -1;
    for (int k = 0; k < network.getCardinality(i); k++) {
        childNums.push_back(BITSET_NEW(recordCount));
        childNums[k] |= recordNums;
        childNums[k] &= consistentRecords[i][k];

        // also look for the mcv
        int count = childNums[k].count();
        if (count > mcvCount) {
            mcv = k;
            mcvCount = count;
        }
    }

    // update the mcv
    vn->setMcv(mcv);

    // otherwise, recurse
    for (int k = 0; k < network.getCardinality(i); k++) {
        if (k == mcv || childNums[k].count() == 0) {
            continue;
        }

        ADNode *child = makeADTree(i + 1, childNums[k], depth + 1, variables);
        vn->setChild(k, child);
    }

    return vn;
}

scoring::ContingencyTableNode* scoring::ADTree::makeContab(varset variables) {
    return makeContab(variables, root, -1);
}

scoring::ContingencyTableNode* scoring::ADTree::makeContab(varset remainingVariables, scoring::ADNode* node, int nodeIndex) {
    // check base case
    if (remainingVariables == zero) {
        ContingencyTableNode *ctn = new ContingencyTableNode(node->getCount(), 0, 1);
        return ctn;
    }

    int firstIndex = VARSET_FIND_FIRST_SET(remainingVariables); // first set bit
    int n = network.getCardinality(firstIndex);
    VaryNode *vn = node->getChild(firstIndex - nodeIndex - 1);
    ContingencyTableNode *ct = new ContingencyTableNode(0, n, 0);
    varset newVariables = varsetClearCopy(remainingVariables, firstIndex);

    ContingencyTableNode *ctMcv = makeContab(newVariables, node, nodeIndex);

    for (int k = 0; k < n; k++) {
        if (vn->getChild(k) == NULL) { // also finds mcv
            continue;
        }

        ADNode *adn = vn->getChild(k);

        ContingencyTableNode *child = NULL;
        if (adn->getLeafList().size() == 0) {
            child = makeContab(newVariables, adn, firstIndex);
        } else {
            child = makeContabLeafList(newVariables, adn->getLeafList());
        }

        ct->setChild(k, child);
        ct->leafCount += child->leafCount;

        ctMcv->subtract(ct->getChild(k));
    }
    ct->setChild(vn->getMcv(), ctMcv);
    ct->leafCount += ctMcv->leafCount;

    return ct;
}

scoring::ContingencyTableNode* scoring::ADTree::makeContabLeafList(varset variables, bitset &records) {
    if (variables == zero) {
        ContingencyTableNode *ct = new ContingencyTableNode(records.count(), 0, 1);
        return ct;
    }

    int firstIndex = VARSET_FIND_FIRST_SET(variables); // first set bit
    int cardinality = network.getCardinality(firstIndex);
    ContingencyTableNode *ct = new ContingencyTableNode(0, cardinality, 0);
    varset remainingVariables = VARSET_CLEAR(variables, firstIndex); // clear bit

    BITSET_CREATE(r, recordCount);
    for (int k = 0; k < cardinality; k++) {
        BITSET_CLEAR(r);
        BITSET_OR(r, records);
        BITSET_AND(r, consistentRecords[firstIndex][k]);

        if (BITSET_COUNT(r) > 0) {
            ContingencyTableNode *child = makeContabLeafList(remainingVariables, r);
            ct->setChild(k, child);
            ct->leafCount += child->leafCount;
        }
    }

    return ct;
}
