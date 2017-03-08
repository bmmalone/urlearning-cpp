/* 
 * File:   ad_tree_node.h
 * Author: malone
 *
 * Created on November 22, 2012, 9:16 PM
 */

#ifndef AD_TREE_NODE_H
#define	AD_TREE_NODE_H

#include <vector>
#include <boost/dynamic_bitset.hpp>

namespace scoring {
    
    class VaryNode;
    
class ADNode {

public:
    ADNode() {}
    
    ADNode(int size, int count) {
        this->count = count;
        
        for(int i = 0; i < size; i++) {
            children.push_back(NULL);
        }
    }
    
    ~ADNode();
    
    int getCount() {
        return count;
    }
    
    void setLeafList(boost::dynamic_bitset<> &leafList) {
        this->leafList = leafList;
    }
    
    boost::dynamic_bitset<> &getLeafList() {
        return leafList;
    }
    
    void setChild(int index, VaryNode *child) {
        children[index] = child;
    }
    
    VaryNode *getChild(int index) {
        return children[index];
    }
    
private:
    std::vector<VaryNode*> children;
    int count;
    boost::dynamic_bitset<> leafList;
    
};
    
}



#endif	/* AD_TREE_NODE_H */

