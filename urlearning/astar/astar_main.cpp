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

#include "urlearning/astar/astar_searcher.h"

namespace po = boost::program_options;


/**
 * A timer to keep track of how long the algorithm has been running.
 */
boost::asio::io_service io;

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

/**
 * The variables for this search.
 */
std::string sccArgument;      //csv list

/**
 * The maximum running time for the algorithm.
 */
int runningTime;

astar::AStarSearcher *astar_searcher;

void print_command_line_options() {
	printf("URLearning, A*\n");
	printf("Dataset: '%s'\n", scoreFile.c_str());
	printf("Net file: '%s'\n", netFile.c_str());
	printf("Best score calculator: '%s'\n", bestScoreCalculator.c_str());
	printf("Heuristic type: '%s'\n", heuristicType.c_str());
	printf("Heuristic argument: '%s'\n", heuristicArgument.c_str());
	printf("Ancestors: '%s'\n", ancestorsArgument.c_str());
	printf("SCC: '%s'\n", sccArgument.c_str());
}

/**
 * Handler when out of time.
 */
void timeout(const boost::system::error_code& /*e*/) {
    printf("Out of time\n");

    if (astar_searcher != NULL) {
    	astar_searcher->set_out_of_time(true);
    }
}

void astar_main() {
	print_command_line_options();

    boost::timer::auto_cpu_timer act;

    // create the searcher
	astar_searcher = new astar::AStarSearcher();

	act.start();
    printf("Reading score cache\n");
    scoring::ScoreCache cache;
    cache.read(scoreFile);
    astar_searcher->set_cache(cache);
    int variable_count = cache.getVariableCount();
    act.stop();
    act.report();

    printf("Parsing the ancestor and SCC_i variables\n");
    VARSET_NEW(ancestors,variable_count);
    VARSET_NEW(scc,variable_count);

    // if no scc was specified, assume we just want to learn everything
    if (sccArgument == "") {
        VARSET_SET_ALL(scc, variable_count);
    } else {
        setFromCsv(scc, sccArgument);
        setFromCsv(ancestors, ancestorsArgument);
    }

    VARSET_NEW(goal, variable_count);
    VARSET_SET_VALUE(goal, ancestors);
    goal = VARSET_OR(goal, scc);


#ifdef DEBUG
    printf("The start is '%s'\n", varsetToString(ancestors).c_str());
    printf("The goal is '%s'\n", varsetToString(all_variables).c_str());
#endif

    astar_searcher->set_start(ancestors);
    astar_searcher->set_goal(goal);
    astar_searcher->update_vars_to_add();


    act.start();
    printf("Creating BestScore calculators\n");
    bestscorecalculators::best_score_calculators best_score_calculators = bestscorecalculators::create(bestScoreCalculator, cache);
    astar_searcher->set_best_score_calculators(best_score_calculators);
    act.stop();
    act.report();

    act.start();
    printf("Creating the heuristic\n");
    heuristics::Heuristic *heuristic = heuristics::createWithAncestors(
    		heuristicType,
			heuristicArgument,
			best_score_calculators,
			ancestors,
			scc
	);

    astar_searcher->set_heuristic(heuristic);
#ifdef DEBUG
    heuristic->print();
#endif

    act.stop();
    act.report();

    printf("Initializing the open list\n");
    astar_searcher->init_open_list();

    act.start();
    printf("Performing search\n");
    astar_searcher->search();
    act.stop();
    act.report();

    astar_searcher->print_statistics();
	heuristic->printStatistics();

	// wrap up the search
    if (astar_searcher->found_solution()) {
        printf("Found solution: %f\n", astar_searcher->get_optimal_structure_score());

        if (netFile.length() > 0) {
        	datastructures::BayesianNetwork *network = astar_searcher->get_optimal_structure();
        	fileio::HuginStructureWriter writer;
            writer.write(network, netFile);
        }
    } else {
        printf("No solution found.\n");
        printf("Lower bound: %f\n", astar_searcher->get_current_lower_bound());
    }

    // the astar_searcher deconstructor will clean up for us

    // kill our timer
    io.stop();
}

int main(int argc, char** argv) {
    boost::timer::auto_cpu_timer act;
    srand(time(NULL));

    std::string description = std::string("Learn an optimal Bayesian network using A*.  Example usage: ") + argv[0] + " iris.pss";
    po::options_description desc(description);

    desc.add_options()
    		("pss",
    			po::value<std::string > (&scoreFile)->required(),
    			"The file containing the local scores in pss format. First "
				"positional argument.")
            ("best-score,b",
            	po::value<std::string > (&bestScoreCalculator)->default_value("list"),
				bestscorecalculators::bestScoreCalculatorString.c_str())
            ("heuristic,e",
            	po::value<std::string > (&heuristicType)->default_value("static"),
				heuristics::heuristicTypeString.c_str())
            ("argument,a",
            	po::value<std::string > (&heuristicArgument)->default_value("2"),
				heuristics::heuristicArgumentString.c_str())
            ("pc_{i-1},p",
            	po::value<std::string > (&ancestorsArgument)->default_value(""),
				"Variables which can only be used as ancestors. They will not "
				"be added in the search. CSV-list of variable indices. See "
				"UAI '14.")
            ("scc_i,s",
            	po::value<std::string > (&sccArgument)->default_value(""),
				"Variables which will be added in the search. CSV-list of "
				"variable indices. Leave blank to add all variables. See "
				"UAI '14.")
            ("running-time,r",
            	po::value<int> (&runningTime)->default_value(0),
				"The maximum running time for the algorithm.  0 means no "
				"running time.")
            ("net-file,n",
            	po::value<std::string > (&netFile)->default_value(""),
				"The file to which the learned network is written.  Leave "
				"blank to not create the file.")
            ("help,h", "Show this help message.")
            ;

    po::positional_options_description positionalOptions;
    positionalOptions.add("pss", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc)
            .positional(positionalOptions).run(),
            vm);

    if (vm.count("help") || argc == 1) {
        std::cout << desc;
        return 0;
    }

    po::notify(vm);

    boost::to_lower(bestScoreCalculator);

    boost::asio::deadline_timer t(io);
    if (runningTime > 0) {
        printf("Maximum running time: %d\n", runningTime);
        t.expires_from_now(boost::posix_time::seconds(runningTime));
        t.async_wait(timeout);
        boost::thread workerThread(astar_main);
        io.run();
        workerThread.join();
    } else {
        astar_main();
    }

    return 0;
}

