/* 
 * File:   HuginStructureWriter.cpp
 * Author: bmmalone
 * 
 * Created on May 10, 2013, 8:35 PM
 */

#include "hugin_structure_writer.h"
#include "urlearning/base/variable.h"

void fileio::HuginStructureWriter::write(datastructures::BayesianNetwork *network, std::string filename) {
    this->network = network;
    file = fopen(filename.c_str(), "w");

    fputs("net {}\n", file);
    for (int i = 0; i < network->size(); i++) {
        writeVariableDescription(i);
    }

    // write the parents
    for (int i = 0; i < network->size(); i++) {
        if (cardinality(network->get(i)->getParents()) > 0) {
            writeNonrootVariable(i);
        } else {
            writeRootVariable(i);
        }
    }

    fclose(file);
}

void fileio::HuginStructureWriter::writeVariableDescription(int index) {
    datastructures::Variable *v = network->get(index);

    int cardinality = v->getCardinality();
    std::string variableName = v->getName();

    fprintf(file, "node %s { \n states = ( ", variableName.c_str());

    // build the list of values
    for (int j = 0; j < cardinality; j++) {
        fprintf(file, "\"%s\" ", v->getValue(j).c_str());
    }

    fprintf(file, ");\n");
    fprintf(file, "}\n");
}

void fileio::HuginStructureWriter::writeRootVariable(int index) {
    datastructures::Variable *v = network->get(index);

    fprintf(file, "potential ( %s ) {\n", v->getName().c_str());
    fprintf(file, " data = (");

    printProbabilities(v->getParameters(), 0, v);

    fprintf(file, ");\n");
    fprintf(file, "}\n");
}

void fileio::HuginStructureWriter::writeNonrootVariable(int index) {
    datastructures::Variable *v = network->get(index);
    varset parents = v->getParents();

    fprintf(file, "potential ( %s | ", v->getName().c_str());

    printParents(parents);

    fprintf(file, " ) {\n");
    fprintf(file, " data = ");

    // now, print the probabilities
    datastructures::Record *ins = v->getFirstInstantiation();

    std::vector<std::vector<double> > parameters = v->getParameters();


    for (int i = 0; i < cardinality(parents) + 1; i++) {
        fprintf(file, "(");
    }

    for (int pIndex = 0; pIndex < v->getParametersSize(); pIndex++) {
        int pI = v->getParentIndex(ins);


        printProbabilities(parameters, pI, v);

        int count = v->getNextParentInstantiation(ins);
        for (int i = 0; i < count + 1; i++) {
            fprintf(file, ")");
        }
        for (int i = 0; i < count + 1; i++) {
            fprintf(file, "(");
        }
    }
    for (int i = 0; i < cardinality(parents) + 1; i++) {
        fprintf(file, ")");
    }

    fprintf(file, ";\n");
    fprintf(file, "}\n");
}

void fileio::HuginStructureWriter::printParents(varset &parents) {
    for (int p = 0; p < network->size(); p++) {
        if (VARSET_GET(parents, p)) {

            fprintf(file, network->get(p)->getName().c_str());
            fprintf(file, " ");
        }
    }
}

void fileio::HuginStructureWriter::printInstantiation(datastructures::Record *instantiation, varset & parents) {
    // build up the list of parents
    std::string line;
    for (int p = 0; p < network->size(); p++) {
        if (VARSET_GET(parents, p)) {
            line.append(instantiation->get(p));
            line.append(",");
        }
    }
    if (line.length() > 0) {
        line = line.substr(0, line.length() - 1);
    }

    fprintf(file, line.c_str());
}

void fileio::HuginStructureWriter::printProbabilities(std::vector<std::vector<double> > &values, int pIndex, datastructures::Variable * v) {
    std::string s;
    for (int i = 0; i < v->getCardinality(); i++) {
        char val[1024];
        snprintf(val, 1024, "%.2f", values[pIndex][i]);
        s.append(val);
        s.append(" ");
    }
    if (s.length() > 0) {
        s = s.substr(0, s.length() - 1);
    }

    fprintf(file, s.c_str());
}
