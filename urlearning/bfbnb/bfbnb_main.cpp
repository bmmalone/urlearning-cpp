/* 
 * File:   main.cpp
 * Author: malone
 *
 * Created on August 6, 2012, 9:05 PM
 */

//#define PRINT_SUBNETWORK_SCORES

#include <string>
#include <iostream>
#include <ostream>
#include <queue>
#include <vector>
#include <cstdlib>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


#include "urlearning/base/node.h"
#include "urlearning/base/typedefs.h"

#include "urlearning/score_cache/score_cache.h"
#include "urlearning/score_cache/best_score_calculator.h"
#include "urlearning/score_cache/best_score_creator.h"

#include "urlearning/priority_queue/priority_queue.h"
#include "urlearning/fileio/hugin_structure_writer.h"


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
 * The upper bound used for pruning.  If this is not given, then this algorithm
 * is the same as dynamic programming.
 */
float upperBound;

/**
 * The maximum number of nodes to keep in the closed list for immediate
 * duplicate detection.  The string version makes entering scientific notation
 * easier.
 */
std::string maxNodesString;

/**
 * The maximum number of nodes to keep in the closed list for immediate
 * duplicate detection.
 */
int maxNodes;

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
 * The number of nodes expanded during each layer of the search.
 */
std::vector<int> nodesExpanded;

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

class TempFile {
    // <editor-fold defaultstate="collapsed" desc="Fields">
public:
    /**
     * The name of this temp file on disk.
     */
    std::string filename;
    /**
     * The number of nodes in this file.
     */
    int size;
    /**
     * The index of the current node (which has not yet been processed).
     */
    int currentIndex;
    /**
     * A reference to the current node. This node <em>has</em> been read in
     * from disk, but <em>has not</em> been processed, yet.
     */
    Node *currentNode;
    /**
     * The actual file from which we read.
     */
    boost::archive::binary_iarchive *file;
    std::ifstream filestream;
    // </editor-fold>

    //<editor-fold defaultstate="collapsed" desc="Constructors, Initializers and Deconstruction">

    TempFile(std::string filename, int size) {
        this->filename = filename;
        this->size = size;
        currentNode = new Node();
    }

    void initialize() {
        currentIndex = -1;

        // +1 because layer gives the currently expanding layer
        filestream.open(filename, std::ios::binary);
        file = new boost::archive::binary_iarchive(filestream);

        // and read the first node
        currentNode = new Node();
        next();
    }

    void close() {
        filestream.close();
        currentNode = NULL;
    }
    //</editor-fold>

    //<editor-fold defaultstate="collapsed" desc="Methods">

    /**
     * Compare two temp files based on the subnetwork of their current node.
     *
     * @param o the other file
     * @return the value {@code 0} if the files have the same subnetwork for
     * their current node, a value less than {@code 0} if the subnetwork of
     * {@link #currentNode} in {@code this} is less than that of {@code o},
     * and a value greater than {@code 0} otherwise
     */
    int compareTo(TempFile *o) {
        if (currentNode->getSubnetwork() < o->currentNode->getSubnetwork()) {
            return 1;
        } else if (currentNode->getSubnetwork() > o->currentNode->getSubnetwork()) {
            return -1;
        }
        return 0;
    }

    /**
     * Check if there is another node in the file.
     *
     * @return {@code true} if another file can be read in from this file
     */
    bool hasNext() {
        return (currentIndex != size);
    }

    /**
     * Advance to the next node in the file. Update {@link #currentIndex}
     * and {@link #currentNode} accordingly. If there are no more nodes,
     * then set {@link #currentNode} to {@code null}. This method assumes
     * there are no duplicates in the file.
     */
    void next() {
        currentIndex++;

        if (currentIndex == size) {
            currentNode = NULL;
            return;
        }

        float g;
        byte leaf;
        varset variables;

        (*file) >> g;
        (*file) >> leaf;
        (*file) >> variables;

        currentNode->setSubnetwork(variables);
        currentNode->setG(g);
        currentNode->setLeaf(leaf);
    }
    //</editor-fold>

};

class TempFileComparison {
public:

    bool operator() (TempFile *lhs, TempFile *rhs) {
        return (lhs->compareTo(rhs) < 0);
    }
};

std::vector<varset> reconstructSolution(Node *goal, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs) {
    throw std::runtime_error("BFBnB does not implement solution reconstruction.");

    std::vector<varset> optimalParents;
    for (int i = 0; i < spgs.size(); i++) {
        optimalParents.push_back(VARSET(spgs.size()));
    }

    VARSET_COPY(goal->getSubnetwork(), remainingVariables);
    Node *current = goal;
    float score = 0;
    for (int i = 0; i < spgs.size(); i++) {
        int leaf = current->getLeaf();
        score += spgs[leaf]->getScore(remainingVariables);
        varset parents = spgs[leaf]->getParents();
        optimalParents[leaf] = parents;

        VARSET_CLEAR(remainingVariables, leaf);

        //        current = findInFile(); closedList[remainingVariables];
    }

    return optimalParents;
}

void writeNode(Node *u, boost::archive::binary_oarchive *ar) {
    float g = u->getG();
    byte l = u->getLeaf();
    varset subnetwork = u->getSubnetwork();

    (*ar) << g;
    (*ar) << l;
    (*ar) << subnetwork;
}

std::string getTempFilename(int layer, int tempFileIndex) {
    std::string tempFilename = "layer_" + TO_STRING(layer) + "_temp_" + TO_STRING(tempFileIndex);
    return tempFilename;
}

void createTempFile(NodeMap &closedList, int layer, int tempFileIndex) {

    // get the keys and sort them
    std::vector<varset> keys;
    for (auto it = closedList.begin(); it != closedList.end(); it++) {
        keys.push_back((*it).first);
    }
    std::sort(keys.begin(), keys.end());

    // create the temp file
    std::string tempFilename = getTempFilename(layer, tempFileIndex);
    std::ofstream tempFile(tempFilename, std::ios::binary);
    boost::archive::binary_oarchive *temp = new boost::archive::binary_oarchive(tempFile);

    // write all of the nodes to disk
    for (auto subnetwork = keys.begin(); subnetwork != keys.end(); subnetwork++) {
        auto s = closedList.find(*subnetwork);
        Node *u = (*s).second;
        writeNode(u, temp);
        
        delete u;
    }

    delete temp;
    tempFile.close();
}

void writeEntireLayer(NodeMap &closedList, int layer, int &previousLayerCount) {
    // +1 because layer gives the currently expanding layer
    std::string curName = "layer_" + TO_STRING(layer + 1);
    std::ofstream curFile(curName, std::ios::binary);
    boost::archive::binary_oarchive *cur = new boost::archive::binary_oarchive(curFile);
    
    // sort them for debugging
    std::vector<varset> keys;
    for (auto it = closedList.begin(); it != closedList.end(); it++) {
        keys.push_back((*it).first);
    }
    std::sort(keys.begin(), keys.end());
    
    // write all of the nodes to disk
    previousLayerCount = 0;
    for (auto subnetwork = keys.begin(); subnetwork != keys.end(); subnetwork++) {
        auto s = closedList.find(*subnetwork);
        Node *u = (*s).second;
        writeNode(u, cur);
        previousLayerCount++;
        
        delete u;
    }

//    previousLayerCount = 0;
//    for (auto it = closedList.begin(); it != closedList.end(); it++) {
//        Node* u = (*it).second;
//        writeNode(u, cur);
//
//        previousLayerCount++;
//    }
    delete cur;
    curFile.close();
}

/**
 * Check the first element of each file and return the smallest element.
 * Advance that file to its next element. If we reach the end of the file,
 * then we will close it.
 *
 * @return the smallest element
 */
void getNext(Node *smallest, std::priority_queue<TempFile*, std::vector<TempFile*>, TempFileComparison> &pq) {
    // get the next thing off of the priority queue
    TempFile *smallestFile = pq.top();
    pq.pop();
    smallest->copy(smallestFile->currentNode);

    // move to the next element of that file
    smallestFile->next();

    // reinsert into the pq, but only if not at the end of that file
    if (smallestFile->hasNext()) {
        pq.push(smallestFile);
    } else {
        // otherwise, just close the file
        smallestFile->close();
        // and clean up the pointer
        delete smallestFile;
    }
}

void externalMemoryMerge(int layer, std::vector<int> &tempFileSizes, int &previousLayerCount) {


    // +1 because layer gives the currently expanding layer
    std::string curName = "layer_" + TO_STRING(layer + 1);
    std::ofstream curFile(curName, std::ios::binary);
    boost::archive::binary_oarchive *cur = new boost::archive::binary_oarchive(curFile);

    previousLayerCount = 0;

    std::priority_queue<TempFile*, std::vector<TempFile*>, TempFileComparison> pq;

    // add all of the temp files to the priority queue
    for (int i = 0; i < tempFileSizes.size(); i++) {
        if (tempFileSizes[i] > 0) {
            TempFile *tempFile = new TempFile(getTempFilename(layer, i), tempFileSizes[i]);
            tempFile->initialize();
            pq.push(tempFile);
        }
    }

    // get the first element
    Node *current = new Node();
    getNext(current, pq);
    Node *smallest = new Node();

    while (pq.size() > 0) {
        // find the next smallest element
        getNext(smallest, pq);
        
#ifdef DEBUG
//        printf("Name of the current smallest file: %s\n", pq.top()->filename.c_str());
#endif
#ifdef DEBUG
//        printf("current: %" PRIu64 ", smallest: %" PRIu64 "\n", current->getSubnetwork(), smallest->getSubnetwork());
#endif
        
        if (current->getSubnetwork() == smallest->getSubnetwork()) {
            // is this better than the previous value for this index
            if (smallest->getG() < current->getG()) {
                current->copy(smallest);
            }
        } else {
            // write the previous element to disk
            writeNode(current, cur);

            // and move to the next node we found
            previousLayerCount++;
            current->copy(smallest);
        }
    }

    // write the last element to disk
    writeNode(current, cur);

    // and move to the next node we found
    previousLayerCount++;

    // and close the sorted output file
    curFile.close();

    // delete the temp files
    for (int i = 0; i < tempFileSizes.size(); i++) {
        if (tempFileSizes[i] > 0) {
            std::string filename = getTempFilename(layer, i);
            remove(filename.c_str());
        }
    }

}

void bfbnb() {
    printf("URLearning, BFBnB\n");
    printf("This version DOES NOT RECONSTRUCT THE OPTIMAL SOLUTION.\n");
    printf("Dataset: '%s'\n", scoreFile.c_str());
    printf("Net file: '%s'\n", netFile.c_str());
    printf("Upper bound: '%f'\n", upperBound);
    printf("Max nodes: '%d'\n", maxNodes);
    printf("Best score calculator: '%s'\n", bestScoreCalculator.c_str());
    printf("Heuristic type: '%s'\n", heuristicType.c_str());
    printf("Heuristic argument: '%s'\n", heuristicArgument.c_str());

    boost::timer::auto_cpu_timer act;

    printf("Reading score cache.\n");
    scoring::ScoreCache cache;
    cache.read(scoreFile);
    int variableCount = cache.getVariableCount();

    act.start();
    printf("Creating BestScore calculators.\n");
    std::vector<bestscorecalculators::BestScoreCalculator*> spgs = bestscorecalculators::create(bestScoreCalculator, cache);
    act.stop();
    act.report();

    act.start();
    printf("Creating heuristic.\n");
    heuristics::Heuristic *heuristic = heuristics::create(heuristicType, heuristicArgument, spgs);

#ifdef DEBUG
    heuristic->print();
#endif

    act.stop();
    act.report();


    VARSET_NEW(empty, variableCount);
    bool complete = false;
    float lb = heuristic->h(empty, complete);

    NodeMap currentLayer;
    init_map(currentLayer);

    byte leaf(0);
    Node *root = new Node(0.0f, lb, empty, leaf);
    Node *goal = NULL;

    // write node to disk
    std::ofstream firstFile("layer_0", std::ios::binary);
    boost::archive::binary_oarchive *first = new boost::archive::binary_oarchive(firstFile);
    writeNode(root, first);
    delete first;
    firstFile.close();

    // check if the upper bound is used
    if (upperBound <= 0) {
        upperBound = std::numeric_limits<float>::max();
    }

    act.start();
    printf("Beginning search\n");

    int previousLayerCount = 1;

    for (int layer = 0; layer < variableCount; layer++) {
        nodesExpanded.push_back(0);

        act.report();
        printf("Expanding layer: '%d'\n", layer);

        std::vector<int> tempFileSizes;

        std::string prevName = "layer_" + TO_STRING(layer);
        std::ifstream prevFile(prevName, std::ios::binary);
        boost::archive::binary_iarchive prev(prevFile);

        for (int i = 0; i < previousLayerCount; i++) {
            float oldG;
            byte l;
            varset variables;

            prev >> oldG;
            prev >> l;
            prev >> variables;
            
#ifdef PRINT_SUBNETWORK_SCORES
            printf("%s&%.2f \\\\\n", varsetToString(variables).c_str(), oldG);
#endif

            //varset variables = u->getSubnetwork();
            nodesExpanded[layer]++;

            // expand
            for (byte leaf = 0; leaf < variableCount; leaf++) {
                // make sure this variable was not already present
                if (VARSET_GET(variables, leaf)) continue;

                // get the new variable set
                VARSET_COPY(variables, newVariables);
                VARSET_SET(newVariables, leaf);


                auto s = currentLayer.find(newVariables);
                // check if this is the first time we have generated this node
                if (s == currentLayer.end()) {
                    // get the cost along this path
                    //float g = u->getG() + spgs[leaf]->getScore(newVariables);
                    float g = oldG + spgs[leaf]->getScore(newVariables);

                    // calculate the heuristic estimate (we don't really need to store h)
                    complete = false;
                    float h = heuristic->h(newVariables, complete);

#ifdef CHECK_COMPLETE
                    if (complete) {
                        if (upperBound > g + h) {
                            upperBound = g + h;
                            printf("new upperBound: %f\n", upperBound);
                        }
                        continue;
                    }
#endif

                    // prune it if possible
                    if (g + h >= upperBound) continue;

                    // update all the values
                    Node *succ = new Node(g, h, newVariables, leaf);

                    // also, check if this is the last layer
                    if (layer == variableCount - 1) {
                        goal = new Node();
                        goal->copy(succ);
                    }

                    // and to the current layer
                    currentLayer[newVariables] = succ;
                    continue;
                }

                // so we have generated a node in the open list
                // see if the new path is better
                //float g = u->getG() + spgs[leaf]->getScore(variables);
                float g = oldG + spgs[leaf]->getScore(variables);
                Node *succ = (*s).second;
                if (g < succ->getG()) {
                    // the update the information
                    succ->setLeaf(leaf);
                    succ->setG(g);
                    
                    if (goal != NULL) {
                        goal->copy(succ);
                    }
                } // end if new successor is better
            } // end for each successor

            // check if we have too many nodes in the map
            if (currentLayer.size() > maxNodes) {
                createTempFile(currentLayer, layer, tempFileSizes.size());
                tempFileSizes.push_back(currentLayer.size());

                currentLayer = NodeMap();
                init_map(currentLayer);
            }
        } // end for each node in the previous layer

        // either write the entire layer to disk or perform DDD
        if (tempFileSizes.size() == 0) {
            // the layer fit in RAM, so just write it all out
            writeEntireLayer(currentLayer, layer, previousLayerCount);
        } else {
            // there are temp files, so write the remaining nodes to a temp file
            createTempFile(currentLayer, layer, tempFileSizes.size());
            tempFileSizes.push_back(currentLayer.size());

            // and perform the external memory merge
            externalMemoryMerge(layer, tempFileSizes, previousLayerCount);
        }
        
        // and we can delete the previous layer
        remove(prevName.c_str());



#ifdef PD_STATISTICS
        printf("Static pattern database uses: '%d'\n", ((heuristics::CombinedPatternDatabase*)heuristic)->staticUses);
        printf("Dynamic pattern database uses: '%d'\n", ((heuristics::CombinedPatternDatabase*)heuristic)->dynamicUses);

        ((heuristics::CombinedPatternDatabase*)heuristic)->staticUses = 0;
        ((heuristics::CombinedPatternDatabase*)heuristic)->dynamicUses = 0;
#endif

        currentLayer = NodeMap();
        init_map(currentLayer);
    } // end for each layer
    
    // delete the last layer
    std::string filename = "layer_" + TO_STRING(variableCount);
    remove(filename.c_str());
    
    act.stop();
    act.report();

    VARSET_NEW(allVariables, variableCount);
    VARSET_SET_ALL(allVariables, variableCount);

    int totalExpanded = 0;
    printf("Layer,Nodes expanded\n");
    for (int layer = 0; layer < variableCount; layer++) {
        printf("%d,%d\n", layer, nodesExpanded[layer]);
        totalExpanded += nodesExpanded[layer];
    }
    printf("All,%d\n\n", totalExpanded);

    if (goal != NULL) {
        printf("Found solution: %f\n", goal->getG());

        if (netFile.length() > 0) {
            
            printf("ERROR: The current implementation of BFBnB does not support solution reconstruction.\n");

            datastructures::BayesianNetwork *network = cache.getNetwork();
            network->fixCardinalities();
            //            std::vector<varset> optimalParents = reconstructSolution(goal, spgs, generatedNodes);
            //            network->setParents(optimalParents);
            network->setUniformProbabilities();

            fileio::HuginStructureWriter writer;
            writer.write(network, netFile);
        }
    } else {
        printf("No solution found.\n");
    }
    
    for (auto spg = spgs.begin(); spg != spgs.end(); spg++) {
        delete *spg;
    }
    
    //delete heuristic;
}

int main(int argc, char** argv) {
    boost::timer::auto_cpu_timer act;

    std::string description = std::string("Learn an optimal Bayesian network using BFBnB.  Example usage: ") + argv[0] + " iris.pss";
    po::options_description desc(description);

    desc.add_options()
            ("scoreFile", po::value<std::string > (&scoreFile)->required(), "The file containing the local scores in pss format. First positional argument.")
            ("upperBound,u", po::value<float> (&upperBound)->default_value(0.0f), "The upper bound to use for pruning.  0 means no pruning.")
            ("bestScore,b", po::value<std::string > (&bestScoreCalculator)->default_value("list"), "The data structure to use for BestScore calculations. [\"list\", \"tree\", \"bitwise\"]")
            ("maxNodes,m", po::value<std::string > (&maxNodesString)->default_value("1E6"), "The maximum number of nodes to keep in memory for immediate duplicate detection.")
            ("heuristic,e", po::value<std::string > (&heuristicType)->default_value("static"), "The type of heuristic to use. [\"static\", \"dynamic_optimal\", \"dynamic\"]")
            ("argument,a", po::value<std::string > (&heuristicArgument)->default_value("2"), "The argument for creating the heuristic, such as number of pattern databases to use.")
            ("runningTime,r", po::value<int> (&runningTime)->default_value(0), "The maximum running time for the algorithm.  0 means no running time limit.")
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

    boost::to_lower(bestScoreCalculator);

    maxNodes = (int) (std::stod(maxNodesString));

    boost::asio::deadline_timer t(io);
    if (runningTime > 0) {
        printf("Maximum running time: %d\n", runningTime);
        t.expires_from_now(boost::posix_time::seconds(runningTime));
        t.async_wait(timeout);
        boost::thread workerThread(bfbnb);
        io.run();
        workerThread.join();
    } else {
        bfbnb();
    }

    return 0;
}

