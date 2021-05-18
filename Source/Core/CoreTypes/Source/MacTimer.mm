#include "CoreTypesPrecompiledHeader.h"

#include <Foundation/Foundation.h>

extern "C" {
  // the mach_time headers dont take care of this...
  #include <mach/mach.h>
  #include <mach/mach_time.h>
}

#include <pthread.h>
#include <unistd.h>

// =================================================================================================

// undefine to use Microseconds isntead of the mach timers...
#define MUseMachTime

// =================================================================================================
  
// -------------------------------------------------------------------------------------------------

#if defined(MUseMachTime)

static double sMachTimeBaseConversion = 0.0;
  
static void SInitMachTimeBase()
{
  MAssert(sMachTimeBaseConversion == 0.0, "Already initialized!");
  
  mach_timebase_info_data_t info;
  if (::mach_timebase_info(&info) == KERN_SUCCESS)
  {
    sMachTimeBaseConversion = 1e-6 * (double)info.numer / (double)info.denom;
  }
  else
  {
    MInvalid("mach_timebase_info failed!");
    
    if (TLog::SLog())
    {
      TLog::SLog()->AddLine("Timer", 
        "Error: mach_timebase_info is not available!");
    }
    else
    {
      ::perror("Error: mach_timebase_info is not available");
    }
    
    sMachTimeBaseConversion = 1e-6;
  }
}

#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TStamp::TStamp() 
{
  #if defined(MUseMachTime)
    if (sMachTimeBaseConversion == 0.0)
    {
      SInitMachTimeBase();
    }
  #endif
  
  Start();
}

// -------------------------------------------------------------------------------------------------

void TStamp::Start()
{
  #if defined(MUseMachTime)
    MStaticAssert(sizeof(uint64_t) == sizeof(mStart));
    mStart = (TTime_a)::mach_absolute_time();
  
  #else
    MStaticAssert(sizeof(UnsignedWide) == sizeof(mStart));
    ::Microseconds((UnsignedWide*)&mStart);
  #endif
}

// -------------------------------------------------------------------------------------------------

float TStamp::DiffInMs()const
{
  #if defined(MUseMachTime)
    const uint64_t Now = ::mach_absolute_time();
    return (float)((Now - (uint64_t)mStart) * sMachTimeBaseConversion);
  
  #else
    long long unsigned int Now;
    ::Microseconds((UnsignedWide*)&Now);

    return (float)((Now - (long long unsigned int)mStart) / 1000.0);
  #endif
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

THighResolutionStamp::THighResolutionStamp() 
{
  #if defined(MUseMachTime)
    if (sMachTimeBaseConversion == 0.0)
    {
      SInitMachTimeBase();
    }
  #endif
  
  Start();
}

// -------------------------------------------------------------------------------------------------

void THighResolutionStamp::Start()
{
  #if defined(MUseMachTime)
    MStaticAssert(sizeof(uint64_t) == sizeof(mStart));
    mStart = (TTime_a)::mach_absolute_time();

  #else
    MStaticAssert(sizeof(UnsignedWide) == sizeof(mStart));
    ::Microseconds((UnsignedWide*)&mStart);
  #endif
}

// -------------------------------------------------------------------------------------------------

float THighResolutionStamp::DiffInMs()const
{
  #if defined(MUseMachTime)
    const uint64_t Now = ::mach_absolute_time();
    return (float)((Now - (uint64_t)mStart) * sMachTimeBaseConversion);

  #else
    long long unsigned int Now;
    ::Microseconds((UnsignedWide*)&Now);

    return (float)((Now - (long long unsigned int)mStart) / 1000.0);
  #endif
}

