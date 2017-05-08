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

//#define RUN_GOBNILP

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
 * The data structure to use to calculate best parent set scores.
 * "tree", "list", "bitwise"
 */
std::string bestScoreCalculator;

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
 * The ancestor-only variables for the search.
 * See UAI '14.
 */
std::string ancestorsArgument;       // csv list
varset ancestors;

/**
 * The variables for this search.
 */
std::string sccArgument;      //csv list
varset scc;

/**
 * The number of nodes expanded during the search.
 */
int nodesExpanded;

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

std::vector<varset> reconstructSolution(Node *goal, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, NodeMap &closedList) {
    std::vector<varset> optimalParents;
    for (int i = 0; i < spgs.size(); i++) {
        optimalParents.push_back(VARSET(spgs.size()));
    }

    VARSET_COPY(goal->getSubnetwork(), remainingVariables);
    Node *current = goal;
    float score = 0;
    int count = cardinality(scc);
    for (int i = 0; i < count; i++) {
        int leaf = current->getLeaf();
        score += spgs[leaf]->getScore(remainingVariables);
        varset parents = spgs[leaf]->getParents();
        optimalParents[leaf] = parents;

        VARSET_CLEAR(remainingVariables, leaf);

        current = closedList[remainingVariables];
    }

    return optimalParents;
}

void astar() {
    printf("URLearning, A*\n");
    printf("Dataset: '%s'\n", scoreFile.c_str());
    printf("Net file: '%s'\n", netFile.c_str());
    printf("Best score calculator: '%s'\n", bestScoreCalculator.c_str());
    printf("Heuristic type: '%s'\n", heuristicType.c_str());
    printf("Heuristic argument: '%s'\n", heuristicArgument.c_str());
    printf("Ancestors: '%s'\n", ancestorsArgument.c_str());
    printf("SCC: '%s'\n", sccArgument.c_str());

    boost::timer::auto_cpu_timer act;

    
    printf("Reading score cache.\n");
    scoring::ScoreCache cache;
    cache.read(scoreFile);
    printf("Done reading score cache.\n");
    int variableCount = cache.getVariableCount();
    printf("Variable count is %d\n",variableCount);
    // parse the ancestor and scc variables
    VARSET_NEW(ancestors,variableCount);
    VARSET_NEW(scc,variableCount);
    
    // if no scc was specified, assume we just want to learn everything
    if (sccArgument == "") {
        VARSET_SET_ALL(scc, variableCount);
    } else {
        setFromCsv(scc, sccArgument);
        setFromCsv(ancestors, ancestorsArgument);
    }
    
    
    act.start();
    printf("Creating BestScore calculators.\n");
    std::vector<bestscorecalculators::BestScoreCalculator*> spgs = bestscorecalculators::create(bestScoreCalculator, cache);
    act.stop();
    act.report();

    act.start();
    printf("Creating heuristic.\n");
    heuristics::Heuristic *heuristic = heuristics::createWithAncestors(heuristicType, heuristicArgument, spgs, ancestors, scc);

#ifdef DEBUG
    heuristic->print();
#endif

    act.stop();
    act.report();
    

    NodeMap generatedNodes;
    init_map(generatedNodes);

    PriorityQueue openList;

    byte leaf(0);
    Node *root = new Node(0.0f, 0.0f, ancestors, leaf);
    openList.push(root);
    
#ifdef DEBUG
    printf("The start is '%s'\n", varsetToString(ancestors).c_str());
    
    bool c;
    float lb = heuristic->h(ancestors, c);
    
    printf("The lb is %.2f\n", lb);
#endif

    Node *goal = NULL;
    VARSET_NEW(allVariables, variableCount);
    VARSET_SET_VALUE(allVariables, ancestors);
    allVariables = VARSET_OR(allVariables, scc);
    
#ifdef DEBUG
    printf("The goal is '%s'\n", varsetToString(allVariables).c_str());
#endif

    float upperBound = std::numeric_limits<float>::max();
    bool complete = false;

    act.start();
    printf("Beginning search\n");
    nodesExpanded = 0;
    while (openList.size() > 0 && !outOfTime) {
        //printf("top of while\n");
        Node *u = openList.pop();
        //printf("Expanding: '%s', g: '%.2f'\n", varsetToString(u->getSubnetwork()).c_str(), u->getG());
        nodesExpanded++;

        //        if (nodesExpanded % 100 == 0) {
        //            std::cout << ".";
        //            std::cout.flush();
        //        }

        varset variables = u->getSubnetwork();

        // check if it is the goal
        if (variables == allVariables) {
            goal = u;
            break;
        }

        // TODO: this is mostly broken
        if (u->getF() > upperBound) {
            break;
        }

        // note that it is in the closed list
        u->setPqPos(-2);

        // expand
        for (byte leaf = 0; leaf < variableCount; leaf++) {
            // make sure this variable was not already present
            if (VARSET_GET(variables, leaf)) continue;
            
            // also make sure this is one of the variables we care about
            if (!VARSET_GET(scc, leaf)) continue;

            // get the new variable set
            VARSET_COPY(variables, newVariables);
            VARSET_SET(newVariables, leaf);


            Node *succ = generatedNodes[newVariables];

            // check if this is the first time we have generated this node
            if (succ == NULL) {
                // get the cost along this path
                
                //printf("About to check for using leaf '%d', newVariables: '%s'\n", leaf, varsetToString(newVariables).c_str());
                
                float g = u->getG() + spgs[leaf]->getScore(newVariables);
                //printf("I have g\n");
                // calculate the heuristic estimate
                complete = false;
                float h = heuristic->h(newVariables, complete);
                //printf("I have h: %.2f\n", h);

#ifdef RUN_GOBNILP
                std::string filename = "exclude_" + TO_STRING(newVariables) + ".pss";
                time_t t;
                double seconds;

                t = time(NULL);
                int count = cache.writeExclude(filename, newVariables);
                seconds = difftime(time(NULL), t);

                // check whether to execute or not
                if (count < 10000 && variableCount > 25) {

                    // now, call gobnilp on the subproblem
                    FILE *fpipe;
                    char buffer[1048576];
                    char cmd[4096];
                    int i;

                    t = time(NULL);
                    snprintf(cmd, sizeof (cmd), "gobnilp %s", filename.c_str());

                    if (0 == (fpipe = (FILE*) popen(cmd, "r"))) {
                        perror("popen() failed.");
                        exit(1);
                    }

                    while ((i = fread(buffer, sizeof (char), sizeof buffer, fpipe))) {


                        /* We found some solutions, so split them up and add the cutting planes. */

                        // make sure to clean up the buffer so it behaves as a string
                        buffer[i] = 0;

                        //printf("buffer: '%s'\n", buffer);

                        char *pch;
                        pch = strtok(buffer, "\n");

                        bool optimal = false;

                        while (pch != NULL) {

                            // check if this is a "Dual Bound         : -4.62524687000000e+02" line

                            // I believe this is the only line in the output which starts with a 'D'
                            // this is really brittle, though
                            if (pch[0] == 'D') {
                                std::vector<std::string> tokens;
                                boost::split(tokens, pch, boost::is_any_of(" "), boost::token_compress_on);
                                float score = -1 * atof(tokens[3].c_str());
                                printf("Node: %s, g: %f, (%s) h(gobnilp): %f, f(gobnilp): %f, h(pd): %f, f(pd): %f\n",
                                        varsetToString(newVariables).c_str(), g, (optimal ? "optimal" : "bound"), score, (score + g), h, (g + h));
                                h = score;
                            }

                            // also, check if this line contains the word "optimal"
                            // if so, then this is actually the globally optimal solution
                            if (strstr(pch, "optimal") != NULL) {
                                optimal = true;
                            }

                            pch = strtok(NULL, "\n");
                        }
                    }
                    pclose(fpipe);

                    remove(filename.c_str());

                    seconds = difftime(time(NULL), t);
                    printf("Time to solve and parse ip: %f(s)\n", seconds);
                }

#endif

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

                // update all the values
                succ = new Node(g, h, newVariables, leaf);
                
                //printf("I have created succ\n");

                // add it to the open list
                openList.push(succ);
                
                //printf("pushed succ\n");

                // and to the list of generated nodes
                generatedNodes[newVariables] = succ;
                //printf("added succ to generatedNodes\n");
                continue;
            }

            // assume the heuristic is consistent
            // so check if it was in the closed list
            if (succ->getPqPos() == -2) {
                continue;
            }
            // so we have generated a node in the open list
            // see if the new path is better
            float g = u->getG() + spgs[leaf]->getScore(variables);
            if (g < succ->getG()) {
                // the update the information
                succ->setLeaf(leaf);
                succ->setG(g);

                // and the priority queue
                openList.update(succ);
            }
        }
    }
    act.stop();
    act.report();

    printf("Nodes expanded: %d, open list size: %d\n", nodesExpanded, openList.size());
    
    heuristic->printStatistics();

    if (goal != NULL) {
        //printf("Found solution: %f\n", goal->getF());
        printf("Found solution: %f\n", goal->getG());

        if (netFile.length() > 0) {

            datastructures::BayesianNetwork *network = cache.getNetwork();
            network->fixCardinalities();
            std::vector<varset> optimalParents = reconstructSolution(goal, spgs, generatedNodes);
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
    
    for (auto spg = spgs.begin(); spg != spgs.end(); spg++) {
        delete *spg;
    }
    
    for (auto pair = generatedNodes.begin(); pair != generatedNodes.end(); pair++) {
        delete (pair->second);
    }
    
    delete heuristic;
    
    io.stop();
}

int main(int argc, char** argv) {
    boost::timer::auto_cpu_timer act;
    srand(time(NULL));

    std::string description = std::string("Learn an optimal Bayesian network using A*.  Example usage: ") + argv[0] + " iris.pss";
    po::options_description desc(description);

    desc.add_options()
            ("score-file", po::value<std::string > (&scoreFile)->required(), "The file containing the local scores in pss format. First positional argument.")
            ("best-score,b", po::value<std::string > (&bestScoreCalculator)->default_value("list"), bestscorecalculators::bestScoreCalculatorString.c_str())
            ("heuristic,e", po::value<std::string > (&heuristicType)->default_value("static"), heuristics::heuristicTypeString.c_str())
            ("argument,a", po::value<std::string > (&heuristicArgument)->default_value("2"), heuristics::heuristicArgumentString.c_str())
            ("pc_{i-1},p", po::value<std::string > (&ancestorsArgument)->default_value(""), "Variables which can only be used as ancestors. They will not be added in the search. CSV-list of variable indices. See UAI '14.")
            ("scc_i,s", po::value<std::string > (&sccArgument)->default_value(""), "Variables which will be added in the search. CSV-list of variable indices. Leave blank to add all variables. See UAI '14.")
            ("running-time,r", po::value<int> (&runningTime)->default_value(0), "The maximum running time for the algorithm.  0 means no running time.")
            ("net-file,n", po::value<std::string > (&netFile)->default_value(""), "The file to which the learned network is written.  Leave blank to not create the file.")
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

    boost::to_lower(bestScoreCalculator);

    boost::asio::deadline_timer t(io);
    if (runningTime > 0) {
        printf("Maximum running time: %d\n", runningTime);
        t.expires_from_now(boost::posix_time::seconds(runningTime));
        t.async_wait(timeout);
        boost::thread workerThread(astar);
        io.run();
        workerThread.join();
    } else {
        astar();
    }

    return 0;
}

