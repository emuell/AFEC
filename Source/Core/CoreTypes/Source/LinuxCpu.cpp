#include "CoreTypesPrecompiledHeader.h"

#include <utility> // pair
#include <set>

#include <cstring> // strchr, strstr, strcasecmp
#include <unistd.h>

// =================================================================================================

/*!
 * CPU related info, parsed from /proc/cpuinfo
!*/

struct TSystemInfo
{
  typedef std::pair<unsigned, unsigned> core_entry; // [physical ID, core id]

  static TSystemInfo SSystemInfo();

  // number of processor entries (available, but not necessary enabled!)
  unsigned int mNumberOfProcessors;
  int mProcessorLevel;
  int mProcessorRevision;
  long long mCpuHz;
  int mCpuFlags;

  // same as mNumberOfProcessors: available, but not necessary enabled
  std::set<core_entry> mCores;
  std::set<unsigned> mPhysicalProcessors;

private:
  TSystemInfo() { }
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSystemInfo TSystemInfo::SSystemInfo()
{
  TSystemInfo Info;

  Info.mNumberOfProcessors = 1;
  Info.mProcessorLevel = 5; /* 586 */
  Info.mProcessorRevision = 0;
  Info.mCpuHz = 0;
  Info.mCpuFlags = TCpu::kLegacy;
    
  core_entry current_core_entry;

  char line[200];
  FILE *f = ::fopen("/proc/cpuinfo", "r");

  if (!f)
  {
    MInvalid("Failed to open '/proc/cpuinfo'");
    ::perror("Failed to open '/proc/cpuinfo'");

    return Info;
  }
    
  while (::fgets(line, sizeof(line), f) != NULL)
  {
    char *s, *value;

    // NOTE: the ':' is the only character we can rely on
    if (!(value = ::strchr(line,':')))
    {
      continue;
    }
      
    // terminate the valuename
    s = value - 1;
    
    while ((s >= line) && ((*s == ' ') || (*s == '\t'))) 
    {
      --s;
    }
      
    *(s + 1) = '\0';

    // and strip leading spaces from value
    value += 1;
    
    while (*value == ' ')
    {
      ++value;
    }
      
    if ((s = ::strchr(value,'\n')))
    {
      *s = '\0';
    }

    if (!::strcasecmp(line, "processor"))
    {
      // processor number counts up...
      unsigned int x;
      if (::sscanf(value, "%d", &x))
      {
        if (x + 1 > (unsigned)Info.mNumberOfProcessors)
        {
          Info.mNumberOfProcessors = x + 1;
        }
      }
        
      continue;
    }

    if (!::strcasecmp(line, "physical id"))
    {
      int x;
      if (::sscanf(value, "%d", &x))
      {
        current_core_entry.first = x;
        Info.mPhysicalProcessors.insert(x);
      }

      continue;
    }

    if (!::strcasecmp(line, "core id"))
    {
      int x;
      if (::sscanf(value, "%d", &x))
      {
        current_core_entry.second = x;
        Info.mCores.insert(current_core_entry);
      }

      continue;
    }
    
    if (!::strcasecmp(line, "model"))
    {
      int  x;
      if (::sscanf(value, "%d", &x))
      {
        Info.mProcessorRevision = Info.mProcessorRevision | (x << 8);
      }
        
      continue;
    }

    // 2.1 method
    if (!::strcasecmp(line, "cpu family")) 
    {
      if (::isdigit(value[0]))
      {
        switch (value[0] - '0')
        {
        case 3: 
          Info.mProcessorLevel= 3;
          break;
        
        case 4: 
          Info.mProcessorLevel= 4;
          break;
        
        case 5: 
          Info.mProcessorLevel= 5;
          break;
        
        case 6: 
          Info.mProcessorLevel= 6;
          break;
        
        default:
          Info.mProcessorLevel = atoi(value);
          break;
        }
      }
      continue;
    }

    // old 2.0 method
    if (!::strcasecmp(line, "cpu")) 
    {
      if (::isdigit(value[0]) && 
          value[1] == '8' &&
          value[2] == '6' && 
          value[3] == 0) 
      {
        switch (value[0] - '0') 
        {
        case 3: 
          Info.mProcessorLevel= 3;
          break;
        
        case 4: 
          Info.mProcessorLevel= 4;
          break;
        
        case 5: 
          Info.mProcessorLevel= 5;
          break;
        
        case 6: 
          Info.mProcessorLevel= 6;
          break;
        
        default:
          Info.mProcessorLevel = atoi(value);
          break;
        }
      }
      continue;
    }
    
    if (!::strcasecmp(line,"stepping")) 
    {
      int  x;
      if (::sscanf(value, "%d", &x))
      {
        Info.mProcessorRevision = Info.mProcessorRevision | x;
      }
        
      continue;
    }
    
    if (!::strcasecmp(line, "cpu MHz")) 
    {
      double cmz;
      if (::sscanf(value, "%lf", &cmz) == 1) 
      {
        Info.mCpuHz = (long long)(cmz * 1000 * 1000);
      }
        
      continue;
    }
    
    if (!::strcasecmp(line,"flags")  ||
        !::strcasecmp(line,"features")) 
    {
      if (::strstr(value,"mmx"))
      {
        Info.mCpuFlags |= TCpu::kMmx;
      }
      
      if (::strstr(value,"3dnow"))
      {
        Info.mCpuFlags |= TCpu::k3dnow;
      }
      
      if (::strstr(value,"sse"))
      {
        Info.mCpuFlags |= TCpu::kSse;
      }
      
      if (::strstr(value,"sse2"))
      {
        Info.mCpuFlags |= TCpu::kSse2;
      }
      
      continue;
    }
  }
  
  ::fclose(f);

  return Info;
}

// =================================================================================================

namespace TCpuImpl
{
  void Init();

  unsigned int mHz = 0;
  TCpu::TCpuCapsFlags mCaps = TCpu::kLegacy;
  
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
  const TSystemInfo SysInfo = TSystemInfo::SSystemInfo();

  mNumAvailablePhysicalProcessors = MMax(1L, SysInfo.mPhysicalProcessors.size());
  mNumAvailableProcessorCores = MMax(1L, SysInfo.mCores.size());
  mNumAvailableLogicalProcessors = MMax(1L, ::sysconf(_SC_NPROCESSORS_ONLN));
  mHz = (unsigned int)SysInfo.mCpuHz;
  mCaps = SysInfo.mCpuFlags;
  
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

