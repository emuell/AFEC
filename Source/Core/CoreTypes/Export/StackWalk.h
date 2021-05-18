#pragma once

#ifndef _StackWalk_h_
#define _StackWalk_h_

// =================================================================================================

#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/BaseTypes.h"

#include <vector>
#include <string>

// =================================================================================================

namespace TStackWalk
{
  //! to be called on startup
  void Init();
  //! to be called on exit
  void Exit();
  
  //! Get frame pointers for the current stack. The FramePointers can
  //! later be resolved via 'DumpStack'
  void GetStack(
    std::vector<uintptr_t>& FramePointers,
    int                     Depth = -1,  
    int                     StartAt = 0);

  //! dump/resolve the passed stack (generated via Getstack) 
  //! to a list of strings. 
  void DumpStack(
    const std::vector<uintptr_t>& FramePointers, 
    std::vector<std::string>&     Stack);

  #if defined(MCompiler_VisualCPP)
    //! dump the whole stack when catching structured exceptions in windows
    //! used by visual CPP structured exception handlers (see Exception.h)
    void DumpExceptionStack(struct _EXCEPTION_POINTERS *ep);
  #endif
}


#endif  // _StackWalk_h_

