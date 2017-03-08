/* 
 * File:   LongFloatScorer.h
 * Author: malone
 *
 * Created on August 7, 2012, 1:24 PM
 */

#ifndef LONGFLOATSCORER_H
#define	LONGFLOATSCORER_H

#include <vector>
#include <string>

#include <boost/unordered_map.hpp>

#include "urlearning/base/typedefs.h"
#include "urlearning/base/bayesian_network.h"

namespace datastructures {
    class BayesianNetwork;
}

namespace scoring {

    class ScoreCache {
    public:

        ScoreCache() {
            // should never be called
        }

        ScoreCache(std::string filename);
        ~ScoreCache();
        
        void setVariableCount(int variableCount);

        float getScore(int variable, varset parents) const {
            return (*cache[variable])[parents];
        }

        void putScore(int variable, varset parents, float score) {
            (*cache[variable])[parents] = score;
        }

        void removeScore(int variable, varset parents) {
            (*cache[variable]).erase(parents);
        }

        int getVariableCount() const {
            return variableCount;
        }

        FloatMap *getCache(int variable) const {
            return cache[variable];
        }
        
        std::string getMetaInformation(std::string key) {
            return metaInformation[key];
        }
        
        void updateMetaInformation(std::string key, std::string value) {
            metaInformation[key] = value;
        }
        
        datastructures::BayesianNetwork *getNetwork() {
            return network;
        }

        void deleteCache(int variable) {
            if (cache[variable] != NULL) {
                cache[variable]->clear();
            }

            delete cache[variable];
        }

        void read(std::string filename);
        void readUrlBinary(std::string filename);
        void write(std::string filename);
        int writeExclude(std::string filename, varset exclude);

    private:
        int variableCount;
        datastructures::BayesianNetwork *network;
        std::vector<int> variableCardinalities;
        std::vector < FloatMap * > cache;
        boost::unordered_map<std::string, std::string> metaInformation;
    };

}
#endif	/* LONGFLOATSCORER_H */

