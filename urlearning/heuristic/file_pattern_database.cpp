#include "file_pattern_database.h"
#include "static_pattern_database.h"
#include "dynamic_pattern_database.h"

#include <fstream>
#include <iostream>

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>

heuristics::FilePatternDatabase::FilePatternDatabase(const int variableCount, std::string filename, varset &ancestors, varset &scc) {
    this->variableCount = variableCount;
    this->filename = filename;
    this->ancestors = ancestors;
    this->scc = scc;
}

heuristics::FilePatternDatabase::~FilePatternDatabase() {
    for (auto pd = pds.begin(); pd != pds.end(); pd++) {
        delete (*pd);
    }
}

heuristics::DynamicPatternDatabase *heuristics::FilePatternDatabase::getDynamicPatternDatabase(std::vector<std::string> &tokens, std::vector<bestscorecalculators::BestScoreCalculator*>& spgs) {
    int maxSize = atoi(tokens[1].c_str());
    heuristics::DynamicPatternDatabase *pd = new heuristics::DynamicPatternDatabase(spgs.size(), maxSize, false);
    pd->initialize(spgs, ancestors, scc);
    return pd;
}

heuristics::StaticPatternDatabase *heuristics::FilePatternDatabase::getStaticPatternDatabase(std::vector<std::string> &tokens, std::vector<bestscorecalculators::BestScoreCalculator*>& spgs) {
    std::vector<varset> variableSets;
    
    // each of those tokens is a variable set
    for (int i = 1; i < tokens.size(); i++) {
        std::string vs = tokens[i];
        VARSET_NEW(variableSet, variableCount);

        // now get the variables for this variable set, comma-delimited
        std::vector<std::string> vsTokens;
        boost::split(vsTokens, (vs), boost::is_any_of(","), boost::token_compress_on);

        for (auto v = vsTokens.begin(); v != vsTokens.end(); v++) {
            int var = atoi((*v).c_str());
            VARSET_SET(variableSet, var);
        }

        variableSets.push_back(variableSet);
    }

    // create the static pd from these variable sets
    heuristics::StaticPatternDatabase *pd = new heuristics::StaticPatternDatabase();
    pd->initialize(spgs, variableSets, ancestors, scc);
    
    return pd;
}

void heuristics::FilePatternDatabase::initialize(std::vector<bestscorecalculators::BestScoreCalculator*>& spgs) {
    std::ifstream in(filename.c_str());
    
    // make sure we found the file
    if (!in.is_open()) {
        throw std::runtime_error("Could not open the pattern database file: '" + filename + "'");
    }
    
    std::vector<std::string> tokens;
    std::string line;

    // read in the pattern databases
    while (!in.eof()) {
        std::getline(in, line);

        // skip empty lines and comments
        if (line.size() == 0 || line.compare(0, 1, "#") == 0) {
            continue;
        }
        
        // first, split on semicolons
        boost::trim(line);
        boost::split(tokens, line, boost::is_any_of(";"), boost::token_compress_on);
        
        heuristics::Heuristic *pd = NULL;
        if (tokens[0] == "s") {
            pd = getStaticPatternDatabase(tokens, spgs);
        } else if (tokens[0] == "d") {
            pd = getDynamicPatternDatabase(tokens, spgs);
        } else {
            throw std::runtime_error("Invalid pattern database specification: '" + line + "'");
        }
        
        pds.push_back(pd);
        uses.push_back(0);
    }
}


float heuristics::FilePatternDatabase::h(const varset& variables, bool &complete) {
    float max = 0;
    int maxIndex = -1;
    
    int curIndex = 0;
    for (auto pd = pds.begin(); pd != pds.end(); pd++) {
        float s = (*pd)->h(variables, complete);
        if (s > max) {
            max = s;
            maxIndex = curIndex;
        }
        
        if (complete) {
            break;
        }
        
        curIndex++;
    }
    
    if (maxIndex > -1) {
        uses[maxIndex]++;
    }
    return max;
}

void heuristics::FilePatternDatabase::print() {
    printf("*** Static pattern database ***\n");
    //    spd->print();
}

void heuristics::FilePatternDatabase::printStatistics() {
    printf("uses,file_pattern_database\n");
    for (int i = 0; i < uses.size(); i++) {
        printf("statistics,%d,%d\n", i, uses[i]);
    }
}

int heuristics::FilePatternDatabase::size() {
    int size = 0;

    for (auto pd = pds.begin(); pd != pds.end(); pd++) {
        size += (*pd)->size();
    }

    return size;
}