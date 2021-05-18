#pragma once

#ifndef _SystemGlobals_h_
#define _SystemGlobals_h_

// =================================================================================================

#include "CoreTypes/Export/System.h"
#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Directory.h"

#include <list>
#include <string>

// =================================================================================================

/*!
 * Global variables used internally by TSystem
!*/

class TSystemGlobals
{
public:
  //! Access to the singleton
  static TSystemGlobals* SSystemGlobals();

  //! Create/destroy the singleton.
  static void SCreate();
  static void SDestroy();

  //! App (window) handles
  void* mAppHandle = nullptr;

  //! Message (redirection)
  TSystem::TPShowMessageFunc mShowMessageFunc = TSystem::StandardMessageRedirector;

  int mSupressInfoMessages = 0;

  //! Process killed handlers
  typedef std::list<TSystem::TPProcessKilledHandlerFunc> TPProcessKilledHandlerList;
  TPProcessKilledHandlerList mProcessKilledHandlers;

  //! Suspend mode handlers
  typedef std::list<TSystem::TPSuspendModeHandlerFunc> TPSuspendModeHandlerList; 
  TPSuspendModeHandlerList mSuspendModeHandlers;

private:
  TSystemGlobals();

  static TSystemGlobals* spSystemGlobals;
};


#endif // _SystemGlobals_h_

