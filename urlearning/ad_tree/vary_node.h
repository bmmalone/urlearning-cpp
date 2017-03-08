/* 
 * File:   vary_node.h
 * Author: malone
 *
 * Created on November 22, 2012, 9:19 PM
 */

#ifndef VARY_NODE_H
#define	VARY_NODE_H

#include <cstddef>
#include <vector>

namespace scoring {

    class ADNode;

    class VaryNode {
    public:
        VaryNode(int size) {
            for(int i = 0; i < size; i++) {
                children.push_back(NULL);
            }
        }
        
        ~VaryNode();
        
        void setMcv(int mcv) {
            this->mcv = mcv;
        }
        
        int getMcv() {
            return mcv;
        }
        
        void setChild(int index, ADNode *child) {
            children[index] = child;
        }
        
        ADNode *getChild(int index) {
            return children[index];
        }

    private:
        std::vector<ADNode*> children;
        int mcv;

    };

}


#endif	/* VARY_NODE_H */

