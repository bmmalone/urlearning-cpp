/* 
 * File:   Files.h
 * Author: malone
 *
 * Created on August 7, 2012, 5:44 PM
 */

#ifndef FILES_H
#define	FILES_H

#include <fstream>
#include <iostream>
#include <endian.h>
#include <boost/dynamic_bitset.hpp>

inline std::string readString(std::ifstream& in) {
    int length;
    in.read((char*) &length, sizeof (length));
    length = be32toh(length);
    
    char string[length+1];
    in.read(string, length);
    string[length] = 0;
    
    return std::string(string);
}

inline void writeString(std::ofstream &out, std::string &s) {
    int size = s.size();
    
    out.write(reinterpret_cast <const char*>(size), sizeof(size));
    
    for(int i = 0; i < size; i++) {
        out.write(reinterpret_cast <const char*>(s[i]), sizeof(s[i]));
    }
}

inline int readInt(std::ifstream& in) {
    int value;
    in.read((char*) &value, sizeof (value));
    value = be32toh(value);
    return value;
}

inline void writeInt(std::ofstream &out, int i) {
    out.write(reinterpret_cast <const char*>(i), sizeof(i));
}

inline char readByte(std::ifstream& in) {
    char value;
    in.read((char*) &value, sizeof (value));
    return value;
}

inline void writeByte(std::ofstream &out, char c) {
    out.write(reinterpret_cast <const char*>(c), sizeof(c));
}

inline float readFloat(std::ifstream& in) {
    union v {
        float        f;
        unsigned int i;
    };
     
    union v val;
    in.read((char*) &val.f, sizeof (val.f));
     
    val.i = htobe32(val.i);
                   
    return val.f;
}

inline void writeFloat(std::ofstream &out, float f) {
    out.write((char*)(&f), sizeof(f));
}

inline uint64_t readLong(std::ifstream& in) {
    uint64_t value;
    in.read((char*) &value, sizeof (value));
    value = be64toh(value);
    return value;
}

inline void writeLong(std::ofstream &out, uint64_t l) {
    out.write(reinterpret_cast <const char*>(l), sizeof(l));
}

inline boost::dynamic_bitset<> readBitset(std::ifstream& in, int n) {
    long val = readLong(in);
    boost::dynamic_bitset<> value(n);

    for(int i = 0; i < n; i++) {
        value[i] = (val & 1<<i);
    }
    
    return value;    
}

#endif	/* FILES_H */

