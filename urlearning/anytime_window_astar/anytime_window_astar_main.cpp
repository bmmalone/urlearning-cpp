/* 
 * File:   main.cpp
 * Author: malone
 *
 * Created on August 6, 2012, 9:05 PM
 */

#include <string>
#include <iostream>
#include <ostream>
#include <vector>
#include <cstdlib>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include "urlearning/base/node.h"
#include "urlearning/base/typedefs.h"
#include "urlearning/score_cache/score_cache.h"

#include "urlearning/priority_queue/priority_queue.h"
#include "urlearning/fileio/hugin_structure_writer.h"

#include "urlearning/score_cache/best_score_calculator.h"
#include "urlearning/score_cache/best_score_creator.h"

#include "urlearning/heuristic/heuristic.h"
#include "urlearning/heuristic/heuristic_creator.h"

namespace po = boost::program_options;

/**
 * A timer to keep track of how long the algorithm has been running.
 */
boost::asio::io_service io;

/**
 * A variable to check if the user-specified time limit has expired.
 */
bool outOfTime;

/**
 * The path to the score cache file.
 */
std::string scoreFile;

/**
 * The type of heuristic to use.
 */
std::string heuristicType;

/**
 * The argument for creating the pattern database.
 */
std::string heuristicArgument;

/**
 * The file to write the learned network.
 */
std::string netFile;

/**
 * The data structure to use to calculate best parent set scores.
 * Either "tree", "list" or "bitwise".
 */
std::string bestScoreCalculator;

/**
 * The number of nodes expanded during the search.
 */
int nodesExpanded;

/**
 * The number of variables in this dataset.
 */
int variableCount;

/**
 * The sparse parent graphs created from the score cache.
 */
std::vector<bestscorecalculators::BestScoreCalculator*> spgs;

/**
 * The pattern database for this search.
 */
heuristics::Heuristic *heuristic;

/**
 * The open list for the search.
 */
PriorityQueue openList;

/**
 * A list of all nodes that have been generated in the search.
 */
NodeMap generated;

/**
 * A list of all nodes frozen in the current iteration.
 */
std::vector<Node*> frozen;

/**
 * The goal node of the search.  We keep track of this for pruning.
 */
Node *goal;

/**
 * A lower bound on the score of the best network.  In practice, after each
 * iteration, this is the lowest f-cost among the frozen nodes.
 */
float lowerBound;

float upperBound;

/**
 * The maximum running time for the algorithm.
 */
int runningTime;

/**
 * Handler when out of time.
 */
void timeout(const boost::system::error_code& /*e*/) {
    printf("Out of time\n");
    outOfTime = true;
}

std::vector<varset> reconstructSolution(Node *goal) {
    std::vector<varset> optimalParents;
    for (int i = 0; i < variableCount; i++) {
        optimalParents.push_back(VARSET(spgs.size()));
    }

    VARSET_COPY(goal->getSubnetwork(), remainingVariables);
    Node *current = goal;
    float score = 0;
    for (int i = 0; i < variableCount; i++) {
        int leaf = current->getLeaf();
        score += spgs[leaf]->getScore(remainingVariables);
        varset parents = spgs[leaf]->getParents();
        optimalParents[leaf] = parents;

        VARSET_CLEAR(remainingVariables, leaf);

        current = generated[remainingVariables];
    }

    return optimalParents;
}

void updateLowerBound() {
    lowerBound = goal->getG();

    for (auto iter = frozen.begin(); iter != frozen.end(); iter++) {
        // only add nodes that would not be pruned
        if ((*iter)->getF() < lowerBound) {
            lowerBound = (*iter)->getF();
        }
    }
}

void windowSearch(int windowSize) {
    // keep track of the deepest nodes we have seen
    int deepest = -1;
    
    bool complete = false;

    while ((openList.size() != 0) && (goal->getG() > openList.peek()->getF()) && !outOfTime) {
        Node *u = openList.pop();
        // note that it is in the closed list
        u->setPqPos(-2);

        int layer = u->getLayer();
        if (layer <= (deepest - windowSize)) {
            frozen.push_back(u);
            continue;
        } else if (layer > deepest) {
            deepest = layer;
        }

        nodesExpanded++;
        varset variables = u->getSubnetwork();

        // expand
        for (byte leaf = 0; leaf < variableCount; leaf++) {
            // make sure this variable was not already present
            if (VARSET_GET(variables, leaf)) continue;

            // get the new variable set
            VARSET_COPY(variables, newVariables);
            VARSET_SET(newVariables, leaf);

            Node *succ = generated[newVariables];

            // get the cost along this path
            float g = u->getG() + spgs[leaf]->getScore(newVariables);


            // check if this is the first time we have generated this node
            if (succ == NULL) {
                // calculate the heuristic estimate
                complete = false;
                float h = heuristic->h(newVariables, complete);
                
#ifdef CHECK_COMPLETE
                if (complete) {
                    float score = g + h;
                    if (score < upperBound) {
                        upperBound = score;
                        printf("new upperBound: %f, nodes expanded: %d, open list size: %d\n", upperBound, nodesExpanded, openList.size());
                    }
                    continue;
                }
#endif

                // check for pruning
                if (upperBound < g + h) {
                    continue;
                }
                
                // check for pruning
                if (goal->getG() < g + h) {
                    continue;
                }

                // create the new node
                succ = new Node(g, h, newVariables, leaf);

                // add it to the open list
                openList.push(succ);

                // and to the list of generated nodes
                generated[newVariables] = succ;
                continue;
            }

            // so we have generated a node we have seen before
            // see if the new path is better
            if (g < succ->getG()) {
                // the update the information
                succ->setLeaf(leaf);
                succ->setG(g);

                // and the priority queue
                // check if it was already there
                if (succ->getPqPos() > -1) {
                    openList.update(succ);
                } else {
                    openList.push(succ);
                }
            }
        }
    }
}

void search() {
    printf("URLearning, Anytime Window A*\n");
    printf("Dataset: '%s'\n", scoreFile.c_str());
    printf("Net file: '%s'\n", netFile.c_str());
    printf("Best score calculator: '%s'\n", bestScoreCalculator.c_str());
    printf("Heuristic type: '%s'\n", heuristicType.c_str());
    printf("Heuristic argument: '%s'\n", heuristicArgument.c_str());
    
    printf("Reading score cache.\n");
    scoring::ScoreCache cache;
    cache.read(scoreFile);
    variableCount = cache.getVariableCount();

    printf("Creating BestScore calculators.\n");
    spgs = bestscorecalculators::create(bestScoreCalculator, cache);
    
    printf("Creating heuristic.\n");
    heuristic = heuristics::create(heuristicType, heuristicArgument, spgs);

    // initialized the generated node list
    init_map(generated);

    // create the start node and push it to the open list
    VARSET_NEW(empty, variableCount);
    byte leaf(0);
    
    upperBound = std::numeric_limits<float>::max();

    bool complete = false;
    float lb = heuristic->h(empty, complete);
    Node *start = new Node(0.0f, lb, empty, leaf);
    openList.push(start);
    generated[empty] = start;

    // initialize the goal node and add it to the closed list
    VARSET_NEW(allVariables, variableCount);
    VARSET_SET_ALL(allVariables, variableCount);
    goal = new Node(std::numeric_limits<float>::max(), 0.0f, allVariables, leaf);
    generated[allVariables] = goal;

    // for sorting the frozen nodes
    CompareNodeStarLexicographic cnsl;

    // start the search
    printf("Beginning search\n");
    boost::timer::auto_cpu_timer act;

    act.start();
    nodesExpanded = 0;
    int windowSize = 0;
    while (openList.size() > 0 && !outOfTime) {
        windowSearch(windowSize);
        updateLowerBound();

        printf("Window size: %d, upper bound: %f, lower bound: %f, nodes expanded: %d\n", windowSize, goal->getG(), lowerBound, nodesExpanded);
        act.report();

        // update open nodes to show they are no longer open
        for (auto iter = openList.begin(); iter != openList.end(); iter++) {
            (*iter)->setPqPos(-2);
        }
        openList.clearNoDelete();

        // move non-duplicate frozen nodes to open

        // sort lexicographically, then by f-cost, to detect duplicates
        std::sort(frozen.begin(), frozen.end(), cnsl);
        VARSET_NEW(prev, variableCount);
        for (auto iter = frozen.begin(); iter != frozen.end(); iter++) {
            // skip duplicates
            if ((*iter)->getSubnetwork() == prev) continue;

            // only add nodes that would not be pruned
            if (goal->getG() > (*iter)->getF()) {
                openList.push(*iter);
            }
        }

        // clear the frozen list
        frozen.clear();

        // increase the window size
        windowSize++;

    }
    act.stop();
    act.report();

    printf("Nodes expanded: %d\n", nodesExpanded);

    if (goal != NULL) {
        printf("Found solution: %f\n", goal->getF());

        if (netFile.length() > 0) {

            datastructures::BayesianNetwork *network = cache.getNetwork();
            network->fixCardinalities();
            std::vector<varset> optimalParents = reconstructSolution(goal);
            network->setParents(optimalParents);
            network->setUniformProbabilities();

            fileio::HuginStructureWriter writer;
            writer.write(network, netFile);
        }
    } else {
        Node *u = openList.pop();
        printf("No solution found.\n");
        printf("Lower bound: %f\n", u->getF());
    }
}

int main(int argc, char** argv) {
    boost::timer::auto_cpu_timer act;

    std::string description = std::string("Learn an optimal Bayesian network using Anytime Window A*.  Example usage: ") + argv[0] + " iris.pss";
    po::options_description desc(description);

    desc.add_options()
            ("scoreFile", po::value<std::string > (&scoreFile)->required(), "The file containing the local scores in pss format. First positional argument.")
            ("bestScore,b", po::value<std::string > (&bestScoreCalculator)->default_value("list"), "The data structure to use for BestScore calculations. [\"list\", \"tree\", \"bitwise\"]")
            ("heuristic,e", po::value<std::string> (&heuristicType)->default_value("static"), "The type of heuristic to use. [\"static\", \"dynamic_optimal\", \"dynamic\"]")
            ("argument,a", po::value<std::string> (&heuristicArgument)->default_value("2"), "The argument for creating the heuristic, such as number of pattern databases to use.")
            ("runningTime,r", po::value<int> (&runningTime)->default_value(0), "The maximum running time for the algorithm.  0 means no running time.")
            ("netFile,n", po::value<std::string > (&netFile)->default_value(""), "The file to which the learned network is written.  Leave blank to not create the file.")
            ("help,h", "Show this help message.")
            ;

    po::positional_options_description positionalOptions;
    positionalOptions.add("scoreFile", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc)
            .positional(positionalOptions).run(),
            vm);

    if (vm.count("help") || argc == 1) {
        std::cout << desc;
        return 0;
    }

    po::notify(vm);
    outOfTime = false;

    boost::asio::deadline_timer t(io);
    if (runningTime > 0) {
        printf("Maximum running time: %d\n", runningTime);
        t.expires_from_now(boost::posix_time::seconds(runningTime));
        t.async_wait(timeout);
        boost::thread workerThread(search);
        io.run();
        workerThread.join();
    } else {
        search();
    }

    return 0;
}

