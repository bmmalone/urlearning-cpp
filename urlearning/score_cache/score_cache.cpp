#include "score_cache.h"
#include "urlearning/base/files.h"
#include "urlearning/base/variable.h"

#include <fstream>
#include <iostream>

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
//
//scoring::ScoreCache::ScoreCache(int variableCount) {
//    this->variableCount = variableCount;
//
//    for (int i = 0; i < variableCount; i++) {
//        cache.push_back(new FloatMap());
//    }
//}


void scoring::ScoreCache::setVariableCount(int variableCount) {
    this->variableCount = variableCount;

    for (int i = 0; i < variableCount; i++) {
        cache.push_back(new FloatMap());
    }
}

std::vector<std::string> parse(std::string line, int start, std::string delimiters) {
    std::vector<std::string> tokens;
    std::string trimmedLine = line.substr(start);
    boost::trim(trimmedLine);
    boost::split(tokens, trimmedLine, boost::is_any_of(delimiters.c_str()), boost::token_compress_on);
    return tokens;
}

std::vector<std::string> parseMetaInformation(std::string line) {
    return parse(line, 4, "=");
}

std::vector<std::string> parseVariableValues(std::string line) {
    return parse(line, 0, " ,");
}

bool contains(std::string line, std::string str) {
    boost::algorithm::to_lower(line);
    boost::algorithm::to_lower(str);

    int index = line.find(str);

    return (index > -1);
}

void scoring::ScoreCache::read(std::string filename) {
    network = new datastructures::BayesianNetwork();
    
    std::ifstream in(filename.c_str());
    
    // make sure we found the file
    if (!in.is_open()) {
        throw std::runtime_error("Could not open the score cache file: '" + filename + "'");
    }
    
    std::vector<std::string> tokens;
    std::string line;

    // read meta information until we hit the first variable
    while (!in.eof()) {
        std::getline(in, line);

        // skip empty lines and comments
        if (line.size() == 0 || line.compare(0, 1, "#") == 0) {
            continue;
        }

        // check if we reached the first variable
        if (contains(line, "var ")) break;

        // make sure this is a meta line
        if (!contains(line, "meta")) {
            throw std::runtime_error("Error while parsing META information of network.  Expected META line or Variable.  Line: '" + line + "'");
        }

        tokens = parseMetaInformation(line);

        if (tokens.size() != 2) {
            throw std::runtime_error("Error while parsing META information of network.  Too many tokens.  Line: '" + line + "'");
        }

        boost::trim(tokens[0]);
        boost::trim(tokens[1]);
        updateMetaInformation(tokens[0], tokens[1]);
    }

    // line currently points to a variable name
    tokens = parse(line, 0, " ");
    datastructures::Variable *v = network->addVariable(tokens[1]);

    // read in the variable names
    while (!in.eof()) {
        std::getline(in, line);

        // skip empty lines and comments
        if (line.size() == 0 || line.compare(0, 1, "#") == 0) {
            continue;
        }

        if (contains(line, "meta")) {
            tokens = parseMetaInformation(line);

            if (contains(tokens[0], "arity")) {
                v->setArity(atoi(tokens[1].c_str()));
            } else if (contains(tokens[0], "values")) {
                std::vector<std::string> values = parseVariableValues(tokens[1]);
                v->setValues(values);
            } else {

                boost::trim(tokens[0]);
                boost::trim(tokens[1]);
                v->updateMetaInformation(tokens[0], tokens[1]);
            }
        }

        if (contains(line, "var ")) {
            tokens = parse(line, 0, " ");
            v = network->addVariable(tokens[1]);
        }
    }

    in.close();
    setVariableCount(network->size());

    // now that we have the variable names, read in the parent sets
    in.open(filename.c_str());
    while (!in.eof()) {
        std::getline(in, line);
        
        if (line.size() == 0 || line.compare(0, 1, "#") == 0 || contains(line, "meta")) {
            continue;
        }

        tokens = parse(line, 0, " ");
        if (contains(line, "var ")) {
            v = network->get(tokens[1]);
            continue;
        }

        // then parse the score for the current variable
        VARSET_NEW(parents, network->size());
        float score = -1 * atof(tokens[0].c_str()); // multiply by -1 to minimize

        for (int i = 1; i < tokens.size(); i++) {
            int index = network->getVariableIndex(tokens[i]);
            VARSET_SET(parents, index);
        }

        putScore(v->getIndex(), parents, score);
    }

    in.close();
}

void scoring::ScoreCache::readUrlBinary(std::string filename) {
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

    byte version = readByte(in);
    int maxParentCount = readInt(in);
    int recordCount = readInt(in);
    variableCount = readInt(in);

    network = new datastructures::BayesianNetwork(variableCount);

    for (int variable = 0; variable < variableCount; variable++) {
        /*   nameLength: int
         *      name: String, where each character is a byte
         *      cardinality: int
         *          value names: length, then String
         *      count: int
         *      scores: long, float
         */
        std::string name = readString(in);
        int cardinality = readInt(in);

        std::vector<std::string> values;
        for (int i = 0; i < cardinality; i++) {
            std::string value = readString(in);
            values.push_back(value);
        }

        datastructures::Variable *var = network->get(variable);
        var->setName(name);
        var->setValues(values);

        FloatMap *map = new FloatMap();
        init_map(map);
        cache.push_back(map);

        int count = readInt(in);
        for (int i = 0; i < count; i++) {
            // TODO: properly read and bitsets
            uint64_t l = readLong(in);
            VARSET_NEW_INIT(parents, variableCount, l);
            float score = readFloat(in);
            (*cache[variable])[parents] = score;
        }
    }

    in.close();
}

int scoring::ScoreCache::writeExclude(std::string filename, varset exclude) {
    
    FILE *out = fopen(filename.c_str(), "w");
    
    // we do not really need the header information
//    std::string header = "META pss_version = 0.1\nMETA input_file=" + getMetaInformation("input_file") + "\nMETA num_records=" + getMetaInformation("num_records") + "\n";
//    header += "META parent_limit=" + getMetaInformation("parent_limit") + "\nMETA score_type=" + getMetaInformation("score_type") + "\nMETA ess=" + getMetaInformation("ess") + "\n\n";
//    fprintf(out, header.c_str());
    
    int count = 0;

    for (int variable = 0; variable < variableCount; variable++) {
        
        

        datastructures::Variable *var = network->get(variable);
        fprintf(out, "VAR %s\n", var->getName().c_str());
//        fprintf(out, "META arity=%d\n", var->getCardinality());
//
//        fprintf(out, "META values=");
//        for (int i = 0; i < var->getCardinality(); i++) {
//            fprintf(out, "%s ", var->getValue(i).c_str());
//        }
//        fprintf(out, "\n");
//        
        // skip anything that we are excluding
        // we still have to write the variable information because others can use it as a parent
        if (VARSET_GET(exclude, variable)) {
            fprintf(out, "0\n\n");
            continue;
        }


        for (auto score = cache[variable]->begin(); score != cache[variable]->end(); score++) {
            varset parentSet = (*score).first;
            float s = (*score).second;
            
            if (s > 0) {
                s = -s;
            }
            
//            // if the parent set contains something than has been excluded
//            // then skip this parent set
//            if (VARSET_AND(parentSet, exclude) != 0) {
//                continue;
//            }

            fprintf(out, "%f ", s);

            for (int p = 0; p < network->size(); p++) {
                if (VARSET_GET(parentSet, p)) {
                    fprintf(out, "%s ", network->get(p)->getName().c_str());
                }
            }

            fprintf(out, "\n");
            count++;
        }

        fprintf(out, "\n");
        
    }
    
    fclose(out);
    return count;
}

scoring::ScoreCache::~ScoreCache() {
    variableCardinalities.clear();

    for (auto c : cache) {
        delete c;
    }

    cache.clear();
    
    if (network != NULL) {
        delete network;
    }
}
