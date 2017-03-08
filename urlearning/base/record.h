/* 
 * File:   record.h
 * Author: malone
 *
 * Created on November 22, 2012, 9:27 PM
 */

#ifndef RECORD_H
#define	RECORD_H

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "typedefs.h"

namespace datastructures {

    class Record {
    public:

        Record() {
        }

        Record(int size) {
            for (int i = 0; i < size; i++) {
                record.push_back("");
            }
        }

        Record(std::string &s, char &delimiter) {
            // split on the delimiter and copy to record
            boost::trim(s);
            boost::split(record, s, boost::is_any_of(TO_STRING(delimiter)), boost::token_compress_on);
        }

        int size() {
            return record.size();
        }

        std::vector<std::string> & getRecord() {
            return record;
        }

        std::string &get(int index) {
            return record[index];
        }

        void set(int index, std::string value) {
            record[index] = value;
        }

    private:
        std::vector<std::string> record;

    };

}


#endif	/* RECORD_H */

