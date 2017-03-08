/* 
 * File:   PriorityQueue.h
 * Author: malone
 *
 * Created on August 21, 2012, 10:34 PM
 */

#ifndef PRIORITYQUEUE_H
#define	PRIORITYQUEUE_H

#include <vector>

#include "urlearning/base/typedefs.h"
#include "urlearning/base/node.h"

class PriorityQueue {
public:

    PriorityQueue();
    ~PriorityQueue();

    void push(Node *n);
    Node *pop();
    Node *peek();
    void update(Node *sn);
    
    void remove(varset &variables);
    
    void clear();
    void clearNoDelete();
    
    void print();

    int size() const {
        return pq_.size();
    }
    
    std::vector<Node*>::iterator begin() {
        return pq_.begin();
    }
    
    std::vector<Node*>::iterator end() {
        return pq_.end();
    }

private:
    std::vector<Node*> pq_;
    CompareNodeStar cns;
    CompareNodeStarLexicographic cnsl;
};


#endif	/* PRIORITYQUEUE_H */

