/* 
 * File:   typedefs.h
 * Author: malone
 *
 * Created on October 22, 2012, 10:33 PM
 */

#ifndef TYPEDEFS_H
#define	TYPEDEFS_H

#include <stdint.h>

#include <string>
#include <sstream>
#include <cmath>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <boost/algorithm/string.hpp>

// declare the BYTE data type
typedef uint8_t byte;

// convenience method to easily cast (some) things to string
#include <boost/lexical_cast.hpp>
#define TO_STRING(s) (boost::lexical_cast<std::string>(s))

/**
 *  Decide how to represent variable sets.
 *  Please note that the macro VARSET_NEW is quite sensitive.  Caveat emptor.
 *  Similarly, VARSET_COPY is very sensitive.
 */
#define BOOST_VARSET
//#define NATIVE_VARSET

/**
 *  Decide how to handle dynamic bitsets (i.e., for sparse parent graphs).
 */
#define BOOST_BITSET
//#define BITMAGIC_BITSET
//#define CUSTOM_BITSET

/**
 *  Decide how to handle unordered maps (i.e., for duplicate detection).
 */
#define BOOST_MAP
//#define GOOGLE_SPARSE_MAP
//#define GOOGLE_DENSE_MAP
//#define STD_MAP




/**
 *  Macros related to variable sets.
 */
#ifdef BOOST_VARSET
#include <boost/dynamic_bitset.hpp>
#include "dynamic_bitset_extensions.h"
typedef boost::dynamic_bitset<> varset;

#define VARSET(size) varset(size)
#define VARSET_NEW(varname, size) varset varname(size)
#define VARSET_NEW_INIT(varname, size, value) varset varname(size, value)
#define VARSET_SET(vs, index) (vs.set(index))
#define VARSET_SET_VALUE(vs, value) (vs = value)
#define VARSET_SET_ALL(vs, maxIndex) (vs = setFirstN(vs,maxIndex))
#define VARSET_CLEAR(vs, index) (vs.set(index, false))
#define VARSET_CLEAR_INTERSECTION(vs, other) (vs ^= other)
#define VARSET_CLEAR_ALL(vs) (vs.reset())
#define VARSET_GET(vs, index) (vs.test(index))
#define VARSET_LESS_THAN(vs, other) (varsetLessThan(vs,other))  //TODO this is not efficient at all, hopefully not that used
#define VARSET_IS_SUBSET_OF(vs, other) (vs.is_subset_of(other))  // Note that set is defined to be subset of itself.
#define VARSET_FIND_FIRST_SET(vs) (vs.find_first())
#define VARSET_FIND_NEXT_SET(vs, index) (vs.find_next(index)) 
#define VARSET_NOT(vs) (varset(vs).flip()) // TODO use of this in static_pattern_database requires a copy here, not efficient overall
#define VARSET_AND(vs, other) (vs & other)
#define VARSET_OR(vs, other) (vs |= other)
#define VARSET_COPY(vs, other) varset other(vs)
#define VARSET_EQUAL(vs, other) (varsetsEqual(vs,other))
#define VARSET_TO_LONG(vs) vs.to_ulong()

inline int previousSetBit(varset &vs, int index) {
    for (int i = index-1; i >= 0; i--) {
        if (VARSET_GET(vs, i)) {
            return i;
        }
    }
    return -1;
}
inline int lastSetBit(varset &vs) {
    return previousSetBit(vs, vs.size());
}
inline bool varsetLessThan(varset first, varset second)
{
    for(int i=first.size(); i>0; i--)
    {
        int indexFirst = previousSetBit(first,i);
        int indexSecond = previousSetBit(second,i);
        if (indexFirst < indexSecond) return true;
        else if (indexFirst > indexSecond) return false;
    }
    return false;
}
inline bool firstNBitsEqual(varset first, varset second, int n) 
{
    for(int i=0; i<n; i++)
    {
        if (first.test(i) != second.test(i)) return false;        
    }
    return true;
}
inline bool varsetsEqual(varset first, varset second)
{
    if (first.size() != second.size()) return false;
    for(int i=0; i<first.size(); i++)
    {
        if (first.test(i) != second.test(i)) return false;
    }
    return true;
}
inline varset newVarset(varset vs, ulong value)
{
    varset newVarset = varset(vs.size(),value);
    return newVarset;
}
inline varset setFirstN(varset vs, int n)
{
    for(int i=0; i<n; i++)
    {
        vs = vs.set(i,1);
    }
    return vs;
}
// TODO this doesn't quite work right now, replicates the difference between two bitsets made of ints
// Note that this is in lexicographical sense
inline long differenceBetweenVarsets(varset first, varset second){
    if (first.size() != second.size()) return -1; //should not be used in this case
    long result = 0;
    for(ulong i=0; i<first.size(); i++){
        if (first[i] != second[i]){
            if (first.test(i)) result += i*i; //define as the diff the position, we cannot use same as with longs as it might not fit in long
            else result -= i*i;
        }
    }
    return result;
}
inline byte cardinality(varset &vs) {
    return vs.count();
}
inline varset nextPermutation(varset &vs) {
    unsigned long variables = vs.to_ulong();
    unsigned long temp = (variables | (variables - 1)) + 1;
    unsigned long nextVariables(temp | ((((temp & -temp) / (variables & -variables)) >> 1) - 1));
    return varset(vs.size(), nextVariables);
}
inline varset varsetClearCopy(const varset &vs, int index) {
    varset cp(vs);
    cp.set(index, false);
    return cp;
}
inline std::string varsetToString(const varset &vs) {
    std::ostringstream os;
    os << vs;
    return os.str();
}
inline void printVarset(varset &vs) {
    printf("{");
    for (int i = 0; i < 64; i++) {
        if (VARSET_GET(vs, i)) {
            printf("%d, ", i);
        }
    }
    printf("}");
}
inline void setFromCsv(varset &vs, std::string csv) {
    // do nothing for empty strings
    if (csv.length() == 0) return;
    
    // split out the scc variables
    std::vector<std::string> tokens;
    boost::split(tokens, (csv), boost::is_any_of(","), boost::token_compress_on);

    for (auto v = tokens.begin(); v != tokens.end(); v++) {
        int var = atoi((*v).c_str());
        
        // at least a little error checking
        if (var == 0 && (*v)[0] != '0') {
            throw std::runtime_error("Invalid csv string: '" + csv + "'");
        }
        VARSET_SET(vs, var);
    }
}
#elif defined NATIVE_VARSET
#include <string.h>
typedef uint64_t varset;
#define VARSET_NEW(varname, size) varset varname(0)
#define VARSET(size) varset(0)
#define VARSET_NEW_INIT(varname, size, value) varset varname(value)
#define VARSET_SET(vs, index) (vs |= (1L << index))
#define VARSET_SET_VALUE(vs, value) (vs = varset(value))
#define VARSET_SET_ALL(vs, maxIndex) (vs |= (uint64_t)pow(2, maxIndex) - 1)
#define VARSET_CLEAR(vs, index) (vs ^= (1L << index))
#define VARSET_CLEAR_INTERSECTION(vs, other) (vs ^= other)
#define VARSET_CLEAR_ALL(vs) (vs = 0)
#define VARSET_GET(vs, index) (vs & (1L << index))
#define VARSET_LESS_THAN(vs, other) (vs < other)
#define VARSET_IS_SUBSET_OF(vs, other) ((vs & other) == vs)
#define VARSET_FIND_FIRST_SET(vs) (ffsl(vs) - 1) // -1 because, if the first bit is set, ffsl returns 1 (expecting 0)
#define VARSET_FIND_NEXT_SET(vs, index) (index + ffsl(vs >> (index+1)))
#define VARSET_NOT(vs) (~vs)
#define VARSET_AND(vs, other) (vs & other)
#define VARSET_OR(vs, other) (vs | other)
#define VARSET_COPY(vs, other) varset other(vs)
#define VARSET_EQUAL(vs, other) (vs == other)
#define VARSET_TO_LONG(vs) vs

inline int differenceBetweenVarsets(varset first, varset second){
    return first-second;
}
/**
 * Taken from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan.
 * 
 * @param v the variable set to count
 * @return 
 **/
byte inline cardinality(varset v) {
    byte layer = 0;
    for (layer = 0; v; layer++) {
        v &= v - 1; // clear the least significant bit set
    }
    return layer;
}
inline bool firstNBitsEqual(varset first, varset second, int n) {
    unsigned mask = ((1 << n)-1);
    return (first & mask)==(second & mask);
}
inline varset nextPermutation(varset &vs) {
    varset nextVariables; // next permutation of bits
    varset temp = (vs | (vs - 1)) + 1;
    nextVariables = temp | ((((temp & -temp) / (vs & -vs)) >> 1) - 1);
    return nextVariables;
}

inline varset varsetClearCopy(const varset &vs, int index) {
    return vs ^ (1L << index);
}

inline std::string varsetToString(const varset &vs) {
    std::ostringstream os;
    os << "{";
    for (int i = 0; i < 64; i++) {
        if (vs & (1L << i)) {
            os << i << ", ";
        }
    }
    os << "}";
    return os.str();
}

inline int previousSetBit(varset &vs, int index) {
    for (int i = index-1; i >= 0; i--) {
        if (VARSET_GET(vs, i)) {
            return i;
        }
    }
    return -1;
}

inline int lastSetBit(varset &vs) {
    return previousSetBit(vs, 64);
}

inline void printVarset(varset &vs) {
    printf("{");
    for (int i = 0; i < 64; i++) {
        if (VARSET_GET(vs, i)) {
            printf("%d, ", i);
        }
    }
    printf("}");
}
inline void setFromCsv(varset &vs, std::string csv) {
    // do nothing for empty strings
    if (csv.length() == 0) return;
    
    // split out the scc variables
    std::vector<std::string> tokens;
    boost::split(tokens, (csv), boost::is_any_of(","), boost::token_compress_on);

    for (auto v = tokens.begin(); v != tokens.end(); v++) {
        int var = atoi((*v).c_str());
        
        // at least a little error checking
        if (var == 0 && (*v)[0] != '0') {
            throw std::runtime_error("Invalid csv string: '" + csv + "'");
        }
        VARSET_SET(vs, var);
    }
}
#endif

/**
 *  Macros related to dynamic bitsets.
 */
#ifdef BOOST_BITSET
#include <boost/dynamic_bitset.hpp>
typedef boost::dynamic_bitset<> bitset;

#define BITSET_FREE(bs) (delete bs)
#define BITSET_CREATE(bs, size) boost::dynamic_bitset<> bs(size)
#define BITSET_NEW(size) (boost::dynamic_bitset<>(size))
#define BITSET_SET(bs, index) (bs.set(index))
#define BITSET_FLIP_ALL(bs) (bs.flip())
#define BITSET_SET_ALL(bs) (bs.set())
#define BITSET_AND(bs, other) (bs &= other)
#define BITSET_OR(bs, other) (bs |= other)
#define BITSET_FIRST_SET_BIT(bs) (bs.find_first())
#define BITSET_CLEAR(bs) (bs.reset())
#define BITSET_COUNT(bs) (bs.count())
#define BITSET_COPY(bs, other) boost::dynamic_bitset<> other(bs)


#elif defined BITMAGIC_BITSET
#include <bm/bm.h>
typedef bm::bvector<> bitset;

#define BITSET_FREE(bs) (delete bs)
#define BITSET_NEW(size) (bm::bvector<>(size))
#define BITSET_SET(bs, index) (bs.set_bit(index))
#define BITSET_FLIP_ALL(bs) (bs.flip())
#define BITSET_SET_ALL(bs) (bs.set())
#define BITSET_AND(bs, other) (bs &= other)
#define BITSET_FIRST_SET_BIT(bs) (bs.get_first())



#elif defined CUSTOM_BITSET
#include "bitset.h"
typedef struct BitSetStruct bitset;

#define BITSET_FREE(bs) (bitset_free(bs))
#define BITSET_NEW(size) (bitset_new(size))
#define BITSET_SET(bs, index) (bitset_set(bs, index))
#define BITSET_FLIP_ALL(bs) (bitset_flip_all(bs))
#define BITSET_SET_ALL(bs) (bitset_set_all(bs))
#define BITSET_AND(bs, other) (bitset_and(bs, other))
#define BITSET_FIRST_SET_BIT(bs) (bitset_first_set_bit(bs))
#endif

/**
 *  Macros related to unordered maps.
 */

class Node;
class DFSNode;

#ifdef BOOST_MAP
#include <boost/unordered_map.hpp>
typedef boost::unordered_map<varset, Node*> NodeMap;
typedef boost::unordered_map<varset, DFSNode*> DFSNodeMap;
typedef boost::unordered_map<varset, float> FloatMap;
typedef boost::unordered_map<varset, std::string> StringMap;

#elif defined GOOGLE_SPARSE_MAP
#include <google/sparse_hash_map>
typedef google::sparse_hash_map<varset, Node*> NodeMap;
typedef google::sparse_hash_map<varset, DFSNode*> DFSNodeMap;
typdef google::sparse_hash_map<varset, float> FloatMap;


#elif defined GOOGLE_DENSE_MAP
#include <google/dense_hash_map>
typedef google::dense_hash_map<varset, Node*> NodeMap;
typedef google::dense_hash_map<varset, DFSNode*> DFSNodeMap;
typdef google::dense_hash_map<varset, float> FloatMap;


#elif defined STD_MAP
#include <unordered_map>
typedef std::unordered_map<varset, Node*> NodeMap;
typedef std::unordered_map<varset, DFSNode*> DFSNodeMap;
typedef std::unordered_map<varset, float> FloatMap;


#endif

inline void init_map(NodeMap &map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

inline void init_map(DFSNodeMap &map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

inline void init_map(FloatMap &map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

inline void init_map(NodeMap *map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

inline void init_map(DFSNodeMap *map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

inline void init_map(FloatMap *map) {
#if defined(GOOGLE_SPARSE_MAP) || defined(GOOGLE_DENSE_MAP)
    map.set_deleted_key(-2);
    map.set_empty_key(-1);
#endif
}

#endif	/* TYPEDEFS_H */

