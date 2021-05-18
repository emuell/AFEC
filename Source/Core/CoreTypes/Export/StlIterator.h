#pragma once

#ifndef _StlIterator_h_
#define _StlIterator_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"

#include <iterator>

// =================================================================================================

/*!
 * Baseclass for a std::iterator conform random access iterator
!*/

template<typename TValueType> 
class TStlRandomAccessIterator
{
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef TValueType value_type;
  typedef ptrdiff_t difference_type;
  typedef ptrdiff_t distance_type;
  typedef TValueType* pointer;
  typedef TValueType& reference;
};

// =================================================================================================

/*!
 * Baseclass for a std::iterator conform bidirectional iterator
!*/

template<typename TValueType> 
class TStlBidirectionalIterator
{
public:
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef TValueType value_type;
  typedef ptrdiff_t difference_type;
  typedef ptrdiff_t distance_type;
  typedef TValueType* pointer;
  typedef TValueType& reference;
};


#endif // _StlIterator_h_

