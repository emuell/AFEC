#include "CoreTypesPrecompiledHeader.h"

#include <windows.h>
#include <intrin.h>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

// Helper function to count set bits in the processor mask.
static DWORD SCountSetBits(ULONG_PTR bitMask)
{
  DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
  DWORD bitSetCount = 0;
  ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
  DWORD i;
    
  for (i = 0; i <= LSHIFT; ++i)
  {
    bitSetCount += ((bitMask & bitTest)?1:0);
    bitTest/=2;
  }
  
  return bitSetCount;
}

// -------------------------------------------------------------------------------------------------

static BOOL SCountLogicalProcessors(
  unsigned int* pTotalAvailLogical,
  unsigned int* pTotalAvailCores,
  unsigned int* pNumberOfPhysicalCpus)
{
  typedef BOOL (WINAPI *TPGetLogicalProcessorInformation)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
    PDWORD);

  TPGetLogicalProcessorInformation pGetLogicalProcessorInformation = 
    (TPGetLogicalProcessorInformation)::GetProcAddress(
      ::GetModuleHandle(L"kernel32"), 
      "GetLogicalProcessorInformation");
  
  if (pGetLogicalProcessorInformation == NULL) 
  {
    // GetLogicalProcessorInformation not available
    return FALSE;
  }

  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pBuffer = NULL;
  
  DWORD BufferSize = 0;
  if (pGetLogicalProcessorInformation(NULL, &BufferSize) || 
      BufferSize <= 0)
  {
    MInvalid("GetLogicalProcessorInformation failed");
    return FALSE;
  }

  pBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)::malloc(BufferSize);
  if (pGetLogicalProcessorInformation(pBuffer, &BufferSize) == FALSE)
  {
    MInvalid("GetLogicalProcessorInformation failed");

    ::free(pBuffer);
    return FALSE;
  }

  DWORD LogicalProcessorCount = 0;
  DWORD ProcessorCoreCount = 0;
  DWORD ProcessorPackageCount = 0;
  
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pProcessorInfo = pBuffer;
  DWORD Offset = 0;

  while (Offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= BufferSize) 
  {
    switch (pProcessorInfo->Relationship) 
    {
    case RelationNumaNode:
      // Non-NUMA systems report a single record of this type.
      // We don't care about numa nodes here.
      break;

    case RelationProcessorCore:
      // A hyper threaded core supplies more than one logical processor.
      LogicalProcessorCount += SCountSetBits(pProcessorInfo->ProcessorMask);
      ++ProcessorCoreCount;
      break;

    case RelationCache:
      // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
      // We don't care about caches here.
      break;

    case RelationProcessorPackage:
      // Logical processors share a physical package.
      ++ProcessorPackageCount;
      break;

    default:
      MInvalid("Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.");
      break;
    }

    Offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    ++pProcessorInfo;
  }
  
  ::free(pBuffer);

  if (LogicalProcessorCount == 0 || 
      ProcessorCoreCount == 0 || 
      ProcessorPackageCount == 0)
  {
    // Happens in WinXP 64bit SPII editions, probably because 
    // GetLogicalProcessorInformation is a stub there.
    return FALSE;
  }
  else
  {
    *pTotalAvailLogical = LogicalProcessorCount,
    *pTotalAvailCores = ProcessorCoreCount;
    *pNumberOfPhysicalCpus = ProcessorPackageCount;

    return TRUE;
  }
}

// =================================================================================================

namespace TIntel
{
  // -----------------------------------------------------------------------------------------------

  unsigned int MaxLogicalProcPerPhysicalProc(void);
  unsigned int MaxCorePerPhysicalProc(void);
  
  void CPUCount(
    unsigned int* TotAvailLogical,
    unsigned int* TotAvailCore,
    unsigned int* PhysicalNum);
  
  // -----------------------------------------------------------------------------------------------

  // CpuIDSupported will return 0 if CPUID instruction is unavailable. 
  // Otherwise, it will return the maximum supported standard function.
  
  unsigned int CpuIDSupported(void)
  {
    __try 
    {
      unsigned int CPUInfo[4] = { 0, 0, 0, 0 };

      // call cpuid with eax = 0
      __cpuid((int*)CPUInfo, 0);
      
      return CPUInfo[0];
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      // cpuid instruction is unavailable
      return 0;
    }
  }

  // -----------------------------------------------------------------------------------------------

  // GenuineIntel will return 0 if the processor is not a Genuine Intel Processor

  unsigned int GenuineIntel(void)
  {
    if (! CpuIDSupported())
    {
      return 0;
    }
    else
    {
      unsigned int CPUInfo[4] = {0, 0, 0, 0};
      // call cpuid with eax = 0
      __cpuid((int*)CPUInfo, 0);

      return
        (CPUInfo[1] == 'uneG') &&
        (CPUInfo[2] == 'letn') &&
        (CPUInfo[3] == 'Ieni');
    }
  }

  // -----------------------------------------------------------------------------------------------

  // The function returns 1 when the hardware multi-threaded bit set.

  unsigned int HWD_MTSupported(void)
  {
    if (! GenuineIntel())
    {
      return 0;
    }
    else
    {
      unsigned int CPUInfo[4] = { 0, 0, 0, 0 };

      // call cpuid with eax = 1
      __cpuid((int*)CPUInfo, 1);

      // EDX[28]  Bit 28 is set if HT or multi-core is supported
      #define HWD_MT_BIT 0x10000000     

      return (CPUInfo[3] & HWD_MT_BIT);
    }
  }

  // -----------------------------------------------------------------------------------------------

  // Function returns the maximum cores per physical package. Note that
  // the number of AVAILABLE cores per physical to be used by an application 
  // might be less than this maximum value.

  unsigned int MaxCorePerPhysicalProc(void)
  {
    unsigned int Regeax = 0;

    if (! HWD_MTSupported()) 
    {
      return (unsigned int)1;  // Single core
    }
    else
    {
      unsigned int CPUInfo[4] = { 0, 0, 0, 0 };
      // call cpuid with eax = 0
      __cpuid((int*)CPUInfo, 0);

      // check if cpuid supports leaf 4
      if (CPUInfo[0] > 4)
      {
        // call cpuid with eax = 4
        __cpuid((int*)CPUInfo, 4);
        Regeax = CPUInfo[0];
      }
      else
      {
        // single core
        Regeax = 0;
      }

      // EAX[31:26] Bit 26-31 in eax contains the number of cores minus one
      // per physical processor when execute cpuid with
      // eax set to 4.
      #define NUM_CORE_BITS 0xFC000000     

      return (unsigned int)((Regeax & NUM_CORE_BITS) >> 26) + 1;
    }
  }

  // -----------------------------------------------------------------------------------------------

  // Function returns the maximum logical processors per physical package. 
  // Note that the number of AVAILABLE logical processors per physical to 
  // be used by an application might be less than this maximum value.
  
  unsigned int MaxLogicalProcPerPhysicalProc(void)
  {
    if (! HWD_MTSupported()) 
    {
      return (unsigned int) 1;
    }
    else
    {
      unsigned int CPUInfo[4] = { 0, 0, 0, 0 };
      // call cpuid with eax = 1
      __cpuid((int*)CPUInfo, 1);

      // EBX[23:16] Bit 16-23 in ebx contains the number of logical
      // processors per physical processor when execute cpuid with
      // eax set to 1
      #define NUM_LOGICAL_BITS 0x00FF0000     
      
      return (unsigned int) ((CPUInfo[1] & NUM_LOGICAL_BITS) >> 16);
    }
  }

  // -----------------------------------------------------------------------------------------------

  unsigned char GetAPIC_ID(void)
  {
    if (! GenuineIntel())
    { 
      return 0;
    }
    else
    {
      unsigned int CPUInfo[4] = { 0, 0, 0, 0 };

      // call cpuid with eax = 1
      __cpuid((int*)CPUInfo, 1);
      
      // EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique
      // initial APIC ID for the processor this code is running on.
      #define INITIAL_APIC_ID_BITS  0xFF000000  

      return (unsigned char) ((CPUInfo[1] & INITIAL_APIC_ID_BITS) >> 24);
    }
  }

  // -----------------------------------------------------------------------------------------------

  // Determine the width of the bit field that can represent the value CountItem.

  unsigned int Find_MaskWidth(unsigned int CountItem)
  {
    int MaskWidth = 32;

    if (!CountItem)
    {
      return 0;
    }

    if (!(CountItem & 0xffff0000u))
    {
      CountItem <<= 16;
      MaskWidth -= 16;
    }

    if (!(CountItem & 0xff000000u))
    {
      CountItem <<= 8;
      MaskWidth -= 8;
    }

    if (!(CountItem & 0xf0000000u))
    {
      CountItem <<= 4;
      MaskWidth -= 4;
    }

    if (!(CountItem & 0xc0000000u))
    {
      CountItem <<= 2;
      MaskWidth -= 2;
    }

    if (!(CountItem & 0x80000000u))
    {
      CountItem <<= 1;
      MaskWidth -= 1;
    }

    return MaskWidth - 1;
  }

  // -----------------------------------------------------------------------------------------------

  // Extract the subset of bit field from the 8-bit value FullID.  
  // It returns the 8-bit sub ID value

  unsigned char GetNzbSubID(
    unsigned char FullID,
    unsigned char MaxSubIDValue,
    unsigned char ShiftCount)
  {
    unsigned int MaskWidth;
    unsigned char MaskBits;

    MaskWidth = Find_MaskWidth((unsigned int) MaxSubIDValue);
    
    MaskBits = (0xff << ShiftCount) ^
      ((unsigned char)(0xff << (ShiftCount + MaskWidth)));

    return (FullID & MaskBits);
  }

  // -----------------------------------------------------------------------------------------------

  void CPUCount(
    unsigned int* TotAvailLogical,
    unsigned int* TotAvailCore,
    unsigned int* PhysicalNum)
  {
    unsigned int numLPEnabled = 0;
    DWORD dwAffinityMask;
    int j = 0, MaxLPPerCore;
    unsigned char apicID, PackageIDMask;
    unsigned char tblPkgID[256] = {0};
    unsigned char tblCoreID[256] = {0};
    unsigned char tblSMTID[256] = {0};
    
    *TotAvailCore = 1;
    *PhysicalNum  = 1;

    DWORD_PTR dwProcessAffinity, dwSystemAffinity;

    ::GetProcessAffinityMask(::GetCurrentProcess(),
      &dwProcessAffinity, &dwSystemAffinity);

    // Assume that cores within a package have the SAME number of
    // logical processors.  Also, values returned by
    // MaxLogicalProcPerPhysicalProc and MaxCorePerPhysicalProc do not have
    // to be power of 2.
    MaxLPPerCore = MaxLogicalProcPerPhysicalProc() / MaxCorePerPhysicalProc();
    

    // ... Count logical

    dwAffinityMask = 1;
    while (dwAffinityMask && dwAffinityMask <= dwSystemAffinity)
    {
      if (SetThreadAffinityMask(GetCurrentThread(), dwAffinityMask))
      {
        Sleep(0);  // Ensure system to switch to the right CPU
        apicID = GetAPIC_ID();

        // Store SMT ID and core ID of each logical processor
        // Shift value for SMT ID is 0
        // Shift value for core ID is the mask width for maximum logical
        // processors per core

        tblSMTID[j]  = GetNzbSubID(apicID, (unsigned char)MaxLPPerCore, 0);
        
        tblCoreID[j] = GetNzbSubID(apicID, (unsigned char)MaxCorePerPhysicalProc(), 
          (unsigned char)Find_MaskWidth(MaxLPPerCore));

        // Extract package ID, assume single cluster.
        // Shift value is the mask width for max Logical per package

        PackageIDMask = (unsigned char)(0xff << Find_MaskWidth(MaxLogicalProcPerPhysicalProc()));

        tblPkgID[j] = apicID & PackageIDMask;
        
        numLPEnabled++;   // Number of available logical processors in the system.

      } // if

      j++;
      dwAffinityMask = 1 << j;
    } // while

    // restore the affinity setting to its original state
    ::SetThreadAffinityMask(::GetCurrentThread(), dwProcessAffinity);
    
    Sleep(0);
    
    *TotAvailLogical = numLPEnabled;

    
    // ... Count available cores (TotAvailCore) in the system

    unsigned char CoreIDBucket[256];
    DWORD ProcessorMask, pCoreMask[256];
    unsigned int i, ProcessorNum;

    CoreIDBucket[0] = tblPkgID[0] | tblCoreID[0];
    ProcessorMask = 1;
    pCoreMask[0] = ProcessorMask;

    for (ProcessorNum = 1; ProcessorNum < numLPEnabled; ProcessorNum++)
    {
      ProcessorMask <<= 1;
      
      for (i = 0; i < *TotAvailCore; i++)
      {
        // Comparing bit-fields of logical processors residing in different packages
        // Assuming the bit-masks are the same on all processors in the system.
        if ((tblPkgID[ProcessorNum] | tblCoreID[ProcessorNum]) == CoreIDBucket[i])
        {
          pCoreMask[i] |= ProcessorMask;
          break;
        }
      }  // for i

      if (i == *TotAvailCore)   // did not match any bucket.  Start a new one.
      {
        CoreIDBucket[i] = tblPkgID[ProcessorNum] | tblCoreID[ProcessorNum];
        pCoreMask[i] = ProcessorMask;

        (*TotAvailCore)++;  // Number of available cores in the system
      }
    }  // for ProcessorNum


    // ... Count physical processor (PhysicalNum) in the system

    unsigned char PackageIDBucket[256];
    DWORD pPackageMask[256];

    PackageIDBucket[0] = tblPkgID[0];
    ProcessorMask = 1;
    pPackageMask[0] = ProcessorMask;

    for (ProcessorNum = 1; ProcessorNum < numLPEnabled; ProcessorNum++)
    {
      ProcessorMask <<= 1;
      
      for (i = 0; i < *PhysicalNum; i++)
      {
        // Comparing bit-fields of logical processors residing in different packages
        // Assuming the bit-masks are the same on all processors in the system.
        if (tblPkgID[ProcessorNum]== PackageIDBucket[i])
        {
          pPackageMask[i] |= ProcessorMask;
          break;
        }
      }  // for i

      if (i == *PhysicalNum)   // did not match any bucket.  Start a new one.
      {
        PackageIDBucket[i] = tblPkgID[ProcessorNum];
        pPackageMask[i] = ProcessorMask;

        (*PhysicalNum)++;  // Total number of physical processors in the system
      }
    }  // for ProcessorNum
  }
}

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
};

// -------------------------------------------------------------------------------------------------

void TCpuImpl::Init()
{
  // ... HZ

  mHz = 0;

  HKEY RegKey; // lookup ~MHz in the registry
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_QUERY_VALUE, &RegKey) == ERROR_SUCCESS)
  {
    DWORD Data, Type, DataLen = (DWORD)sizeof(Data);
    if (::RegQueryValueEx(RegKey, L"~MHz", NULL, &Type, 
            (BYTE*)&Data, &DataLen) == ERROR_SUCCESS)
    {
      mHz = (unsigned int)Data * 1000000;
    }

    ::RegCloseKey(RegKey);
  }

  if (mHz == 0) // use rdtsc as fallback
  {
    unsigned __int64 SecondStart = 0, Freq = 0, SecondEnd = 0;
    if (::QueryPerformanceFrequency((LARGE_INTEGER*)&Freq))
    {
      ::QueryPerformanceCounter((LARGE_INTEGER*)&SecondEnd);
      SecondEnd += Freq / 100; // 10 ms

      const unsigned __int64 StartTime = __rdtsc();
      
      do 
      {
        ::QueryPerformanceCounter((LARGE_INTEGER *)&SecondStart);
      }
      while (SecondStart < SecondEnd);

      const unsigned __int64 EndTime = __rdtsc();
      mHz = (unsigned int)(EndTime - StartTime) * 100;
    }
  }


  // ... Cpu Flags

  TCpu::TCpuCapsFlags Caps = TCpu::kLegacy;

  unsigned int CPUInfo[4] = {0, 0, 0, 0};
  __cpuid((int*)CPUInfo, 0);
  const int nIds = CPUInfo[0];
  
  if (nIds >= 1)
  {
    __cpuid((int*)CPUInfo, 1);
    const int FeatureInfo = CPUInfo[3];

    if (FeatureInfo & (1 << 23))
    {
      Caps |= TCpu::kMmx;
    }

    if (FeatureInfo & (1 << 25))
    {
      Caps |= TCpu::kSse;
    }
    
    if (FeatureInfo & (1 << 26))
    {
      Caps |= TCpu::kSse2;
    }
  }

  // Calling __cpuid with 0x80000000 as the InfoType argument
  // gets the number of valid extended IDs.
  __cpuid((int*)CPUInfo, 0x80000000);
  unsigned int nExIds = CPUInfo[0];

  // Get the information associated with each extended ID.
  if (nExIds >= 0x80000001)
  {
    __cpuid((int*)CPUInfo, 0x80000001);

    if (CPUInfo[3] & 0x40000000)
    {
      Caps |= TCpu::kE3dnow;
    }
    
    if (CPUInfo[3] & 0x80000000)
    {
      Caps |= TCpu::k3dnow;
    }
  }
   
  mCaps = Caps;

  
  // ... Cpu Count

  // try windows "LogicalProcessorInfo" first, cause it works better with AMDs.
  // Unfortunately it's only available on WinXP >= SP3:
  if (::SCountLogicalProcessors(
          &mNumAvailableLogicalProcessors, 
          &mNumAvailableProcessorCores, 
          &mNumAvailablePhysicalProcessors))
  {
    // succeeded
  }
  else
  {
    // only use TIntel::CPUCount for Intel Processors to get rid of 
    // detection probs with AMDs
    if (TIntel::GenuineIntel())
    {
      TIntel::CPUCount(
        &mNumAvailableLogicalProcessors, 
        &mNumAvailableProcessorCores, 
        &mNumAvailablePhysicalProcessors);
    }
    // use SYSTEM_INFO as last fallback. Unfortunately does not give us very detailed info
    else
    {
      SYSTEM_INFO SysInfo;
      ::GetSystemInfo(&SysInfo);
    
      mNumAvailablePhysicalProcessors = MMax(1L, SysInfo.dwNumberOfProcessors);
      mNumAvailableLogicalProcessors = MMax(1L, mNumAvailablePhysicalProcessors);
      mNumAvailableProcessorCores = MMax(1L, mNumAvailablePhysicalProcessors);
    }
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
