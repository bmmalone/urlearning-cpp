#ifndef PREVIOUS_LAYER_STREAM_HPP
#define	PREVIOUS_LAYER_STREAM_HPP

#include <iostream>
#include <ostream>
#include <queue>
#include <vector>
#include <cstdlib>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/unordered/detail/buckets.hpp>

#include "urlearning/base/typedefs.h"
#include "urlearning/base/node.h"

/**
 * Not thread safe
 */
class PreviousLayerStream {
public:
    std::vector<std::string> fileNames;
    StringMap fileNamesMap;
    std::ifstream filestream;
    int fileIndex; // -1 --> already closed, -1 --> not yet opened, >-1 --> file open
    std::vector<std::string>::iterator iterator;
    StringMap::iterator mapIterator;
    
    PreviousLayerStream(StringMap &fileNamesMap) : fileNamesMap(fileNamesMap), fileIndex(-1) {
        for (auto kv : fileNamesMap) {
            //std::cout<<"Found file "<<kv.second<<std::endl;
            if (!(std::find(fileNames.begin(), fileNames.end(), kv.second) != fileNames.end())) {
                // we have not handled this file yet
                //std::cout<<"Added "<<kv.second<<std::endl;
                fileNames.push_back(kv.second);
            }
        }
        //std::cout<<"fileNames.size()=="<<fileNames.size()<<std::endl;
    }
    ~PreviousLayerStream() {
        if (fileIndex > -1) {
            closeOpenFile();
        }
        fileIndex = -2;
    }
    /**
     * Keeps track of files: opens and closes files when necessary to
     * provide next node. Returns NULL when all files have been circulated.
     * 
     * @return 
     */
    Node* getNextNode(int variableCount) {
        if (fileIndex < -1) return NULL;
        if (fileIndex == -1) {
            // first time every getNextNode is called
            fileIndex = 0;
            iterator = fileNames.begin();
            mapIterator = fileNamesMap.begin();
            if (fileIndex >= fileNames.size()) return NULL;
            openNewFile();
        }
        Node *out;
        while ((out = tryReadingFromFile(variableCount)) == NULL){
            // if we got NULL, it means that file has nothing more left
            closeOpenFile();
            fileIndex++;
            iterator++;
            mapIterator++;
            //std::cout<<"At this point fileIndex is "<<fileIndex<<std::endl;
            if (fileIndex >= fileNames.size()) return NULL;
            openNewFile();
        }
        return out;
    }
private:
    void closeOpenFile() {
        filestream.close();
    }
    void openNewFile() {
        //std::cout<<"About to open file "<<*iterator<<std::endl;
        filestream.open(*iterator, std::ios::binary);
    }
    Node* tryReadingFromFile(int variableCount) {
        while(!filestream.eof()) {
            // read into a hashmap
            Node *aux = new Node(variableCount);
            aux->load(filestream);
            if (filestream.eof()) {
	        delete aux;	
	        return NULL;
            }
            return aux;
        }
        return NULL;
    }
};

#endif //PREVIOUS_LAYER_STREAM_HPP
