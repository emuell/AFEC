#include "CoreTypesPrecompiledHeader.h"

#include <windows.h>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TStamp::TStamp() 
{
  Start();
}

// -------------------------------------------------------------------------------------------------

void TStamp::Start()
{
  mStart = (TTime_a)::timeGetTime(); 
}

// -------------------------------------------------------------------------------------------------

float TStamp::DiffInMs()const
{
  return (float)((TTime_a)::timeGetTime() - mStart);
}

// =================================================================================================

static bool sHighResolutionStampIsInitialized = false;

static bool sUseQueryPerformanceCounter = false;
static LARGE_INTEGER sQueryPerformceCounterFreq;

// -------------------------------------------------------------------------------------------------

THighResolutionStamp::THighResolutionStamp() 
{
  if (!sHighResolutionStampIsInitialized) 
  {
    sHighResolutionStampIsInitialized = true;
     
    /*
      * Some hardware abstraction layers use the CPU clock in place of
      * the real-time clock as a performance counter reference. This
      * results in:
      *    - inconsistent results among the processors on
      *      multi-processor systems.
      *    - unpredictable changes in performance counter frequency on
      *      "gearshift" processors such as Transmeta and SpeedStep.
      *
      * There seems to be no way to test whether the performance
      * counter is reliable, but a useful heuristic is that if its
      * frequency is 1.193182 MHz or 3.579545 MHz, it's derived from a
      * colorburst crystal and is therefore the RTC rather than the
      * TSC.
      *
      * A sloppier but serviceable heuristic is that the RTC crystal is
      * normally less than 15 MHz while the TSC crystal is virtually
      * assured to be greater than 100 MHz. Since Win98SE appears to
      * fiddle with the definition of the perf counter frequency
      * (perhaps in an attempt to calibrate the clock?), we use the
      * latter rule rather than an exact match.
      *
      * We also assume (perhaps questionably) that the vendors have
      * gotten their act together on Win64, so bypass all this rubbish
      * on that platform.
      */

    const bool QueryPerformanceFrequencySucceeded = !!::QueryPerformanceFrequency(
      &sQueryPerformceCounterFreq);
      
    // first guess: check if QueryPerformanceFrequency returns an error
    sUseQueryPerformanceCounter = QueryPerformanceFrequencySucceeded;

    #if defined(MArch_X86)

      if (QueryPerformanceFrequencySucceeded)
      {
        // The following lines would do an exact match on crystal frequency:
        // if (sQueryPerformceCounterFreq.QuadPart != 1193182LL &&
        //     sQueryPerformceCounterFreq.QuadPart != 3579545LL)
       
        if (sQueryPerformceCounterFreq.QuadPart > 15000000LL)
        {
          // looks like a TSC crystal
          sUseQueryPerformanceCounter = false;

          // As an exception, if every logical processor on the system
          // is on the same chip, we use the performance counter anyway,
          // presuming that everyone's TSC is locked to the same oscillator
          int regs[4];

          // "Genu" "ineI" "ntel" ?
          __cpuid(regs, 0);
            
          if (regs[1] == 0x756e6547  && 
              regs[3] == 0x49656e69  && 
              regs[2] == 0x6c65746e)
          {
            SYSTEM_INFO systemInfo;
            ::GetSystemInfo(&systemInfo);

            // CPUs count matches systems logical CPU count ?
            __cpuid(regs, 1);

            if (((regs[1]&0x00FF0000) >> 16) == (int)systemInfo.dwNumberOfProcessors) 
            {
              sUseQueryPerformanceCounter = true;
            } 
          }
        } 
      }
    #endif

    if (sUseQueryPerformanceCounter)
    {
      TLog::SLog()->AddLine("Timer", 
        "Seems safe to use the 'QueryPerformance' counters...");
    }
    else
    {
      TLog::SLog()->AddLine("Timer", "Can't use 'QueryPerformance' counters. "
        "Falling back to a low resolution timer...");
    }
  }

  Start();
}

// -------------------------------------------------------------------------------------------------

void THighResolutionStamp::Start()
{
  if (!sUseQueryPerformanceCounter)
  {
    mStart = (TTime_a)::timeGetTime(); 
  }
  else
  {
    LARGE_INTEGER Now;
    ::QueryPerformanceCounter(&Now);

    MStaticAssert(sizeof(Now.QuadPart) == sizeof(mStart));
    mStart = (TTime_a)Now.QuadPart;
  }
}

// -------------------------------------------------------------------------------------------------

float THighResolutionStamp::DiffInMs()const
{
  MAssert(mStart != 0, "no Start was called to init the stamp");

  if (!sUseQueryPerformanceCounter)
  {
    return (float)((TTime_a)::timeGetTime() - mStart);
  }
  else
  {
    LARGE_INTEGER Now;
    ::QueryPerformanceCounter(&Now);

    return (float)(((double)Now.QuadPart - (double)mStart) / 
      (double)sQueryPerformceCounterFreq.QuadPart * 1000.0);
  }
}

