#pragma once

#ifndef _Timer_h_
#define _Timer_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/BaseTypes.h"

#include <ctime> // time_t

// =================================================================================================

class TStamp
{
public:
  TStamp();
  
  //! start a new time measure 
  //! (automatically called while constructing)
  void Start();

  //! @return the diff in ms from the last start to now
  float DiffInMs()const;

private:
  // size and type of the time value is a platform detail
  #if defined(MLinux)
    typedef char TTime[sizeof(time_t) + sizeof(long)];
    typedef TTime __attribute__((__may_alias__)) TTime_a;
    TTime_a mStart;

  #else
    typedef TUInt64 TTime_a;
    TTime_a mStart;
  #endif
};
  
// =================================================================================================

//! same as TStamp, but uses a high resolution timer which produces more overhead

class THighResolutionStamp
{
public:
  THighResolutionStamp();
  
  //! start a new time measure 
  //! (automatically called while constructing)
  void Start();

  //! @return the diff in ms from the last start to now
  float DiffInMs()const;

private:
  // size and type of the time value is a platform detail
  #if defined(MLinux)
    typedef char TTime[sizeof(time_t) + sizeof(long)];
    typedef TTime __attribute__((__may_alias__)) TTime_a;
    TTime_a mStart;

  #else
    typedef TUInt64 TTime_a;
    TTime_a mStart;
  #endif
};


#endif // _Timer_h_

