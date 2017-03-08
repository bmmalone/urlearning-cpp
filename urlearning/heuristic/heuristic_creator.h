/* 
 * File:   heuristic_creator.h
 * Author: bmmalone
 *
 * Created on August 30, 2013, 10:18 AM
 */

#ifndef HEURISTIC_CREATOR_H
#define	HEURISTIC_CREATOR_H

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "heuristic.h"
#include "dynamic_pattern_database.h"
#include "static_pattern_database.h"
#include "combined_pattern_database.h"
#include "file_pattern_database.h"


namespace heuristics {
    
    static std::string heuristicTypeString = "The type of heuristic to use. [\"static\", \"static_random\", \"dynamic\", \"dynamic_optimal\", \"combined\", \"file\"]";
    static std::string heuristicArgumentString = "The argument for creating the heuristic, such as number of pattern databases to use.";

    inline Heuristic *createWithAncestors(std::string heuristicType, std::string argument, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs, varset &ancestors, varset &scc) {
        Heuristic *heuristic = NULL;

        boost::algorithm::to_lower(heuristicType);


        if (heuristicType == "static") {
            int pdCount = atoi(argument.c_str());
            heuristic = new heuristics::StaticPatternDatabase(spgs.size(), pdCount, false, ancestors, scc);
        } else if (heuristicType == "static_random") {
            int pdCount = atoi(argument.c_str());
            heuristic = new heuristics::StaticPatternDatabase(spgs.size(), pdCount, true, ancestors, scc);
        } else if (heuristicType == "dynamic_optimal") {
            int maxSize = atoi(argument.c_str());
            heuristic = new heuristics::DynamicPatternDatabase(spgs.size(), maxSize, true);
        } else if (heuristicType == "dynamic") {
            int maxSize = atoi(argument.c_str());
            heuristic = new heuristics::DynamicPatternDatabase(spgs.size(), maxSize, false);
        } else if (heuristicType == "combined") {
            std::vector<std::string> args;
            boost::algorithm::split(args, argument, boost::is_any_of(","), boost::token_compress_on);
            
            int pdCount = atoi(args[0].c_str());
            int maxSize = atoi(args[1].c_str());
            int staticCount = atoi(args[2].c_str());
            
            heuristic = new heuristics::CombinedPatternDatabase(spgs.size(), pdCount, maxSize, staticCount);
        } else if (heuristicType == "file") {
            heuristic = new heuristics::FilePatternDatabase(spgs.size(), argument, ancestors, scc);            
        } else {
            throw std::runtime_error("Invalid heuristic type: '" + heuristicType + "'.  Valid options are 'static', 'static_random', 'dynamic', 'dynamic_optimal', 'combined' and 'file'.");
        }

        heuristic->initialize(spgs);
        return heuristic;
    }
    
    inline Heuristic *create(std::string heuristicType, std::string argument, std::vector<bestscorecalculators::BestScoreCalculator*> &spgs) {
        VARSET_NEW(ancestors, spgs.size());
        VARSET_NEW(scc, spgs.size());
        VARSET_SET_ALL(scc, spgs.size());
        return heuristics::createWithAncestors(heuristicType, argument, spgs, ancestors, scc);
    }
}


#endif	/* HEURISTIC_CREATOR_H */

