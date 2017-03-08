#include "vary_node.h"
#include "ad_node.h"

scoring::VaryNode::~VaryNode() {
    for (int i = 0; i < children.size(); i++) {
        delete children[i];
    }
}
