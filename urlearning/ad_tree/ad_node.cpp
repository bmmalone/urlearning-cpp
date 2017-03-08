#include "ad_node.h"
#include "vary_node.h"

scoring::ADNode::~ADNode() {
    for (int i = 0; i < children.size(); i++) {
        delete children[i];
    }
}
