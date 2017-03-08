/* 
 * File:   constraints.h
 * Author: bmmalone
 *
 * Created on May 17, 2013, 6:58 PM
 */

#ifndef CONSTRAINTS_H
#define	CONSTRAINTS_H

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

typedef boost::char_separator<char> separator_type;

#include "urlearning/base/typedefs.h"
#include "urlearning/base/bayesian_network.h"
#include "urlearning/base/variable.h"

namespace scoring {

    /**
     * This class stores the constraints as a 3d array.
     *
     * The first dimension is the variable. The second dimension is always size 2.
     * <ul> <li>the first entry has the required variables for each constraint</li>
     * <li>the second entry has the forbidden variables for each constraint</li>
     * </ul>
     *
     * The third dimension has one entry for each constraint.
     *
     * @author malone
     */
    class Constraints {
    public:

        /**
         * Create a set of constraints over {@code variableCount} variables.
         *
         * @param variableCount the number of variables
         */

        Constraints(int variableCount) {
            this->variableCount = variableCount;

            for (int i = 0; i < variableCount; i++) {
                std::vector<varset> forbidden;
                std::vector<varset> required;

                std::vector<std::vector<varset> > varConstraints;
                varConstraints.push_back(forbidden);
                varConstraints.push_back(required);

                constraints.push_back(varConstraints);
            }

            VARSET_NEW(empty, variableCount);
        }

        /**
         * Add a constraint for {@code variable}.
         *
         * @param variable the variable
         * @param requiredParents variable set which must be parents of the variable
         * @param forbiddenParents variable set which cannot be parents of the
         * variable
         */
        void addConstraint(int variable, varset requiredParents, varset forbiddenParents) {
            constraints[variable][0].push_back(requiredParents);
            constraints[variable][1].push_back(forbiddenParents);

#ifdef DEBUG
            printf("Added constraint. Variable: %d. RequriedParents: %s. Forbidden parents: %s.\n", variable, varsetToString(requiredParents).c_str(), varsetToString(forbiddenParents).c_str());
#endif
        }

        /**
         * Find the number of constraints for {@code variable}.
         * 
         * @param variable the variable
         * @return the number of constraints
         */
        int size(int variable) {
            return constraints[variable][0].size();
        }

        /**
         * Check if {@code parents} satisfies at least one of the constraints for
         * {@code variable}.
         *
         * @param variable the variable
         * @param parents the parent set
         * @return {@code true} if {@code parents} satisfies at least one constraint
         */
        bool satisfiesConstraints(int variable, varset &parents) {
            // if there are no constraints, then everything satisfies
            if (size(variable) == 0) {
                return true;
            }

            for (int constraint = 0; constraint < constraints[variable][0].size(); constraint++) {
                varset r = constraints[variable][0][constraint];
                varset f = constraints[variable][1][constraint];

                // does it have all of the required and none of the forbidden?
                if (VARSET_IS_SUBSET_OF(r, parents) && VARSET_AND(parents, f) == empty) {
                    return true;
                }
            }
            return false;
        }


    private:

        /**
         * The number of variables in the data set.
         */
        int variableCount;

        /**
         * A helper constant.
         **/
        varset empty;

        /**
         * The actual constraints. See the class description for the meaning of each
         * array dimension.
         */
        std::vector<std::vector<std::vector<varset> > > constraints;
    };

    inline Constraints *parseConstraints(std::string filename, datastructures::BayesianNetwork &network) {
        Constraints *constraints = new Constraints(network.size());

        int variableCount;

        std::ifstream file(filename);
        std::string line;

        variableCount = network.size();

        while (std::getline(file, line)) {
            // strip comments and excess whitespace
            int index = line.find("#");
            std::string stripped = line.substr(0, index);
            boost::trim(stripped);

            if (stripped.length() == 0) {
                continue;
            }

            VARSET_NEW(required, variableCount);
            VARSET_NEW(forbidden, variableCount);
            bool f = false;

            boost::tokenizer<separator_type> tokenizer(stripped, separator_type(", "));

            auto it = tokenizer.begin();

            std::string variableName = *it;
            datastructures::Variable *variable = network.get(variableName);

#ifdef DEBUG
            printf("Constraint variable name: '%s', index: '%d'\n", variableName.c_str(), variable->getIndex());
#endif

            // move to the next token
            it++;

            while (it != tokenizer.end()) {

                if (*it == ")") {
                    constraints->addConstraint(variable->getIndex(), required, forbidden);
                    VARSET_CLEAR_ALL(required);
                    VARSET_CLEAR_ALL(forbidden);
                } else if ( (*it)[0] == '!') {
                    std::string variableName = (*it).substr(1);
                    datastructures::Variable *parent = network.get(variableName);
                    if (parent == NULL) {
                        throw std::runtime_error("Error parsing constraint file.  Expecting a variable name, but found: '" + *it + "'.");
                    }
                    VARSET_SET(forbidden, parent->getIndex());
                    
                } else if (*it == "(") {
                    // skip this token
                    // just do nothing
                }
                else {
                    datastructures::Variable *parent = network.get(*it);
                    if (parent == NULL) {
                        throw std::runtime_error("Error parsing constraint file.  Expecting a variable name, but found: '" + *it + "'.");
                    }

                    VARSET_SET(required, parent->getIndex());
                }

#ifdef DEBUG
                printf("token: %s\n", (*it).c_str());
#endif
                it++;
            }
        }

        file.close();

        return constraints;
    }

}



#endif	/* CONSTRAINTS_H */

