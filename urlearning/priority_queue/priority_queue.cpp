#include "priority_queue.h"
#include "priority_queue-inl.h"

#include "urlearning/base/node.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

PriorityQueue::PriorityQueue() {
    // nothing to init
}

PriorityQueue::~PriorityQueue() {
    //clear();
}

void PriorityQueue::clear() {
    printf("Clearing the priority queue\n");
    for (Node *n : pq_) {
        printf("pq deleting node: '%s'\n", varsetToString(n->getSubnetwork()).c_str());
        if (n != NULL) {
            printf("the node was not null\n");
            delete n;
            printf("the node was deleted\n");
        }
    }
    printf("the pq is completely clear\n");

    pq_.clear();
}

void PriorityQueue::clearNoDelete() {
    pq_.clear();
}

void PriorityQueue::push(Node* n) {
    pq_.push_back(n);
    priority_queue::push_heap(pq_.begin(), pq_.end(), cns, 0);
}

Node* PriorityQueue::pop() {
    Node *ret = pq_.front();
    priority_queue::pop_heap(pq_.begin(), pq_.end(), cns, 0);
    pq_.pop_back();
    return ret;
}

Node* PriorityQueue::peek() {
    Node *ret = pq_.front();
    return ret;
}

/**
 * This method assumes the node n is in the priority queue,
 * and that it has been given a better priority (f value).
 * (So n->getPqPos() gives the position of n in the array representing the heap.
 * It adjusts the heap accordingly.
 * 
 * @param n the node
 */
void PriorityQueue::update(Node *n) {
    std::vector<Node*>::iterator itN = pq_.begin() + n->getPqPos();
    priority_queue::update_heap_pos(pq_.begin(), pq_.end(), itN, cns);
}
