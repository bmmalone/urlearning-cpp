/* 
 * File:   main.cpp
 * Author: malone
 *
 * Created on August 6, 2012, 9:05 PM
 */

//#define PRINT_SUBNETWORK_SCORES
#define DEBUG

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

//#include "urlearning/bfbnb_hash/previous_layer_stream.cpp"
//#include "urlearning/bfbnb_hash/merged_temp_file.cpp"
#include "previous_layer_stream.cpp"
#include "merged_temp_file.cpp"

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
class FileHandleWrite {
public:
    std::string filename;
    std::ofstream filestream;

    FileHandleWrite(std::string filename, bool append) : filename(filename){
        if (append) filestream.open(filename.c_str(), std::ios::out|std::ios::binary|std::ios::app);
        else        filestream.open(filename.c_str(), std::ios::out|std::ios::binary);
    }
    ~FileHandleWrite() {
        filestream.close();
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
void writeNode(Node *u, std::ofstream& of) {
    u->save(of);
}
std::string getTempFilename(int layer, int tempFileIndex) {
    std::string tempFilename = "layer_" + TO_STRING(layer) + "_temp_" + TO_STRING(tempFileIndex);
    return tempFilename;
}
/**
* Calculates the binomial coefficient, $\choose{n, k}$, i.e., the number of
* distinct sets of $k$ elements that can be sampled with replacement from a
* population of $n$ elements.
*
* @tparam T
* Numeric type. Defaults to unsigned long.
* @param n
* Population size.
* @param k
* Number of elements to sample without replacement.
*
* @return
* The binomial coefficient, $\choose{n, k}$.
*
* @note
* Modified from: http://etceterology.com/fast-binomial-coefficients
* 
* Copied from: https://gist.github.com/jeetsukumaran/5392166
*/
unsigned long binomial_coefficient(unsigned long n, unsigned long k) {
    unsigned long i;
    unsigned long b;
    if (0 == k || n == k) {
        return 1;
    }
    if (k > n) {
        return 0;
    }
    if (k > (n - k)) {
        k = n - k;
    }
    if (1 == k) {
        return n;
    }
    b = 1;
    for (i = 1; i <= k; ++i) {
        b *= (n - (k - i));
        if (b < 0) return -1; /* Overflow */
        b /= i;
    }
    return b;
}
unsigned long binomial_coefficient_mod(long n, long k) {
    if (n < 0 || k < 0 || n < k) return 0;
    return binomial_coefficient(n,k);
}
/**
 * Determines how many variables are used from the variable set 
 * to determine into which file the variable set should belong to.
 * 
 * @param maxNodes
 * @param numberVariables
 * @param level
 * @return the number of variables used to determine a corresponding 
 * file for a variable set
 */
unsigned long determineTempFileVariableAmount(unsigned long maxNodes, 
        unsigned long numberVariables, unsigned long level){
    unsigned long increment = 1;
    #ifdef DEBUG
	std::cout<<"before numberVariables="<<numberVariables
            <<",level="<<level
            <<",maxnodes="<<maxNodes
            <<std::endl;
    #endif
    while(level-increment > 1 
            && binomial_coefficient(numberVariables-increment,level-increment) > maxNodes){
        increment += 1;
    }
    #ifdef DEBUG
	std::cout<<"increment will be "<<increment<<std::endl;
    #endif
    return increment;
}
/**
 * Creates new files and if needed writes over the old ones.
 * 
 * Creating combinations based on http://stackoverflow.com/questions/9430568/generating-combinations-in-c
 * 
 * @param closedList
 * @param layer
 * @param n
 * @param r
 */
void createTempFileNames(int layer, int n, int r, StringMap &tempFiles, int variableCount){
    std::vector<bool> v(n);
    std::fill(v.begin() + r, v.end(), true);
    do {
        std::string fileName = "layer."+TO_STRING(layer);
        VARSET_NEW(fileNameVars,variableCount);
        for (int i = 0; i < n; ++i) {
            if (!v[i]) {
                fileName += "_"+TO_STRING(i);
                VARSET_SET(fileNameVars,i);
            }
        }
        fileName += ".tempFile";
        tempFiles[fileNameVars] = fileName;

    } while (std::next_permutation(v.begin(), v.end()));
}
/*
 * Calculates hamming weight of an integer, in this case
 * the number of variables in a varset. 
 * 
 * @param n
 * @return 
 */
int hammingWeight (varset vs) {
   int count = 0;
   ulong n = VARSET_TO_LONG(vs);
   while (n) {
      count += n & 0x1u;
      n >>= 1;
   }
   return count;
}
/**
 * TODO Write proper test cases.
 * 
 * @param in
 * @param bitCount is number of bits that need to be set
 * @return 
 */
varset determineTempFileVarset(varset in, int bitCount, int variableCount){
    VARSET_NEW(out,variableCount);

    std::cout<<"in "<<in<<std::endl;
    std::cout<<"out "<<out<<std::endl;
    
    int lastSet = VARSET_FIND_FIRST_SET(in);
    
    std::cout<<"lastSet "<<lastSet<<std::endl;
    
    // in case there is no set nodes in the start
    if (lastSet<0) return out; 
    
    VARSET_SET(out,lastSet);
    for(int i=0; i<bitCount-1; i++){
        lastSet = VARSET_FIND_NEXT_SET(in,lastSet);
        VARSET_SET(out,lastSet);
    }
    return out;
}
/**
 * Open temp files for append.
 * 
 * Note there is a upper limit to the number of file handles, something
 * around 1000, if that is exceeded we should do something else - like write
 * in batches, which only means that there will be more sweeps over the whole
 * map.
 * 
 * Note it seems every time you open a file a single empty row is appended 
 * also if no info is added. This means that files will contain extra information
 * even if they don't have anything interesting in them.
 * 
 * @param closedList
 * @param tempFiles
 */
void writeNodeMapToTempFiles(NodeMap &closedList, StringMap &tempFiles, int variableCount){
    int bitcount = hammingWeight(tempFiles.begin()->first);
    int maxSize = 500;
    boost::unordered_map<varset, FileHandleWrite*> fileHandles;
    for(auto kv : closedList){
        varset tempFileVarset = determineTempFileVarset(kv.first,bitcount,variableCount);
        auto fileHandleIter = fileHandles.find(tempFileVarset);
        if (fileHandleIter == fileHandles.end()){
            auto tempFileIter = tempFiles.find(tempFileVarset);
            if (tempFileIter == tempFiles.end()){
                throw std::runtime_error("writeNodeMapToTempFiles(): Couldn't find tempFile for varset " + varsetToString(tempFileVarset));
            }
            if (fileHandles.size() > maxSize) {
                // heuristic used for beginning to avoid opening too many files
                // at the same time
                std::cout<<"writeNodeMapToTempFiles() erasing all fileHandles as maxSize "<<maxSize<<" exceeded."<<std::endl;
                for(auto kv : fileHandles){
                    delete kv.second;
                }
                fileHandles.clear();
            }
            FileHandleWrite *handle = new FileHandleWrite(tempFileIter->second,true);
            fileHandles[tempFileVarset] = handle;
            writeNode(kv.second,handle->filestream);
            //std::cout<<"Wrote node "<<varsetToString(kv.first)<<" as "<<varsetToString(tempFileVarset)<<" to file "<<tempFileIter->second<<std::endl;
        } else {
            writeNode(kv.second,(*fileHandleIter).second->filestream);            
            //std::cout<<"Wrote node "<<varsetToString(kv.first)<<" as "<<varsetToString(tempFileVarset)<<" to file "<<(*fileHandleIter).second->filename<<std::endl;
        }
        delete kv.second;
    }
    // close file handles
    for(auto kv : fileHandles){
        delete kv.second;
    }
}
/**
 * http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
 * 
 * @param name
 * @return 
 */
inline bool fileExists(const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}
void readNodesFromTempFilesIntoMemoryAndDoDDD(StringMap &tempFiles, int variableCount){
    std::vector<std::string> handledFiles(0);
    std::vector<long> numberNodes(0);
    for(auto it = tempFiles.begin(); it != tempFiles.end();){
        //std::cout<<"Reading from tempfile "<<it->second<<std::endl;
        if (!(std::find(handledFiles.begin(), handledFiles.end(), it->second) != handledFiles.end())) {
            // we have not handled this file yet
            handledFiles.push_back(it->second);
            long numberNodesLayer = 0;
            StringMap fileNamesMap;
            if (!fileExists(it->second)) {
                it = tempFiles.erase(it);
                continue;
            }
            fileNamesMap[VARSET(variableCount)] = it->second;
            PreviousLayerStream pls(fileNamesMap);
            Node *next;
            NodeMap nodes;
            init_map(nodes);
            while ((next = pls.getNextNode(variableCount)) != NULL) {
                #ifdef DEBUG
		    std::cout<<"Before DDD: read node "<<varsetToString(next->getSubnetwork())<<std::endl;
                #endif
		numberNodesLayer++;
                auto found = nodes.find(next->getSubnetwork());
                if (found == nodes.end()){
                    nodes[next->getSubnetwork()] = next;
                } 
                else {
                    Node* existing = (*found).second;
                    if (next->getG() < existing->getG()) {
                        // new node better than existing
                        existing->setLeaf(next->getLeaf());
                        existing->setG(next->getG());
                    } 
                    delete next;
                }
            }
            // write on top of the old file on disk again
            std::ofstream tempFile(it->second, std::ios::binary);
            for(auto nodeIter : nodes){
                #ifdef DEBUG
		    std::cout<<"After DDD: Wrote node "<<varsetToString(nodeIter.first)<<" with score "<<nodeIter.second->getG()<<" to file "<<it->second<<std::endl;
                #endif 
		writeNode(nodeIter.second,tempFile);
                delete nodeIter.second;
            }
            nodes.clear();
            tempFile.close();
            numberNodes.push_back(numberNodesLayer);
        }
        ++it;
    }
    std::string filesString = "";
    long count = 0;
    for(int i=0; i<numberNodes.size(); i++) {
        filesString += TO_STRING(numberNodes[i]) + ",";
        count += numberNodes[i];
    }
    std::cout<<"Used "<<handledFiles.size()<<": "<<filesString<<" together "<<TO_STRING(count)<<std::endl;
}
void deleteEmptyTempFiles(StringMap &tempFiles){
    for(auto it = tempFiles.begin(); it != tempFiles.end();){
        if (!fileExists(it->second)) it = tempFiles.erase(it);
        else ++it;
    }
}
StringMap* mergeTempFiles(StringMap &map, int layer, int variableCount, long tempFileSubsetCount, int maxNodes){
    std::string out = "";
    out += "[";
    std::vector<MergedTempFile> mergedFiles(1);
    for(auto mapIter : map){
        varset set = mapIter.first;
        long binCoef = binomial_coefficient_mod(variableCount-(lastSetBit(set)+1),layer-tempFileSubsetCount);
        out += varsetToString(set) + "=" + mapIter.second +",";
        #ifdef DEBUG
	    std::cout<<varsetToString(set)<<","<<mapIter.second<<","
                <<"lastSetBit="<<lastSetBit(set)+1
                <<",variableCount="<<variableCount
                <<",layer="<<layer
                <<",tempFileSubsetCount="<<tempFileSubsetCount
                <<",binCoef="<<binCoef
                <<std::endl;
        #endif
	if (!mergedFiles.back().addTempFile(mapIter.second,set,binCoef,maxNodes)) {
            mergedFiles.emplace_back();
            mergedFiles.back().add(mapIter.second,set,binCoef);
        }
        else {
            #ifdef DEBUG
		std::cout<<"merged two files"<<std::endl;
            #endif
	}
    }
    StringMap *currentTempFiles = new StringMap();
    for(auto mergedFile : mergedFiles) {
        std::cout<<"mergedFile["<<mergedFile.tempFile<<","<<mergedFile.uniqueItemSum<<",{";
        for(auto set : mergedFile.varsets) {
            std::cout<<varsetToString(set)<<",";
            (*currentTempFiles)[set] = mergedFile.tempFile;
        }
        std::cout<<"}]"<<std::endl;
    }
    std::cout<<"currentTempFiles[";
    for(auto iter : *currentTempFiles) {
        std::cout<< varsetToString(iter.first)<<"="<<iter.second<<",";
    }
    std::cout<<"]"<<std::endl;
    out += "]";
    std::cout<<out<<std::endl;
    return currentTempFiles;
}
std::vector<MergedTempFile> mergeTempFilesGetFiles(StringMap &map, int layer, int variableCount, long tempFileSubsetCount, int maxNodes){
    std::string out = "";
    out += "[";
    std::vector<MergedTempFile> mergedFiles(1);
    for(auto mapIter : map){
        varset set = mapIter.first;
        long binCoef = binomial_coefficient_mod(variableCount-(lastSetBit(set)+1),layer-tempFileSubsetCount);
        #ifdef DEBUG
	    out += varsetToString(set) + "=" + mapIter.second +",";
       	    std::cout<<varsetToString(set)<<","<<mapIter.second<<","
                <<"lastSetBit="<<lastSetBit(set)+1
                <<",variableCount="<<variableCount
                <<",layer="<<layer
                <<",tempFileSubsetCount="<<tempFileSubsetCount
                <<",binCoef="<<binCoef
                <<std::endl;
        #endif
	if (!mergedFiles.back().addTempFile(mapIter.second,set,binCoef,maxNodes)) {
            std::cout<<"adding new merged file for "<<set<<std::endl;
            mergedFiles.emplace_back();
            mergedFiles.back().add(mapIter.second,set,binCoef);
        }
        else {
            #ifdef DEBUG
		std::cout<<"merged two files"<<std::endl;
            #endif
	}
    }
    std::cout<<out<<std::endl;
    return mergedFiles;
}

void deleteOldTempFiles(StringMap &tempFiles){
    for(auto mapIter : tempFiles){
        remove(mapIter.second.c_str());
    }
}
void deleteOldPreviousLayer(StringMap *previousTempFiles){
    deleteOldTempFiles(*previousTempFiles);
    if (previousTempFiles != NULL) previousTempFiles->clear();
    delete previousTempFiles;
    previousTempFiles = NULL;
}
void addNewNode(float oldG, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs,
        bool &complete, heuristics::Heuristic *heuristic, varset &newVariables, int variableCount, 
        int layer, Node *&goal, byte leaf, NodeMap &currentLayer) {
#ifdef DEBUG
    printf("first time we have generated this node\n");
#endif
    // get the cost along this path
    //float g = u->getG() + spgs[leaf]->getScore(newVariables);
    float g = oldG + spgs[leaf]->getScore(newVariables);
    // calculate the heuristic estimate (we don't really need to store h)
    complete = false;
    float h = heuristic->h(newVariables, complete);
    // prune it if possible
    if (g + h >= upperBound) return;
    // update all the values
    Node *succ = new Node(g, h, newVariables, leaf);
    // also, check if this is the last layer
    if (layer == variableCount - 1)  goal = new Node(g, h, newVariables, leaf);
    // and to the current layer
    currentLayer[newVariables] = succ;
#ifdef DEBUG
    std::cout<<"Added node "<<varsetToString(newVariables).c_str()<<"with score "<<g<<std::endl;
#endif
}
void expand(NodeMap &currentLayer,float oldG, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs,
        bool &complete, heuristics::Heuristic *heuristic, int variableCount, 
        int layer, Node *&goal, byte leaf, varset &variables)
{
    // get the new variable set
    VARSET_COPY(variables, newVariables);
    VARSET_SET(newVariables, leaf);
#ifdef PRINT_SUBNETWORK_SCORES
    printf("New varset: %s\n", varsetToString(newVariables).c_str());
#endif
    auto s = currentLayer.find(newVariables);
    // check if this is the first time we have generated this node
    if (s == currentLayer.end()) {
        addNewNode(oldG,spgs,complete,heuristic,newVariables,variableCount,layer,goal,leaf,currentLayer);
    } else {
        // so we have generated a node in the open list
        // see if the new path is better
        //float g = u->getG() + spgs[leaf]->getScore(variables);
        float g = oldG + spgs[leaf]->getScore(variables);
        Node *succ = (*s).second;
#ifdef DEBUG
        std::cout<<varsetToString(newVariables)<<std::endl;
        printf("Checking whether to update with g(%0.8f) < old_varset_g(%0.8f)\n",g,succ->getG());
#endif
        if (g < succ->getG()) {
#ifdef DEBUG
            printf("updated with %0.2f\n",g);
#endif                    
            // the update the information
            succ->setLeaf(leaf);
            succ->setG(g);

            // also, check if this is the last layer
            if (layer == variableCount - 1) {
                float h = heuristic->h(newVariables, complete);
                goal = new Node(g, h, newVariables, leaf);
            }
        } // end if new successor is better
    }
}
void updateCpuTimes(boost::timer::cpu_times &cpu_times, boost::timer::cpu_times with) {
    cpu_times.system = cpu_times.system + with.system;
    cpu_times.user = cpu_times.user + with.system;
    cpu_times.wall = cpu_times.wall + with.wall;
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
    
    boost::timer::auto_cpu_timer hashing_timer;
    boost::timer::cpu_times hashing_times;
    hashing_times.clear();
    
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

    VARSET_NEW(empty,variableCount);
    bool complete = false;
    float lb = heuristic->h(empty, complete);

    NodeMap currentLayer;
    init_map(currentLayer);

    byte leaf(0);
    Node *root = new Node(0.0f, lb, empty, leaf);
    Node *goal = NULL;

    // check if the upper bound is used
    if (upperBound <= 0) {
        upperBound = std::numeric_limits<float>::max();
    }

    act.start();
    printf("Beginning search\n");

    int previousLayerCount = 1;

    StringMap *currentTempFiles, *previousTempFiles = NULL;
    previousTempFiles = new StringMap();
    std::string layer0Name = "layer.0.tempFile";
    VARSET_NEW_INIT(layer0, variableCount, 0);
    (*previousTempFiles)[layer0] = layer0Name;
    std::ofstream rootStream(layer0Name, std::ios::binary);
    root->save(rootStream);
    rootStream.close();
    //delete root;
    
    std::vector<int> nodesWrittenToDisk;
    
    for (int layer = 0; layer < variableCount; layer++) {
        currentTempFiles = new StringMap();
        nodesExpanded.push_back(0);
        nodesWrittenToDisk.push_back(0);
        act.report();
        printf("Expanding layer: '%d'\n", layer);
        
        // Calculate the supercombination by merging supercombinations
        // on the next level and make sure that the amount of supercombinations
        // kept in the hash map does not exceed M

        // Divide the supercombinations using first-k subcombinations 
        StringMap *superCombinations = new StringMap();
        long superCombinationsSubsetCount = determineTempFileVariableAmount(maxNodes,variableCount,layer+1);
        createTempFileNames(layer,variableCount,superCombinationsSubsetCount,*superCombinations,variableCount);
        // we should here merge the super combinations in case it is possible
        std::vector<MergedTempFile> mergedFiles = mergeTempFilesGetFiles(*superCombinations,layer+1,variableCount,superCombinationsSubsetCount,maxNodes);
        // now we need to iterate over them and create the list of first-k subcombinations we are interested
        int bitcount = hammingWeight((*superCombinations).begin()->first);
        std::cout<<"Bitcout used is "<<bitcount<<std::endl;
        std::cout<<"mergedFiles "<<mergedFiles.size()<<std::endl;
        int rounds = 0;
        for(MergedTempFile file : mergedFiles)
        {
            std::cout<<"Looking at mergedFile "<<file.tempFile<<std::endl;
            rounds++;
            Node *next;
            if (layer == 0 || (file.uniqueItemSum != 0))
            {
                PreviousLayerStream pls(*previousTempFiles);
                #ifdef DEBUG 
			std::cout<<"Expanding "<<file.getVarsetsString()<<std::endl;
                #endif 
		while ((next = pls.getNextNode(variableCount)) != NULL) {
                    // Read in variables from previousTempFiles one by one
                    
                    if (rounds == 1) nodesExpanded[layer]++;
                    
                    float oldG = next->getG();
                    byte l = next->getH();
                    varset variables = next->getSubnetwork();
                    delete next;
                    
                    // we don't want to expand all of the combinations here, only part of them.
                    varset superCombinationVarset = determineTempFileVarset(variables,bitcount,variableCount);
                    std::cout<<"superCombinationVarset "<<superCombinationVarset<<std::endl;
                    // This should be the smallest index variable that is in the group or that
                    // can be added in to make the set belong to the group, right now just a hack
                    int minIndex = (layer == 0 ? 0 : file.getMinimumSetVariableInVarsetsIndex(superCombinationVarset, variableCount));
                    #ifdef DEBUG
			std::cout<<"looking for "<<varsetToString(superCombinationVarset)<<" minIndex "<<minIndex<<std::endl;
                    #endif 
		    if (layer == 0 || file.containsVarset(superCombinationVarset))
                    {
                        #ifdef DEBUG
			    std::cout<<"file contains "<<varsetToString(superCombinationVarset)<<std::endl;
                        #endif
			// 1. if a node on layer l contains the first-k subcombination, 
                        // then we should expand it by adding all the other variables not in the first-k 
                        // subcombination and not in the node yet except the ones that do not follow
                        // the first set variable lexicographically. 

                        // expand
                        for (byte leaf = minIndex; leaf < variableCount; leaf++) {
                            // make sure this variable was not already present
                            if (VARSET_GET(variables, leaf)) continue;
                            VARSET_COPY(variables, newVariables);
                            VARSET_SET(newVariables, leaf);
                            superCombinationVarset = determineTempFileVarset(newVariables,bitcount,variableCount);
                            if (layer == 0 || file.containsVarset(superCombinationVarset))
                            {
                                expand(currentLayer,oldG,spgs,complete,heuristic,variableCount,layer,goal,leaf,variables);
                            }
                        } // end for each successor
                    }
                    else
                    {
                        // Second, if a node on layer l does not contain the first-k subcombination:
                        //  *then we should expand only if the node can be made to contain the first-k 
                        //   subcombination by adding one variable, then we should add it. 
                        //    -this happens if we are using first-1 subcombinations on layer l+1 
                        //    -if the node already contains k-1 variables from the first-k subcombination
                        //  * except the ones that do not follow the first set variable lexicographically.
                        #ifdef DEBUG
			    std::cout<<"file does not contain yet"<<std::endl;
                        #endif
			// TODO in this case, we need to updte the minIndex with the 
                        for (byte leaf = minIndex; leaf < variableCount; leaf++) {
                            // make sure this variable was not already present
                            if (VARSET_GET(variables, leaf)) continue;
                            VARSET_COPY(variables, newVariables);
                            VARSET_SET(newVariables, leaf);
                            superCombinationVarset = determineTempFileVarset(newVariables,bitcount,variableCount);
                            if (file.containsVarset(superCombinationVarset))
                            {
                                expand(currentLayer,oldG,spgs,complete,heuristic,variableCount,layer,goal,leaf,variables);
                            }
                        } // end for each successor
                    }
                    // check if we have too many nodes in the map
                    hashing_timer.start();
                    if (currentLayer.size() > maxNodes) {
                        std::cout<<"About to write to disk as currentLayer.size() "<<currentLayer.size()<<" while maxnodes "<<maxNodes<<std::endl;
                        if (currentTempFiles->size() == 0) {
                            long tempFileSubsetCount = determineTempFileVariableAmount(maxNodes,variableCount,layer+1);
                            createTempFileNames(layer,variableCount,tempFileSubsetCount,*currentTempFiles,variableCount);
                            // we should here merge the tempfiles in case it is possible
                            currentTempFiles = mergeTempFiles(*currentTempFiles,layer+1,variableCount,tempFileSubsetCount,maxNodes);
                        }
                        writeNodeMapToTempFiles(currentLayer,*currentTempFiles,variableCount);

                        nodesWrittenToDisk[layer] = nodesWrittenToDisk[layer] + currentLayer.size();
                        // this makes us to loose all the information we know about the duplicates
                        currentLayer = NodeMap();
                        init_map(currentLayer);
                    }
                    hashing_timer.stop();
                    updateCpuTimes(hashing_times, hashing_timer.elapsed());
                } // end for each node in the previous layer
            }
            if (layer == 0) break; // no need to continue here
            // write to disk to avoid interfering next layer
            std::cout<<"Emptying hash table for next first-k subcombination with size "<<currentLayer.size()<<std::endl;
            hashing_timer.start();
            if (currentTempFiles->size() == 0) {
                long tempFileSubsetCount = determineTempFileVariableAmount(maxNodes,variableCount,layer+1);
                createTempFileNames(layer,variableCount,tempFileSubsetCount,*currentTempFiles,variableCount);
                // we should here merge the tempfiles in case it is possible
                currentTempFiles = mergeTempFiles(*currentTempFiles,layer+1,variableCount,tempFileSubsetCount,maxNodes);
            }
            writeNodeMapToTempFiles(currentLayer,*currentTempFiles,variableCount);

            nodesWrittenToDisk[layer] = nodesWrittenToDisk[layer] + currentLayer.size();
            // this makes us to loose all the information we know about the duplicates
            currentLayer = NodeMap();
            init_map(currentLayer);
            hashing_timer.stop();
            updateCpuTimes(hashing_times, hashing_timer.elapsed());
        }
        std::cout<<"Did "<<rounds<<" rounds on layer "<<layer<<std::endl;
	// either write the entire layer to disk or perform DDD
        hashing_timer.start();
        if (currentTempFiles->size() == 0) {
            //the layer fit in RAM, so just write it all out
            createTempFileNames(layer,variableCount,1,*currentTempFiles,variableCount);
            writeNodeMapToTempFiles(currentLayer,*currentTempFiles,variableCount);
            nodesWrittenToDisk[layer] = nodesWrittenToDisk[layer] + currentLayer.size();
            deleteEmptyTempFiles(*currentTempFiles);
        } else {
            //there are temp files, so write the remaining nodes to a temp file and do ddd
            writeNodeMapToTempFiles(currentLayer,*currentTempFiles,variableCount);
            nodesWrittenToDisk[layer] = nodesWrittenToDisk[layer] + currentLayer.size();
            readNodesFromTempFilesIntoMemoryAndDoDDD(*currentTempFiles,variableCount);
        }
        hashing_timer.stop();
        updateCpuTimes(hashing_times, hashing_timer.elapsed());
        
        // and we can delete the previous layer
        deleteOldPreviousLayer(previousTempFiles);
        previousTempFiles = currentTempFiles;
#ifdef PD_STATISTICS
        printf("Static pattern database uses: '%d'\n", ((heuristics::CombinedPatternDatabase*)heuristic)->staticUses);
        printf("Dynamic pattern database uses: '%d'\n", ((heuristics::CombinedPatternDatabase*)heuristic)->dynamicUses);

        ((heuristics::CombinedPatternDatabase*)heuristic)->staticUses = 0;
        ((heuristics::CombinedPatternDatabase*)heuristic)->dynamicUses = 0;
#endif
        currentLayer = NodeMap();
        init_map(currentLayer);
    } // end for each layer
    deleteOldPreviousLayer(previousTempFiles);    
    act.stop();
    act.report();

    std::cout<<"Hashing timer - "<<boost::timer::format(hashing_times,4)<<std::endl;
    
    VARSET_NEW(allVariables, variableCount);
    VARSET_SET_ALL(allVariables, variableCount);

    int totalExpanded = 0, totalWritten = 0;
    printf("Layer,Nodes expanded, Nodes written to disk\n");
    for (int layer = 0; layer < variableCount; layer++) {
        printf("%d,%d,%d\n", layer, nodesExpanded[layer], nodesWrittenToDisk[layer]);
        totalExpanded += nodesExpanded[layer];
        totalWritten += nodesWrittenToDisk[layer];
    }
    
    printf("All,%d,%d\n\n", totalExpanded, totalWritten);

    if (goal != NULL) {
        printf("Found solution: %f\n", goal->getF());

        if (netFile.length() > 0) {

            datastructures::BayesianNetwork *network = cache.getNetwork();
            network->fixCardinalities();
            //std::vector<varset> optimalParents = reconstructSolution(goal, spgs, generatedNodes);
            //network->setParents(optimalParents);
            network->setUniformProbabilities();

            fileio::HuginStructureWriter writer;
            writer.write(network, netFile);
        }
    } else {
        printf("No solution found.\n");
    }
    delete goal;
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
        char* _emergencyMemory = new char[16384];
        try {
            bfbnb();
        } catch(std::bad_alloc ex) {
            // Delete the reserved memory so we can print an error message before exiting
            delete[] _emergencyMemory;
            std::cout << "Out of memory Exception detected!" << std::endl;
            exit(1);
        }
	delete[] _emergencyMemory;
    }
    return 0;
}

