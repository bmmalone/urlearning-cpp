/*
 * astar_searcher.cpp
 *
 *  Created on: May 8, 2017
 *      Author: bmmalone
 */

#include "astar_searcher.h"

namespace astar {

AStarSearcher::~AStarSearcher() {

    for (auto bsc = best_score_calculators.begin();
    		bsc != best_score_calculators.end();
    		bsc++) {
        delete *bsc;
    }

    for (auto pair = generated_nodes.begin();
    		pair != generated_nodes.end();
    		pair++) {
        delete (pair->second);
    }

    delete heuristic;
}

void AStarSearcher::search() {

	// create the start node
    byte leaf(0);
    Node *start_node = new Node(0.0f, 0.0f, start, leaf);
    open_list.push(start_node);

    nodes_expanded = 0;
	while (open_list.size() > 0 && !out_of_time) {
		Node *u = open_list.pop();
		nodes_expanded++;

		varset variables = u->getSubnetwork();

		// check if it is the goal
		if (variables == goal) {
			goal_node = u;
			break;
		}

		// note that it is in the closed list
		u->setPqPos(-2);

		// expand
		for (byte leaf = 0; leaf < variable_count; leaf++) {
			// make sure this variable was not already present
			if (VARSET_GET(variables, leaf)) continue;

			// also make sure this is one of the variables we care about
			if (!VARSET_GET(vars_to_add, leaf)) continue;

			// get the new variable set
			VARSET_COPY(variables, new_variables);
			VARSET_SET(new_variables, leaf);


			// immediate duplicate detection
			Node *succ = generated_nodes[new_variables];

			// check if this is the first time we have generated this node
			if (succ == NULL) {
				// get the cost along this path
				float g = u->getG();
				g += best_score_calculators[leaf]->getScore(new_variables);

				// calculate the heuristic estimate
				float h = heuristic->h(new_variables, complete);

				// update all the values
				succ = new Node(g, h, new_variables, leaf);

				// add it to the open list
				open_list.push(succ);

				// and to the list of generated nodes
				generated_nodes[new_variables] = succ;
				continue;
			}

			// assume the heuristic is consistent
			// so check if it was in the closed list
			if (succ->getPqPos() == -2) {
				continue;
			}
			// we have generated a node in the open list
			// see if the new path is better
			float g = u->getG();
			g += best_score_calculators[leaf]->getScore(variables);
			if (g < succ->getG()) {
				// the update the information
				succ->setLeaf(leaf);
				succ->setG(g);

				// and the priority queue
				open_list.update(succ);
			}
		}
	}
}

datastructures::BayesianNetwork *AStarSearcher::get_optimal_structure() {

	// make sure we found a path
	if (goal_node == NULL) {
		return NULL;
	}

	// first, find the optimal parent sets
	std::vector<varset> optimal_parents;
	for (int i = 0; i < variable_count; i++) {
		optimal_parents.push_back(VARSET(variable_count));
	}

	VARSET_COPY(goal_node->getSubnetwork(), remaining_variables);
	Node *current = goal_node;
	float score = 0;
	int count = cardinality(vars_to_add);
	for (int i = 0; i < count; i++) {
		int leaf = current->getLeaf();
		score += best_score_calculators[leaf]->getScore(remaining_variables);
		varset parents = best_score_calculators[leaf]->getParents();
		optimal_parents[leaf] = parents;

		VARSET_CLEAR(remaining_variables, leaf);

		current = generated_nodes[remaining_variables];
	}

	// then create the (partial) BN structure
    datastructures::BayesianNetwork *network = cache.getNetwork();
    network->fixCardinalities();
    network->setParents(optimal_parents);
    network->setUniformProbabilities();
    return network;
}

void AStarSearcher::print_statistics() {

	printf("Nodes generated: %d\n", generated_nodes.size());
	printf("Nodes expanded: %d\n", nodes_expanded);
	printf("Open list size: %d\n", open_list.size());
}

} /* namespace astar */
