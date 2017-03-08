/* 
 * File:   HuginStructureWriter.h
 * Author: bmmalone
 *
 * Created on May 10, 2013, 8:35 PM
 */

#ifndef HUGINSTRUCTUREWRITER_H
#define	HUGINSTRUCTUREWRITER_H

#include <stdio.h>

#include "urlearning/base/typedefs.h"

namespace datastructures {
    class BayesianNetwork;
    class Variable;
    class Record;
}

namespace fileio {

    class HuginStructureWriter {
    public:

        HuginStructureWriter() {
        }

        void write(datastructures::BayesianNetwork *network, std::string filename);

    private:

        void writeVariableDescription(int index);
        void writeRootVariable(int index);
        void writeNonrootVariable(int index);
        void printParents(varset &parents);
        void printInstantiation(datastructures::Record *instantiation, varset &parents);
        void printProbabilities(std::vector<std::vector<double> > &values, int pIndex, datastructures::Variable *v);

        datastructures::BayesianNetwork *network;
        FILE *file;
    };
}
#endif	/* HUGINSTRUCTUREWRITER_H */

