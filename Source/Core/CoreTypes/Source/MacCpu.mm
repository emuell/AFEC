#include "CoreTypesPrecompiledHeader.h"

#include <sys/sysctl.h>
#include <mach/mach.h>

// =================================================================================================

namespace TCpuImpl
{
  void Init();

  unsigned int mHz = 0;
  TCpu::TCpuCapsFlags mCaps = 0;

  unsigned int mNumAvailablePhysicalProcessors = 1;
  unsigned int mNumAvailableProcessorCores = 1;
  unsigned int mNumAvailableLogicalProcessors = 1;

  #if defined(MDebug)
    bool m__Initialized = false;
  #endif
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCpuImpl::Init()
{
  size_t Length = 0;

  // ... Hz
  
  unsigned int  Freq;
  Length = sizeof(Freq);

  int FreqMib[2] = { CTL_HW, HW_CPU_FREQ };

  // sysctlbyname fails here, dont ask me why...
  if (::sysctl(FreqMib, 2, &Freq, &Length, NULL, 0) != -1)
  {
    mHz = Freq;
  }
  else
  {
    MInvalid("Failed to query hw.cpufrequency");
    ::perror("Failed to query sysctl hw.cpufrequency. Assuming 2000000000");
    
    mHz = 2000000000;
  }
    
    
  // ... Caps
  
  mCaps = 0;
  
  #if defined(MArch_PPC)
    unsigned int VectorUnit;
    Length = sizeof(VectorUnit);

    if (::sysctlbyname("hw.vectorunit", &VectorUnit, &Length, NULL, 0) != -1)
    {
      if (VectorUnit != 0)
      {
        mCaps |= TCpu::kAltiVec;
      }
    }
    else
    {
      MInvalid("Failed to query hw.vectorunit");
      ::perror("Failed to query sysctl hw.vectorunit. Assuming false");
    }

  #elif defined(MArch_X86) || defined(MArch_X64)
    unsigned int  Mmx;
    Length = sizeof(Mmx);
  
    if (::sysctlbyname("hw.optional.mmx", &Mmx, &Length, NULL, 0) != -1)
    {
      if (Mmx)
      {
        mCaps |= TCpu::kMmx;
      }
    }
    else
    {
      MInvalid("Failed to query hw.optional.mmx");
      ::perror("Failed to query sysctl hw.optional.mmx. Assuming false");
    }

    unsigned int  sse;
    Length = sizeof(sse);
  
    if (::sysctlbyname("hw.optional.sse", &sse, &Length, NULL, 0) != -1)
    {
      if (sse)
      {
        mCaps |= TCpu::kSse;
      }
    }
    else
    {
      MInvalid("Failed to query hw.optional.sse");
      ::perror("Failed to query sysctl hw.optional.sse. Assuming false");
    }
    
    unsigned int  sse2;
    Length = sizeof(sse2);
  
    if (::sysctlbyname("hw.optional.sse2", &sse2, &Length, NULL, 0) != -1)
    {
      if (sse2)
      {
        mCaps |= TCpu::kSse2;
      }
    }
    else
    {
      MInvalid("Failed to query hw.optional.sse2");
      ::perror("Failed to query sysctl hw.optional.sse2. Assuming false");
    }
    
  #else
    #error "unknown platform"

  #endif

        
  // ... Processors / Cores
  
  struct host_basic_info hostinfo;
  mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
  kern_return_t result = ::host_info(mach_host_self(), 
    HOST_BASIC_INFO, reinterpret_cast<host_info_t>(&hostinfo), &count);
                                                
  if (result == KERN_SUCCESS)
  {
    mNumAvailablePhysicalProcessors = hostinfo.physical_cpu;
    mNumAvailableProcessorCores = hostinfo.physical_cpu_max;
    mNumAvailableLogicalProcessors = hostinfo.logical_cpu_max;
  }
  else
  {
    MInvalid("Failed to query host_info");
    ::perror("Failed to query host_info. Assume running on 1 CPU");
    
    mNumAvailablePhysicalProcessors = 1;
    mNumAvailableProcessorCores = 1;
    mNumAvailableLogicalProcessors = 1;
  }

  #if defined(MDebug)
    m__Initialized = true;
  #endif  
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCpu::Init()
{
  TCpuImpl::Init();
}

// -------------------------------------------------------------------------------------------------

unsigned int TCpu::Hz()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mHz;
}

// -------------------------------------------------------------------------------------------------

TCpu::TCpuCapsFlags TCpu::Caps()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mCaps;
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfConcurrentThreads()
{
  return NumberOfEnabledLogicalProcessors();
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfEnabledPhysicalProcessors()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mNumAvailablePhysicalProcessors;
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfEnabledProcessorCores()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mNumAvailableProcessorCores;
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfEnabledLogicalProcessors()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mNumAvailableLogicalProcessors;
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfCoresPerUnit()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");
  
  return TCpuImpl::mNumAvailableProcessorCores / 
  TCpuImpl::mNumAvailablePhysicalProcessors;
}

// -------------------------------------------------------------------------------------------------

int TCpu::NumberOfLogicalProcessorsPerUnit()
{
  MAssert(TCpuImpl::m__Initialized, "Not yet initialized");

  return TCpuImpl::mNumAvailableLogicalProcessors / 
    TCpuImpl::mNumAvailablePhysicalProcessors;
}

