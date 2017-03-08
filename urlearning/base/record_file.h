/* 
 * File:   record_file.h
 * Author: malone
 *
 * Created on November 22, 2012, 10:05 PM
 */

#ifndef RECORD_FILE_H
#define	RECORD_FILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "record.h"

namespace datastructures {

    class RecordFile {
    public:

        RecordFile() {
            filename = "";
            delimiter = ',';
            hasHeader = false;
        }

        RecordFile(std::string filename, char delimiter, bool hasHeader) {
            this->filename = filename;
            this->delimiter = delimiter;
            this->hasHeader = hasHeader;
        }
        
        ~RecordFile() {
            records.clear();
        }

        void read() {
            std::ifstream file(filename);

            std::string line;

            if (hasHeader) {
                std::getline(file, line);
                boost::algorithm::trim(line);
                header = Record(line, delimiter);
            }

            while (std::getline(file, line)) {
                boost::algorithm::trim(line);
                records.push_back(Record(line, delimiter));
            }
        }

        std::vector<Record> &getRecords() {
            return records;
        }

        bool getHasHeader() {
            return hasHeader;
        }

        Record &getHeader() {
            return header;
        }

        int size() {
            return records.size();
        }

    private:
        std::string filename;
        char delimiter;
        bool hasHeader;
        Record header;
        std::vector<Record> records;
    };

}


#endif	/* RECORD_FILE_H */

