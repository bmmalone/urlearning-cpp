#include "bayesian_network.h"
#include "variable.h"
#include <stdexcept>

datastructures::BayesianNetwork::BayesianNetwork() {
    name = "";
}

datastructures::BayesianNetwork::BayesianNetwork(int size) {
    for (int i = 0; i < size; i++) {
        datastructures::Variable *v = new datastructures::Variable(this, i);
        std::string name = "Variable_" + i;
        v->setName(name);
        nameToIndex[v->getName()] = variables.size();
        variables.push_back(v);
    }
}

datastructures::BayesianNetwork::~BayesianNetwork() {
    for (auto var = variables.begin(); var != variables.end(); var++) {
        delete *var;
    }
}

void datastructures::BayesianNetwork::initialize(datastructures::RecordFile &recordFile) {
    printf("recordFile.getRecords()[0].size(): %d\n", recordFile.getRecords()[0].size());
    for (int i = 0; i < recordFile.getRecords()[0].size(); i++) {
        datastructures::Variable *v = new datastructures::Variable(this, i);
        variables.push_back(v);

        if (recordFile.getHasHeader()) {
            printf("Assigning variable name: %s\n", recordFile.getHeader().get(i).c_str());
            variables[i]->setName(recordFile.getHeader().get(i));
        } else {
            std::string name = std::string("Variable_") + TO_STRING(i);
            variables[i]->setName(name);
        }
        
        nameToIndex[v->getName()] = i;
    }
    addValues(recordFile);
}

datastructures::Variable *datastructures::BayesianNetwork::get(int variable) {
    return variables[variable];
}

datastructures::Variable *datastructures::BayesianNetwork::get(std::string variable) {
#ifdef DEBUG
    printf("nameToIndex.size(): %d\n", nameToIndex.size());
#endif
    return variables[nameToIndex[variable]];
}

int datastructures::BayesianNetwork::getVariableIndex(std::string variable) {
    return nameToIndex[variable];
}

datastructures::Variable *datastructures::BayesianNetwork::addVariable(std::string name) {
    if (nameToIndex.find(name) != nameToIndex.end()) {
        throw std::runtime_error("Duplicate variable name: '" + name + "'.");
    }
    
    nameToIndex[name] = variables.size();
    Variable *v = new Variable(this, variables.size());
    v->setName(name);
    variables.push_back(v);
    
    return v;
}

void datastructures::BayesianNetwork::setParents(std::vector<varset>& parents) {
    int i = 0;
    for (auto it = variables.begin(); it != variables.end(); it++) {
        (*it)->setParents(parents[i++]);
    }

    setDefaultParentOrder();
}

/**
 * Set the parent order for each variable to a default ordering of
 * increasing parent index. For example, if $X_0$ has parents $X_1$, $X_3$,
 * $X_7$, then the ordering will be set to $X_1$,$X_3$,$X_7$. This is useful
 * if the network was created by, e.g., structure learning or random
 * generation. If network was read in from a file, though, the order is
 * already set by the order parents appear in the file and should not be
 * changed.
 */
void datastructures::BayesianNetwork::setDefaultParentOrder() {
    for (int i = 0; i < size(); i++) {
        get(i)->setDefaultParentOrder();
    }
}

/**
 * Based on the current structure, generate uniform parameters.
 */
void datastructures::BayesianNetwork::setUniformProbabilities() {

    updateParameterSizes();

    // for each variable
    for (int i = 0; i < size(); i++) {
        get(i)->setUniformProbabilities();
    }
}

/**
 * Update the conditional probability distributions of all variables in the
 * network based on their current parent sets. This just calls
 * {@link Variable#updateParameterSize()} for each variable.
 */
void datastructures::BayesianNetwork::updateParameterSizes() {
    for (int i = 0; i < size(); i++) {
        get(i)->updateParameterSize();
    }
}

/**
 * Ensure that each variable has at least two values assigned to it.  Generally,
 * this should be used when reading scores from a file which does not give the
 * values of the variables.
 */
void datastructures::BayesianNetwork::fixCardinalities() {
    for (auto it = variables.begin(); it != variables.end(); it++) {
        (*it)->fixCardinality();
    }
}

int datastructures::BayesianNetwork::getCardinality(int variable) {
    return variables[variable]->getCardinality();
}

int datastructures::BayesianNetwork::getMaxCardinality() {
    int max = 0;
    for(int i = 0; i < size(); i++) {
        int c = getCardinality(i);
        if (c > max) {
            max = c;
        }
    }
    return max;
}

std::vector< std::vector< bitset > > datastructures::BayesianNetwork::getConsistentRecords(datastructures::RecordFile &recordFile) {

    std::vector< std::vector< bitset > > consistentRecords;

    for (int x = 0; x < size(); x++) {
        consistentRecords.push_back(std::vector< bitset > ());
        int count = getCardinality(x);

        for (int value = 0; value < count; value++) {
            consistentRecords[x].push_back(BITSET_NEW(recordFile.size()));
        }
    }

    for (int index = 0; index < recordFile.size(); index++) {
        Record record = recordFile.getRecords()[index];
        for (int variable = 0; variable < size(); variable++) {
            std::string v = record.get(variable);
            int value = get(variable)->getValueIndex(v);
            BITSET_SET(consistentRecords[variable][value], index);
        }
    }

    return consistentRecords;
}

void datastructures::BayesianNetwork::addValues(RecordFile &recordFile) {
    for (auto it = variables.begin(); it < variables.end(); it++) {
        (*it)->addValues(recordFile);
    }
}

int datastructures::BayesianNetwork::size() {
    return variables.size();
}