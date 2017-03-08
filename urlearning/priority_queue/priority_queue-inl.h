/* 
 * File:   priority_queue-inl.h
 * Author: malone
 *
 * Created on October 23, 2012, 4:01 PM
 */

#ifndef PRIORITY_QUEUE_INL_H
#define	PRIORITY_QUEUE_INL_H

#include <iterator>

#include "urlearning/base/node.h"

namespace priority_queue {

template<typename _RandomAccessIterator, typename _Distance, typename _Tp,
typename _Compare>
void
__push_heap(_RandomAccessIterator __first, _Distance __holeIndex,
        _Distance __topIndex, _Tp __value, _Compare __comp, int firstIndex) {
    _Distance __parent = (__holeIndex - 1) / 2;

    while (__holeIndex > __topIndex
            && __comp(*(__first + __parent), __value)) {
        *(__first + __holeIndex) = _GLIBCXX_MOVE(*(__first + __parent));
        Node* n = *(__first + __parent);
        n->setPqPos(firstIndex + __holeIndex);


        __holeIndex = __parent;

        __parent = (__holeIndex - 1) / 2;
    }
    *(__first + __holeIndex) = _GLIBCXX_MOVE(__value);

    Node* n = *(__first + __holeIndex);
    n->setPqPos(firstIndex + __holeIndex);
}

/**
 *  @brief  Push an element onto a heap using comparison functor.
 *  @param  first  Start of heap.
 *  @param  last   End of heap + element.
 *  @param  comp   Comparison functor.
 *  @ingroup heap_algorithms
 *
 *  This operation pushes the element at last-1 onto the valid heap over the
 *  range [first,last-1).  After completion, [first,last) is a valid heap.
 *  Compare operations are performed using comp.
 */
template<typename _RandomAccessIterator, typename _Compare>
inline void
push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
        _Compare __comp, int firstIndex) {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
    _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type
    _DistanceType;

    // concept requirements
    __glibcxx_function_requires(_Mutable_RandomAccessIteratorConcept<
            _RandomAccessIterator>)
            __glibcxx_requires_valid_range(__first, __last);
    __glibcxx_requires_heap_pred(__first, __last - 1, __comp);

    _ValueType __value = _GLIBCXX_MOVE(*(__last - 1));
    __push_heap(__first, _DistanceType((__last - __first) - 1),
            _DistanceType(0), _GLIBCXX_MOVE(__value), __comp, firstIndex);
}

template<typename _RandomAccessIterator, typename _Distance,
typename _Tp, typename _Compare>
void
__adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
        _Distance __len, _Tp __value, _Compare __comp, int firstIndex) {
    const _Distance __topIndex = __holeIndex;
    _Distance __secondChild = __holeIndex;
    while (__secondChild < (__len - 1) / 2) {
        __secondChild = 2 * (__secondChild + 1);
        if (__comp(*(__first + __secondChild),
                *(__first + (__secondChild - 1))))
            __secondChild--;

        *(__first + __holeIndex) = _GLIBCXX_MOVE(*(__first + __secondChild));
        Node* n = *(__first + __secondChild);
        n->setPqPos(firstIndex + __holeIndex);

        __holeIndex = __secondChild;
    }
    if ((__len & 1) == 0 && __secondChild == (__len - 2) / 2) {
        __secondChild = 2 * (__secondChild + 1);
        *(__first + __holeIndex) = _GLIBCXX_MOVE(*(__first
                + (__secondChild - 1)));

        Node* n = *(__first + (__secondChild - 1));
        n->setPqPos(firstIndex + __holeIndex);

        __holeIndex = __secondChild - 1;
    }
    __push_heap(__first, __holeIndex, __topIndex,
            _GLIBCXX_MOVE(__value), __comp, firstIndex);
}

template<typename _RandomAccessIterator, typename _Compare>
inline void
__pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
        _RandomAccessIterator __result, _Compare __comp, int firstIndex) {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
    _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type
    _DistanceType;

    // I may need to do something with the indices here
    // I'm not sure if this actually moves anything or not.
    _ValueType __value = _GLIBCXX_MOVE(*__result);
    *__result = _GLIBCXX_MOVE(*__first);
    __adjust_heap(__first, _DistanceType(0),
            _DistanceType(__last - __first),
            _GLIBCXX_MOVE(__value), __comp, firstIndex);
}

/**
 *  @brief  Pop an element off a heap using comparison functor.
 *  @param  first  Start of heap.
 *  @param  last   End of heap.
 *  @param  comp   Comparison functor to use.
 *  @ingroup heap_algorithms
 *
 *  This operation pops the top of the heap.  The elements first and last-1
 *  are swapped and [first,last-1) is made into a heap.  Comparisons are
 *  made using comp.
 */
template<typename _RandomAccessIterator, typename _Compare>
inline void
pop_heap(_RandomAccessIterator __first,
        _RandomAccessIterator __last, _Compare __comp, int firstIndex) {
    // concept requirements
    __glibcxx_function_requires(_Mutable_RandomAccessIteratorConcept<
            _RandomAccessIterator>)
            __glibcxx_requires_valid_range(__first, __last);
    __glibcxx_requires_heap_pred(__first, __last, __comp);

    --__last;
    __pop_heap(__first, __last, __last, __comp, firstIndex);
}

template<typename _RandomAccessIterator, typename _Distance, typename _Tp, typename _Compare>
inline void
__up_heap(_RandomAccessIterator __first, _RandomAccessIterator __end, _RandomAccessIterator __pos,
        _Distance, _Tp __value, _Compare __comp, int firstIndex) {
    _Distance __parent = (__pos - __first - 1) / 2;
    _Distance __index = __pos - __first;

    while (__index > 0 && __comp(*(__first + __parent), __value)) {
        *(__first + __index) = *(__first + __parent);
        Node* n = *(__first + __parent);
        n->setPqPos(firstIndex + __index);


        __index = __parent;
        __parent = (__parent - 1) / 2;
    }

    if (__pos != (__first + __index)) {
        *(__first + __index) = __value;
        Node* n = *(__first + __index);
        n->setPqPos(firstIndex + __index);


    }

}

template<typename _RandomAccessIterator, typename _Distance, typename _Tp, typename _Compare>
inline void
__down_heap(_RandomAccessIterator __first, _RandomAccessIterator __last, _RandomAccessIterator __pos,
        _Distance, _Tp __value, _Compare __comp, int firstIndex) {
    _Distance __len = __last - __first;
    _Distance __index = __pos - __first;
    _Distance __left = __index * 2 + 1;
    _Distance __right = __index * 2 + 2;
    _Distance __largest = __len;
    

    while (__index < __len) {
        if ( (__right >= __len) || ((__left < __len) && (__comp(*(__first + __right), *(__first + __left))))) {
            __largest = __left;
        }
            
        if (__largest < __len && __comp(__value, *(__first + __largest))) {
            *(__first + __index) = *(__first + __largest);

            Node* n = *(__first + __largest);
            n->setPqPos(firstIndex + __index);
            


            __index = __largest;
            __left = __index * 2 + 1;
            __right = __index * 2 + 2;
        } else
            break;
    }

    if (__pos != (__first + __index))
        *(__first + __index) = __value;
}

template<typename _RandomAccessIterator, typename _Distance, typename _Compare>
inline void
__update_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
        _RandomAccessIterator __pos, _Distance, _Compare __comp) {

    int firstIndex = 0;

    _Distance __index = (__pos - __first);
    _Distance __parent = (__index - 1) / 2;

    if (__index > 0 && __comp(*(__first + __parent), *(__pos)))
        __up_heap(__first, __last, __pos, _Distance(), *(__pos), __comp, firstIndex);
    else
        __down_heap(__first, __last, __pos, _Distance(), *(__pos), __comp, firstIndex);
}

template<typename _RandomAccessIterator, typename _Compare>
inline void
update_heap_pos(_RandomAccessIterator __first, _RandomAccessIterator __last,
        _RandomAccessIterator __pos, _Compare __comp) {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _DistanceType;

    __update_heap(__first, __last, __pos, _DistanceType(), __comp);
}

};

#endif	/* PRIORITY_QUEUE_INL_H */

