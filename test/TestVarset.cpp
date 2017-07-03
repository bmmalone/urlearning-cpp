#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SIMPLEST_TEST_SUITE
#include <boost/test/unit_test.hpp>
#include "urlearning/base/typedefs.h"
#include <cmath>

#ifdef BOOST_VARSET

uint size = 128;

void checkVarsetEmpty(varset a, ulong size)
{
    BOOST_CHECK(size == a.size());
    BOOST_CHECK(true == a.none());    
}
void checkVarsetNotEmptyFrom(varset a, uint size, uint from, uint count, ulong value)
{
    BOOST_CHECK(size == a.size());
    BOOST_CHECK(false == a.none());
    BOOST_CHECK(true == a.any());
    BOOST_CHECK(from == a.find_first());
    BOOST_CHECK(count == a.count());
    BOOST_CHECK(value == a.to_ulong());    
}
BOOST_AUTO_TEST_CASE(VARSET_CONSTRUCTION)
{
    varset a = VARSET(size);
    checkVarsetEmpty(a,size);
}
BOOST_AUTO_TEST_CASE(VARSET_NEW_CONSTRUCTION)
{
    VARSET_NEW(a,size);
    checkVarsetEmpty(a,size);   
}
BOOST_AUTO_TEST_CASE(VARSET_NEW_INIT)
{
    ulong value = 15;
    VARSET_NEW_INIT(a,size,value);
    
    checkVarsetNotEmptyFrom(a,size,0,4,value);
}
BOOST_AUTO_TEST_CASE(VARSET_SET)
{
    uint index = 10;
    VARSET_NEW(a,size);
    VARSET_SET(a,10);
    
    BOOST_CHECK(index == a.find_first());
    BOOST_CHECK(true == a.any());
    BOOST_CHECK(1 == a.count());
}
BOOST_AUTO_TEST_CASE(VARSET_SET_VALUE)
{
    ulong value = 15;
    VARSET_NEW(a,10);
    VARSET_NEW_INIT(b,size,value);
    VARSET_SET_VALUE(a,b);
    
    checkVarsetNotEmptyFrom(a,size,0,4,value);
}
BOOST_AUTO_TEST_CASE(VARSET_SET_ALL)
{
    uint index = 10;
    VARSET_NEW(a,size);
    VARSET_SET_ALL(a,index);
    
    checkVarsetNotEmptyFrom(a,size,0,index,1023);
}
BOOST_AUTO_TEST_CASE(VARSET_CLEAR)
{
    VARSET_NEW_INIT(a,size,15);
    VARSET_CLEAR(a,0);
    
    checkVarsetNotEmptyFrom(a,size,1,3,14);
}
BOOST_AUTO_TEST_CASE(VARSET_CLEAR_INTERSECTION)
{
    VARSET_NEW_INIT(a,size,15);
    VARSET_NEW_INIT(b,size,7);
    
    VARSET_CLEAR_INTERSECTION(a,b);
    checkVarsetNotEmptyFrom(a,size,3,1,8);
}
BOOST_AUTO_TEST_CASE(VARSET_CLEAR_ALL)
{
    VARSET_NEW_INIT(a,size,15);
    
    VARSET_CLEAR_ALL(a);
    checkVarsetEmpty(a,size);
}
BOOST_AUTO_TEST_CASE(VARSET_GET)
{
    VARSET_NEW_INIT(a,size,15);
    
    BOOST_CHECK(1 == VARSET_GET(a,0));
    BOOST_CHECK(1 == VARSET_GET(a,1));
    BOOST_CHECK(1 == VARSET_GET(a,2));
    BOOST_CHECK(1 == VARSET_GET(a,3));
    BOOST_CHECK(0 == VARSET_GET(a,4));
    BOOST_CHECK(0 == VARSET_GET(a,5));
}
BOOST_AUTO_TEST_CASE(VARSET_LESS_THAN)
{
    VARSET_NEW_INIT(a,size,15);
    VARSET_NEW_INIT(b,size,31);
    
    BOOST_CHECK(true == VARSET_LESS_THAN(a,b));
    BOOST_CHECK(false == VARSET_LESS_THAN(b,a));
    BOOST_CHECK(false == VARSET_LESS_THAN(a,a));
}
BOOST_AUTO_TEST_CASE(VARSET_IS_SUBSET_OF)
{
    VARSET_NEW_INIT(a,size,15);
    VARSET_NEW_INIT(b,size,31);
    
    BOOST_CHECK(true == VARSET_IS_SUBSET_OF(a,b));
    BOOST_CHECK(false == VARSET_IS_SUBSET_OF(b,a));    
    BOOST_CHECK(true == VARSET_IS_SUBSET_OF(a,a)); //Note that set is defined to be subset of itself.
}
BOOST_AUTO_TEST_CASE(VARSET_FIND_FIRST_SET)
{
    VARSET_NEW_INIT(a,size,16);
    
    BOOST_CHECK(4 == VARSET_FIND_FIRST_SET(a));
}
BOOST_AUTO_TEST_CASE(VARSET_FIND_NEXT_SET)
{
    uint indexFirst = 4, indexSecond = 20;
    VARSET_NEW_INIT(a,size,16);
    VARSET_SET(a,indexSecond);
    
    BOOST_CHECK(indexFirst == VARSET_FIND_FIRST_SET(a));
    BOOST_CHECK(indexSecond == VARSET_FIND_NEXT_SET(a,indexFirst));
}
BOOST_AUTO_TEST_CASE(VARSET_NOT)
{
    VARSET_NEW_INIT(a,16,15);
        
    a = VARSET_NOT(a);
    BOOST_CHECK(4 == a.find_first());
    BOOST_CHECK(12 == a.count());
}
BOOST_AUTO_TEST_CASE(VARSET_AND)
{
    uint size = 16;
    ulong value = 15;
    VARSET_NEW_INIT(a,size,value);
    varset b = VARSET_NOT(a);
    
    varset c = VARSET_AND(a,b);
    checkVarsetEmpty(c,size);
    
    varset d = VARSET_AND(a,a);
    checkVarsetNotEmptyFrom(d,size,0,4,value);
}
BOOST_AUTO_TEST_CASE(VARSET_OR)
{
    uint size = 8;
    ulong value = 15;
    VARSET_NEW_INIT(a,size,value);
    varset b = VARSET_NOT(a);
    
    varset c = VARSET_OR(a,b);
    checkVarsetNotEmptyFrom(c,size,0,size,255);
}
BOOST_AUTO_TEST_CASE(VARSET_COPY)
{
    ulong value = 15;
    VARSET_NEW_INIT(a,size,value);
    
    VARSET_COPY(a,b);
    
    checkVarsetNotEmptyFrom(b,size,0,4,value);
}
BOOST_AUTO_TEST_CASE(VARSET_EQUAL)
{
    ulong value = 15;
    VARSET_NEW_INIT(a,size,value);
    VARSET_NEW_INIT(b,size,value);
    VARSET_NEW_INIT(c,size,value+1);
    
    BOOST_CHECK(true == VARSET_EQUAL(a,b));
    BOOST_CHECK(false == VARSET_EQUAL(a,c));
}

#endif
