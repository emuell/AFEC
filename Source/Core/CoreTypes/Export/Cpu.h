#pragma once

#ifndef _Cpu_h_
#define _Cpu_h_

// =================================================================================================

namespace TCpu
{
  //! Init the CPU structs (count processors, Hz...). To be called by
  //! the project init function.
  void Init();

  enum TCpuCaps
  {
    kLegacy         = 0,
    kMmx            = 1 << 0,
    k3dnow          = 1 << 1,
    kE3dnow         = 1 << 2,
    kSse            = 1 << 3,
    kSse2           = 1 << 4,
    kAltiVec        = 1 << 5
  };
  typedef unsigned int TCpuCapsFlags;

  //! Returns the CPUs features as listed in TCpuCaps.
  TCpuCapsFlags Caps();
  
  //! Return the processor speed in Hz.
  unsigned int Hz();

  //! Returns the number of threads the CPU can deal with most efficiently.
  //! Usually will be the number of logical processor, but may also take other
  //! things into account.
  int NumberOfConcurrentThreads();
  
  //! Return the number of physical CPU units (not cores!) that are currently
  //! enabled for this system/process.
  int NumberOfEnabledPhysicalProcessors();
  //! Returns the total number of cores that are currently enabled for this 
  //! system and process.
  int NumberOfEnabledProcessorCores();
  //! Returns the total number of logical CPUS (physical CPUs X cores X HT clones) 
  //! that are currently enabled for this system and process.
  int NumberOfEnabledLogicalProcessors();

  //! Return the number of cores that are available *per* physical processor unit, 
  //! assuming that all of them are enabled for this system and process.
  int NumberOfCoresPerUnit();
  //! Return the number of CPU`s that are available *per* physical processor unit
  //! (a single hyper threading CPU will have two logical CPU`s if hyper threading 
  //! is enabled), assuming that all of them are enabled for this system and process
  int NumberOfLogicalProcessorsPerUnit();
}


#endif // _Cpu_h_

