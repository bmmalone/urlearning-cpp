#ifndef MERGED_TEMP_FILE_HPP
#define	MERGED_TEMP_FILE_HPP

#include <iostream>
#include <vector>
#include <cstdlib>

#include "urlearning/base/typedefs.h"

class MergedTempFile {
public:
    std::vector<varset> varsets;
    boost::unordered_map<varset,uint64_t> varsetCounts;
    std::string tempFile;
    long uniqueItemSum = 0;

    MergedTempFile(){
    }
    const bool addTempFile(std::string name, varset set, long uniqueItemsCount, long maxUniqueItems) {
        if (uniqueItemSum+uniqueItemsCount <= maxUniqueItems) {
            add(name,set,uniqueItemsCount);
            return true;
        }
        return false;
    }
    const void add(std::string name, varset set, long uniqueItemsCount) {
        std::cout<<"Adding "<<set<<" with name "<<name<<std::endl;
        varsets.push_back(set);
        tempFile = name;
        uniqueItemSum = uniqueItemSum + uniqueItemsCount;
        varsetCounts[set] = uniqueItemsCount;
    }
    const uint64_t getVarsetCount(varset set)
    {
        return varsetCounts[set];
    }
    const bool containsVarset(varset set)
    {
        if(std::find(varsets.begin(),varsets.end(),set)!=varsets.end()) return true;
        return false;
    }
    // This should be the smallest index variable that is in the group or that
    // can be added in to make the set belong to the group, right now just a hack
    const int getMinimumSetVariableInVarsetsIndex(varset toExpand, int variableCount)
    {
        int min = variableCount,
            n = -1;
        bool first = true;
        for(varset set : varsets)
        {
            if (first) n = hammingWeight(set);
            
            int localMin = variableCount;
            
            
            if (VARSET_IS_SUBSET_OF(set,toExpand)) 
            {
                localMin = lastSet(set); //we have first-k subcombination already   
            }
            else 
            {
                // find how many first-k bits are set in the first-k subcombination in question
                int bitCount = firstKSubcombinationBitCount(set,n);
                // we need to add a variable to create a first-k subcombination
                for(int i=0; i<variableCount+1; i++)
                {
                    if (VARSET_GET(toExpand, i)) continue;
                    
                    VARSET_COPY(toExpand, newSet);
                    VARSET_SET(newSet, i);
                    
                    if (firstNBitsEqual(set,newSet,bitCount)) 
                    {
                        localMin = i;
                        break;
                    }
                }
            }
                
            if (localMin < min) min = localMin;
        }
        return min;
    }
    const int getMinimumSetVariableInVarsets()
    {
        int min = 999,
            n = -1;
        bool first = true;
        for(varset set : varsets)
        {
            int localMin = VARSET_FIND_FIRST_SET(set);
            if (localMin < min) min = localMin;
        }
        return min;
    }
    const std::string getVarsetsString()
    {
        std::string out = "";
        for(varset set : varsets) out += varsetToString(set) + ",";
        return out;
    }
    //TODO will not work on more than 64...
    const int hammingWeight (varset vs) {
        ulong n = VARSET_TO_LONG(vs);
        int count = 0;
        while (n) {
            count += n & 0x1u;
            n >>= 1;
        }
        return count;
    }
    //TODO this will not work with more than 64...
    const int lastSet(varset vs)
    {
        ulong n = VARSET_TO_LONG(vs);
        unsigned r = 0;
        while (n >>= 1) 
        {
            r++;
        }
        return r;
    }
    int firstKSubcombinationBitCount(varset in, int bitCount){
        int count = 0;
        int lastSet = VARSET_FIND_FIRST_SET(in);
        count += lastSet;
        for(int i=0; i<bitCount-1; i++){
            lastSet = VARSET_FIND_NEXT_SET(in,lastSet);
            count += lastSet;
        }
        return count;
    }
};

#endif //MERGED_TEMP_FILE_HPP

