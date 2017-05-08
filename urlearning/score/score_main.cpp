#include <cstdlib>
#include <ctime>
#include <math.h>
#include <stdexcept>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>  

#include "urlearning/base/record_file.h"
#include "urlearning/base/bayesian_network.h"
#include "urlearning/base/variable.h"

#include "urlearning/ad_tree/ad_node.h"
#include "urlearning/ad_tree/ad_tree.h"

#include "urlearning/scoring_function/scoring_function.h"
#include "urlearning/scoring_function/score_calculator.h"

#include "urlearning/scoring_function/constraints.h"

#include "urlearning/scoring_function/bdeu_scoring_function.h"
#include "urlearning/scoring_function/bic_scoring_function.h"
#include "urlearning/scoring_function/fnml_scoring_function.h"

namespace po = boost::program_options;

/**
 * The file containing the data.
 */
std::string inputFile;

/**
 * The delimiter in each line.
 */
char delimiter = ',';

/**
 * The file to write the scores.
 */
std::string outputFile;

/**
 * File specifying constraints on the scores.
 */
std::string constraintsFile;

/**
 * The minimum number of records in the AD-tree.
 */
int rMin = 5;

/**
 * The scoring function to use.
 */
std::string sf = "BIC";

/**
 * A reference to the scoring function object.
 */
scoring::ScoringFunction *scoringFunction;

/**
 * The ess to use for BDeu.
 */
float equivalentSampleSize = 1.0f;

/**
 * A hard limit on the size of parent sets.
 */
int maxParents = -1;

/**
 * The number of threads to use.
 */
int threadCount = 1;

/**
 * The maximum amount of time to use for each variable.
 */
int runningTime = -1;

/**
 * Whether the data file has variable names in the first row.
 */
bool hasHeader = false;

/**
 * Whether to prune the scores before printing
 */
bool prune = true;

/**
 * Whether to use deCampos-style pruning.
 */
bool enableDeCamposPruning = false;

/**
 * The network information.
 */
datastructures::BayesianNetwork network;

/**
 * Constraints on the allowed scores.
 */
scoring::Constraints *constraints;

inline std::string getTime() {
    time_t now = time(0);
    tm *gmtm = gmtime(&now);
    std::string dt(asctime(gmtm));
    boost::trim(dt);
    return dt;
}

void scoringThread(int thread) {
    scoring::ScoreCalculator scoreCalculator(scoringFunction, maxParents, network.size(), runningTime, constraints);

    for (int variable = 0; variable < network.size(); variable++) {
        if (variable % threadCount != thread) {
            continue;
        }

        printf("Thread: %d, Variable: %d, Time: %s\n", thread, variable, getTime().c_str());

        FloatMap sc;
        init_map(sc);
        scoreCalculator.calculateScores(variable, sc);

        //#ifdef DEBUG
        int size = sc.size();
        printf("Thread: %d, Variable: %d, Size before pruning: %d, Time: %s\n", thread, variable, size, getTime().c_str());
        //#endif

        if (prune) {
            scoreCalculator.prune(sc);
            int prunedSize = sc.size();
            printf("Thread: %d, Variable: %d, Size after pruning: %d, Time: %s\n", thread, variable, prunedSize, getTime().c_str());
        }

        std::string varFilename = outputFile + "." + TO_STRING(variable);
        FILE *varOut = fopen(varFilename.c_str(), "w");

        datastructures::Variable *var = network.get(variable);
        fprintf(varOut, "VAR %s\n", var->getName().c_str());
        fprintf(varOut, "META arity=%d\n", var->getCardinality());

        fprintf(varOut, "META values=");
        for (int i = 0; i < var->getCardinality(); i++) {
            fprintf(varOut, "%s ", var->getValue(i).c_str());
        }
        fprintf(varOut, "\n");


        for (auto score = sc.begin(); score != sc.end(); score++) {
            varset parentSet = (*score).first;
            float s = (*score).second;

            fprintf(varOut, "%f ", s);

            for (int p = 0; p < network.size(); p++) {
                if (VARSET_GET(parentSet, p)) {
                    fprintf(varOut, "%s ", network.get(p)->getName().c_str());
                }
            }

            fprintf(varOut, "\n");
        }

        fprintf(varOut, "\n");
        fclose(varOut);

        sc.clear();
    }
}

int main(int argc, char** argv) {
    boost::timer::auto_cpu_timer t;

    std::string description = std::string("Compute the scores for a csv file.  Example usage: ") + argv[0] + " iris.csv iris.pss";
    po::options_description desc(description);

    desc.add_options()
            ("input", po::value<std::string > (&inputFile)->required(), "The input file. First positional argument.")
            ("output", po::value<std::string > (&outputFile)->required(), "The output file. Second positional argument.")
            ("delimiter,d", po::value<char> (&delimiter)->required()->default_value(','), "The delimiter of the input file.")
            ("constraints,c", po::value<std::string > (&constraintsFile), "The file specifying constraints on the scores.")
            ("r-min,m", po::value<int> (&rMin)->default_value(5), "The minimum number of records in the AD-tree nodes.")
            ("function,f", po::value<std::string > (&sf)->default_value("BIC"), "The scoring function to use.")
            ("ess,e", po::value<float> (&equivalentSampleSize)->default_value(1.0f), "The equivalent sample size, if BDeu is used.")
            ("max-parents,p", po::value<int> (&maxParents)->default_value(0), "The maximum number of parents for any variable. A value less than 1 means no limit.")
            ("threads,t", po::value<int> (&threadCount)->default_value(1), "The number of separate threads to use for score calculations.")
            ("time,r", po::value<int> (&runningTime)->default_value(-1), "The maximum amount of time to use for each variable. A value less than 1 means no limit.")
            ("has-header,s", "Add this flag if the first line of the input file gives the variable names.")
            ("do-not-prune,o", "Add this flag if the scores should NOT be pruned at the end of the search.")
            ("enable-de-campos-pruning", "Add this flag to ENABLE DeCampos & Ji (JMLR 2011) pruning for BDeu. This feature is experimental and appears to contain some bugs for sufficiently large parent limits for BDeu. This flag has no effect for BIC or fNML.")
            ("help,h", "Show this help message.")
            ;

    po::positional_options_description positionalOptions;
    positionalOptions.add("input", 1);
    positionalOptions.add("output", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc)
            .positional(positionalOptions).run(),
            vm);

    if (vm.count("help") || argc == 1) {
        std::cout << desc;
        return 0;
    }

    po::notify(vm);

    hasHeader = vm.count("has-header");
    prune = (vm.count("do-not-prune") == 0);
    enableDeCamposPruning = vm.count("enable-de-campos-pruning");

    if (threadCount < 1) {
        threadCount = 1;
    }

    printf("URLearning, Score Calculator\n");
    printf("Input file: '%s'\n", inputFile.c_str());
    printf("Output file: '%s'\n", outputFile.c_str());
    printf("Delimiter: '%c'\n", delimiter);
    printf("Constraints file: '%s'\n", constraintsFile.c_str());
    printf("r_min: '%d'\n", rMin);
    printf("Scoring function: '%s'\n", sf.c_str());
    printf("ESS: '%f'\n", equivalentSampleSize);
    printf("Maximum parents: '%d'\n", maxParents);
    printf("Threads: '%d'\n", threadCount);
    printf("Running time (per variable): '%d'\n", runningTime);
    printf("Has header: '%s'\n", (hasHeader ? "true" : "false"));
    printf("Enable end-of-scoring pruning: '%s'\n", (prune ? "true" : "False"));
    printf("Enable deCampos-style pruning (experimental for BDeu): '%s'\n", (enableDeCamposPruning ? "true" : "False"));


    printf("Parsing input file.\n");
    datastructures::RecordFile recordFile(inputFile, delimiter, hasHeader);
    recordFile.read();

    printf("Initializing data specifications.\n");
    network.initialize(recordFile);

    printf("Creating AD-tree.\n");
    scoring::ADTree *adTree = new scoring::ADTree(rMin);
    adTree->initialize(network, recordFile);
    adTree->createTree();

    boost::algorithm::to_lower(sf);

    if (maxParents > network.size() || maxParents < 1) {
        maxParents = network.size() - 1;
    }

    if (sf.compare("bic") == 0) {
        int maxParentCount = log(2 * recordFile.size() / log(recordFile.size()));
        if (maxParentCount < maxParents) {
            maxParents = maxParentCount;
        }
    } else if (sf.compare("fnml") == 0) {
    } else if (sf.compare("bdeu") == 0) {
    } else {
        throw std::runtime_error("Invalid scoring function.  Options are: 'BIC', 'fNML' or 'BDeu'.");
    }

    scoring::Constraints *constraints = NULL;
    if (constraintsFile.length() > 0) {
        constraints = scoring::parseConstraints(constraintsFile, network);
    }

    scoringFunction = NULL;

    std::vector<float> ilogi = scoring::LogLikelihoodCalculator::getLogCache(recordFile.size());
    scoring::LogLikelihoodCalculator *llc = new scoring::LogLikelihoodCalculator(adTree, network, ilogi);

    std::vector< std::vector< float >* >* regret = scoring::getRegretCache(recordFile.size(), network.getMaxCardinality());


    if (sf.compare("bic") == 0) {
        scoringFunction = new scoring::BICScoringFunction(network, recordFile, llc, constraints, enableDeCamposPruning);
    } else if (sf.compare("fnml") == 0) {
        scoringFunction = new scoring::fNMLScoringFunction(network, llc, constraints, regret, enableDeCamposPruning);
    } else if (sf.compare("bdeu") == 0) {
        scoringFunction = new scoring::BDeuScoringFunction(equivalentSampleSize, network, adTree, constraints, enableDeCamposPruning);
    }

    std::vector<boost::thread*> threads;
    for (int thread = 0; thread < threadCount; thread++) {
        boost::thread *workerThread = new boost::thread(scoringThread, thread);
        threads.push_back(workerThread);
    }

    for (auto it = threads.begin(); it != threads.end(); it++) {
        (*it)->join();
    }


    // concatenate all of the files together
    std::ofstream out(outputFile, std::ios_base::out | std::ios_base::binary);

    // first, the header information
    std::string header = "META pss_version = 0.1\nMETA input_file=" + inputFile + "\nMETA num_records=" + TO_STRING(recordFile.size()) + "\n";
    header += "META parent_limit=" + TO_STRING(maxParents) + "\nMETA score_type=" + sf + "\nMETA ess=" + TO_STRING(equivalentSampleSize) + "\n\n";
    out.write(header.c_str(), header.size());

    for (int variable = 0; variable < network.size(); variable++) {
        std::string varFilename = outputFile + "." + TO_STRING(variable);
        std::ofstream varFile(varFilename, std::ios_base::in | std::ios_base::binary);

        out << varFile.rdbuf();
        varFile.close();

        // and remove the variable file
        remove(varFilename.c_str());
    }

    out.close();
}
