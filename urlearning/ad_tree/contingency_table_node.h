/* 
 * File:   contingency_table_node.h
 * Author: malone
 *
 * Created on November 23, 2012, 7:19 PM
 */

#ifndef CONTINGENCY_TABLE_NODE_H
#define	CONTINGENCY_TABLE_NODE_H

#include <vector>

namespace scoring {
    
    class ContingencyTableNode {
        
    public:
        ContingencyTableNode(int value, int childrenSize, int leafCount) {
            this->value = value;
            this->leafCount = leafCount;
            
            for(int i = 0; i < childrenSize; i++) {
                children.push_back(NULL);
            }
        }
        
        ~ContingencyTableNode() {
            for(int i = 0; i < children.size(); i++) {
                delete children[i];
            }
        }
        
        int getValue() {
            return value;
        }
        
        ContingencyTableNode *getChild(int index) {
            return children[index];
        }
        
        void setChild(int index, ContingencyTableNode *child) {
            children[index] = child;
        }
        
        bool isLeaf() {
            return children.size() == 0;
        }
        
        // they are assumed to both be rooted at the same variable
        void subtract(ContingencyTableNode *other) {
            
            if(isLeaf()) {
                value -= other->value;
                return;
            }
            
            for(int k = 0; k < children.size(); k++) {
                if(children[k] == NULL || other->children[k] == NULL) {
                    continue;
                }
                
                children[k]->subtract(other->children[k]);
            } // end for each child
        } // end subtract
        
        int leafCount;
        
    private:
        std::vector< ContingencyTableNode* > children;
        int value;
    };
    
}


#endif	/* CONTINGENCY_TABLE_NODE_H */

