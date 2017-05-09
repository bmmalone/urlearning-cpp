/*
 * a_star_searcher.h
 *
 *  Created on: May 8, 2017
 *      Author: bmmalone
 */

#ifndef URLEARNING_ASTAR_ASTAR_SEARCHER_H_
#define URLEARNING_ASTAR_ASTAR_SEARCHER_H_

#include <limits>

#include "urlearning/base/node.h"
#include "urlearning/base/typedefs.h"

#include "urlearning/score_cache/score_cache.h"
#include "urlearning/score_cache/best_score_calculator.h"
#include "urlearning/priority_queue/priority_queue.h"
#include "urlearning/score_cache/best_score_calculator.h"
#include "urlearning/heuristic/heuristic.h"


namespace astar {

/**
 * Manage an instance of the A* search algorithm for BNSL. The search is
 * first configured with appropriate options, then the actual best-first
 * search is performed, and finally the results can be retrieved. The following
 * minimal example shows the required method calls.
 *
 * // Create the searcher
 * astar::AStarSearcher *astar_searcher = new astar::AStarSearcher();
 *
 * // Handle the score cache
 * scoring::ScoreCache cache;
 * cache.read(scoreFile);
 * astar_searcher->set_cache(cache);
 *
 * // Set up the start and goal for the search
 * VARSET_NEW(start,variable_count);
 * VARSET_NEW(goal, variable_count);
 * VARSET_SET_ALL(goal, variable_count);
 *
 * astar_searcher->set_start(start);
 * astar_searcher->set_goal(goal);
 * astar_searcher->update_vars_to_add();
 *
 * // Create the BestScore caclulators
 * bestscorecalculators::best_score_calculators best_score_calculators =
 * 		bestscorecalculators::create(bestScoreCalculator, cache);
 * astar_searcher->set_best_score_calculators(best_score_calculators);
 *
 * // Create the heuristic
 * heuristics::Heuristic *heuristic = heuristics::createWithAncestors(
 *     heuristicType,
 *     heuristicArgument,
 *     best_score_calculators,
 *     start,
 *     goal
 * );
 *
 * // Initialize the search data structures
 * astar_searcher->init_open_list();
 *
 * // Perform the search
 * astar_searcher->search();
 *
 * // wrap up the search
 * if (astar_searcher->found_solution()) {
 *     printf("Found solution: %f\n", astar_searcher->get_optimal_structure_score());
 *     datastructures::BayesianNetwork *network = astar_searcher->get_optimal_structure();
 * else {
 *     printf("No solution found.\n");
 *     printf("Lower bound: %f\n", astar_searcher->get_current_lower_bound());
 * }
 */
class AStarSearcher {
public:
	AStarSearcher(): goal_node(NULL), out_of_time(false) {}
	~AStarSearcher();

	/**
	 * Set the score cache for use in the search.
	 */
    void set_cache(const scoring::ScoreCache& new_cache) {
    	cache = new_cache;
    	variable_count = cache.getVariableCount();
    }

    /**
     * Set the start node for the search. This may not be the node for the
     * empty set if the UAI '14 decomposition strategy is used.
     */
    void set_start(const varset& new_start) {
    	start = new_start;
    }

    /**
     * Set the goal node for the search. This may not be the node for the full
     * set of variables in the UAI '14 decomposition strategy is used.
     */
    void set_goal(const varset& new_goal) {
    	goal = new_goal;
    }

    /**
     * Find the set of variables which we consider during this search. This is
     * equivalent to (goal XOR start).
     */
    void update_vars_to_add() {
    	vars_to_add = VARSET_XOR(goal, start);
    }

    /**
     * Set the best score calculators to use for the search.
     */
    void set_best_score_calculators(bestscorecalculators::best_score_calculators& new_best_score_calculators) {
    	best_score_calculators = new_best_score_calculators;
    }

    /**
     * Set the heuristic function to use during the search.
     */
    void set_heuristic(heuristics::Heuristic *new_heuristic) {
    	heuristic = new_heuristic;
    }

    /**
     * Initialize the generated_nodes map and clear the open list.
     */
    void init_open_list() {
        init_map(generated_nodes);
        open_list.clear();
    }

    /**
     * Perform the search from start to goal using the given heuristic and
     * best score calculators. This method continues until any of:
     *
     * 	(1) The shortest path to the goal node is found
     *
     * 	(2) The open list is empty. If this does not result in a path to the
     * 		goal node, then it is likely inconsistent constraints were used
     * 		when calculating local scores.
     *
     * 	(3) The out_of_time flag is set (by some external source)
     */
    void search();

    /**
     * Check whether ::search found a path to the goal.
     *
     * @return true if a path was found
     */
    bool found_solution() { return (goal_node != NULL); }

    /**
     * If ::search found a path to the goal, use the back pointers to
     * reconstruct the optimal structure for the ::vars_to_add.
     *
     * @return a pointer to a network with the optimal structure for this
     * 		search, or NULL if a path was not found.
     */
    datastructures::BayesianNetwork *get_optimal_structure();

    /**
     * If ::search found a path to ::goal, return the cost of that path.
     *
     * @return the cost of the path, or infinity if a path was not found.
     */
    double get_optimal_structure_score() {
    	if (goal_node != NULL) {
    		return goal_node->getG();
    	}

    	return std::numeric_limits<double>::infinity();
    }

    /**
     * A lower (optimistic) bound on the cost of a path from ::start to ::goal.
     * If a shortest path has been found, then the bound is tight.
     *
     * @return Either the (possibly tight) bound on the cost of the shortest
     * 		path, or infinity if no path exists. See condition (2) of ::search
     * 		for more details about why a path may not exist.
     */
    double get_current_lower_bound() {
    	// do we have a path?
    	if (goal_node != NULL) {
    		return get_optimal_structure_score();
    	}

    	// do we have nodes we have not yet examined?
    	if (open_list.size() > 0) {
    		Node *u = open_list.peek();
    		return u->getF();
    	}

    	// then we cannot say anything
    	return std::numeric_limits<double>::infinity();

    }

    /**
     * Print diagnostic information about the search.
     */
    void print_statistics();

    /**
     * Update the out_of_time flag. Presumably, this setter is used during the
     * search, and the search will quit before expanding any more nodes.
     */
    void set_out_of_time(bool new_out_of_time) {
    	out_of_time = new_out_of_time;
    }


private:
	scoring::ScoreCache cache;
	int variable_count;

	bestscorecalculators::best_score_calculators best_score_calculators;
	heuristics::Heuristic *heuristic;

	NodeMap generated_nodes;
	PriorityQueue open_list;

	int nodes_expanded;
	bool out_of_time;

	varset start;
	varset goal;
	varset vars_to_add;

	Node *goal_node;
	bool complete;

};

} /* namespace astar */

#endif /* URLEARNING_ASTAR_ASTAR_SEARCHER_H_ */
