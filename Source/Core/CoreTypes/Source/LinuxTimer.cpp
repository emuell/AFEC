#include "CoreTypesPrecompiledHeader.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TStamp::TStamp() 
{
  Start();
}

// -------------------------------------------------------------------------------------------------

void TStamp::Start()
{
  MStaticAssert(sizeof(timespec) == sizeof(mStart));
  ::clock_gettime(CLOCK_MONOTONIC, (struct timespec*)&mStart);
}

// -------------------------------------------------------------------------------------------------

float TStamp::DiffInMs()const
{
  struct timespec Now;
  ::clock_gettime(CLOCK_MONOTONIC, &Now);
  
  struct timespec* pStart = (struct timespec*)&mStart;
    
  return (float)((double)(Now.tv_sec - pStart->tv_sec) * 1000.0 + 
    (double)(Now.tv_nsec - pStart->tv_nsec) / 1000000.0);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

THighResolutionStamp::THighResolutionStamp() 
{
  Start();
}

// -------------------------------------------------------------------------------------------------

void THighResolutionStamp::Start()
{
  MStaticAssert(sizeof(timespec) == sizeof(mStart));
  ::clock_gettime(CLOCK_MONOTONIC, (struct timespec*)&mStart);
}

// -------------------------------------------------------------------------------------------------

float THighResolutionStamp::DiffInMs()const
{
  struct timespec Now;
  ::clock_gettime(CLOCK_MONOTONIC, &Now);
  
  struct timespec* pStart = (struct timespec*)&mStart;
    
  return (float)((double)(Now.tv_sec - pStart->tv_sec) * 1000.0 + 
    (double)(Now.tv_nsec - pStart->tv_nsec) / 1000000.0);
}

