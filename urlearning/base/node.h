/* 
 * File:   Node.h
 * Author: malone
 *
 * Created on August 21, 2012, 10:35 PM
 */

#ifndef NODE_H
#define	NODE_H

#include <limits>
#include <iostream>
#include <ostream>
#include <fstream>

#include "typedefs.h"

namespace boost {
namespace serialization {
class access;
}
}

class Node {
public:

    Node():  g(0), h(0), subnetwork(varset(0)), leaf(0), pqPos(-1) {}

    Node(int variableCount) : g(0), h(0), subnetwork(varset(variableCount)), leaf(0), pqPos(-1) {
    }

    Node(float g, float h, varset &subnetwork, byte &leaf) :
    g(g), h(h), subnetwork(subnetwork), leaf(leaf), pqPos(0) {
    }

    float getF() const {
        return g + h;
    }

    varset getSubnetwork() const {
        return subnetwork;
    }

    int getPqPos() const {
        return pqPos;
    }

    byte getLeaf() const {
        return leaf;
    }

    float getH() const {
        return h;
    }

    float getG() const {
        return g;
    }

    /**
     * Count layer of this node.
     * @return 
     */
    byte getLayer() {
        return cardinality(subnetwork);
    }

    void copy(Node* n) {
        g = n->g;
        h = n->h;
        leaf = n->leaf;
        subnetwork = n->subnetwork;
    }

    void setSubnetwork(varset subnetwork) {
        this->subnetwork = subnetwork;
    }

    void setPqPos(int pqPos) {
        this->pqPos = pqPos;
    }

    void setLeaf(byte leaf) {
        this->leaf = leaf;
    }

    void setG(float g) {
        this->g = g;
    }

    void setH(float h) {
        this->h = h;
    }
    void save(std::ofstream& of) {
        of.write((char*)&g, sizeof(g)); 
        of.write((char*)&h, sizeof(h)); 
        of.write((char*)&subnetwork, sizeof(subnetwork)); 
    }
    void load(std::ifstream& inf) { 
        inf.read((char*)&g, sizeof(g)); 
        inf.read((char*)&h, sizeof(h)); 
        inf.read((char*)&subnetwork, sizeof(subnetwork)); 
    } 
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & g;
        ar & h;
        ar & subnetwork;
    }
    
protected:
    float g;
    float h;
    varset subnetwork;
    byte leaf;
    int pqPos;
};

struct CompareNodeStar {

    bool operator()(Node *a, Node * b) {

        float diff = a->getF() - b->getF();
        if (fabs(diff) < std::numeric_limits<float>::epsilon()) {
            return (b->getLayer() - a->getLayer()) > 0;
        }

        return (diff > 0);
    }
};

// TODO: make this work for boost::bitset representation
struct CompareNodeStarLexicographic {
    
    bool operator()(Node *a, Node *b) {
        int diff = differenceBetweenVarsets(a->getSubnetwork(),b->getSubnetwork());
        if (diff == 0) {
            diff = a->getF() - b->getF();
            if (fabs(diff) < std::numeric_limits<float>::epsilon()) {
                return false;
            }
        }
        return (diff > 0);
    }
};

#endif	/* NODE_H */

