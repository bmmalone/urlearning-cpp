/* 
 * File:   ad_tree.h
 * Author: malone
 *
 * Created on November 22, 2012, 9:21 PM
 */

#ifndef AD_TREE_H
#define	AD_TREE_H

#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "urlearning/base/bayesian_network.h"
#include "urlearning/base/record_file.h"
#include "urlearning/base/typedefs.h"

#include "ad_node.h"
#include "contingency_table_node.h"
#include "vary_node.h"

namespace scoring {
    
    class ADTree {
    public:
        ADTree() {
            // do nothing
        }
        ADTree(int rMin);
        ~ADTree() {
            if (root != NULL) {
                delete root;
            }
            root = NULL;
        }

        void initialize(datastructures::BayesianNetwork &network, datastructures::RecordFile &recordFile);
        void createTree();
        
        ContingencyTableNode* makeContab(varset variables);

    private:
        ADNode* makeADTree(int i, bitset &recordNums, int depth, varset variables);
        VaryNode* makeVaryNode(int i, bitset &recordNums, int depth, varset variables);
        
        ContingencyTableNode* makeContab(varset remainingVariables, ADNode* node, int nodeIndex);
        ContingencyTableNode* makeContabLeafList(varset variables, bitset &records);

        std::vector< std::vector< bitset > > consistentRecords;

        datastructures::BayesianNetwork network;
        int recordCount;
        int rMin;

        ADNode *root;
        varset zero;
    };

}

#endif	/* AD_TREE_H */

