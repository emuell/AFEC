#include "CoreTypesPrecompiledHeader.h"

#include <cstdio>
#include <clocale>
#include <cstring>

#include <algorithm>
#include <iostream>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSystem::TSuppressInfoMessages::TSuppressInfoMessages()
{
  ++TSystemGlobals::SSystemGlobals()->mSupressInfoMessages;
}

// -------------------------------------------------------------------------------------------------

TSystem::TSuppressInfoMessages::~TSuppressInfoMessages()
{
  --TSystemGlobals::SSystemGlobals()->mSupressInfoMessages;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TSystem::TSetAndRestoreLocale::TSetAndRestoreLocale(
  int         LocaleCategory, 
  const char* pNewLocaleSetting)
  : mLocaleCategory(LocaleCategory),
    mpOldLocaleSetting(NULL),
    mpNewLocaleSetting(pNewLocaleSetting)
{
  MAssert(pNewLocaleSetting, "New locale can not be NULL!");

  const char* pOldLocaleSetting = ::setlocale(LocaleCategory, NULL);
  
  if (pOldLocaleSetting == NULL || 
      ::strcmp(pOldLocaleSetting, pNewLocaleSetting) != 0)
  {
    if (pOldLocaleSetting)
    {
      mpOldLocaleSetting = (char*)::malloc(::strlen(pOldLocaleSetting) + 1);
      ::strcpy((char*)mpOldLocaleSetting, pOldLocaleSetting);
    }

    if (::setlocale(LocaleCategory, pNewLocaleSetting) == NULL)
    {
      MInvalid("Failed to set a new locale!");
    }
  }
}

// -------------------------------------------------------------------------------------------------

TSystem::TSetAndRestoreLocale::~TSetAndRestoreLocale()
{
  if (mpOldLocaleSetting != NULL &&
      ::strcmp(mpOldLocaleSetting, mpNewLocaleSetting) != 0)
  {
    if (::setlocale(mLocaleCategory, mpOldLocaleSetting) == NULL)
    {
      MInvalid("Failed to reset a locale!");
    }
  }
  
  if (mpOldLocaleSetting)
  {
    ::free((void*)mpOldLocaleSetting);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TSystem::AddProcessKilledHandler(TPProcessKilledHandlerFunc pFunc)
{
  TSystemGlobals::TPProcessKilledHandlerList& ProcessKilledHandlers =
    TSystemGlobals::SSystemGlobals()->mProcessKilledHandlers;

  MAssert(std::find(
    ProcessKilledHandlers.begin(), ProcessKilledHandlers.end(), pFunc) == 
    ProcessKilledHandlers.end(), "Already added!");

  ProcessKilledHandlers.push_back(pFunc);
}

// -------------------------------------------------------------------------------------------------

void TSystem::RemoveProcessKilledHandler(TPProcessKilledHandlerFunc pFunc)
{
  TSystemGlobals::TPProcessKilledHandlerList& ProcessKilledHandlers =
    TSystemGlobals::SSystemGlobals()->mProcessKilledHandlers;

  MAssert(std::find(
    ProcessKilledHandlers.begin(), ProcessKilledHandlers.end(), pFunc) != 
    ProcessKilledHandlers.end(), "Was not added!");

  ProcessKilledHandlers.remove(pFunc);
}

// -------------------------------------------------------------------------------------------------

void TSystem::AddSuspendModeHandler(TPSuspendModeHandlerFunc pFunc)
{
  TSystemGlobals::TPSuspendModeHandlerList& SuspendModeHandlers = 
    TSystemGlobals::SSystemGlobals()->mSuspendModeHandlers;

  MAssert(std::find(SuspendModeHandlers.begin(), 
    SuspendModeHandlers.end(), pFunc) == SuspendModeHandlers.end(), 
    "Already added!");
  
  SuspendModeHandlers.push_back(pFunc);
}

// -------------------------------------------------------------------------------------------------

void TSystem::RemoveSuspendModeHandler(TPSuspendModeHandlerFunc pFunc)
{
  TSystemGlobals::TPSuspendModeHandlerList& SuspendModeHandlers = 
    TSystemGlobals::SSystemGlobals()->mSuspendModeHandlers;

  MAssert(std::find(SuspendModeHandlers.begin(), 
    SuspendModeHandlers.end(), pFunc) != SuspendModeHandlers.end(), 
    "Was not added!");
  
  SuspendModeHandlers.remove(pFunc);
}

// -------------------------------------------------------------------------------------------------

void TSystem::CallSuspendHandlers(TSuspendMode Mode)
{
  TSystemGlobals::TPSuspendModeHandlerList& SuspendModeHandlers = 
    TSystemGlobals::SSystemGlobals()->mSuspendModeHandlers;

  TSystemGlobals::TPSuspendModeHandlerList::const_iterator Iter = 
    SuspendModeHandlers.begin();

  for (; Iter != SuspendModeHandlers.end(); ++Iter)
  {
    (*Iter)(Mode);
  }
}

// -------------------------------------------------------------------------------------------------

void TSystem::ShowMessage(const TString& Message, TSystem::TMessageType Type)
{
  // dump error & warning messages to the log...
  if (Type != kInfo && TLog::SLog())
  {
    const TList<TString> Paragraphs(Message.Paragraphs());
    
    for (int i = 0; i < Paragraphs.Size(); ++i)
    {
      if (!TString(Paragraphs[i]).Strip().IsEmpty())
      {
        TLog::SLog()->AddLine((Type == kError) ? 
          "Error Message" : "Warning Message", "%s", 
          Paragraphs[i].CString());
      }
    }
  }

  if (! TSystemGlobals::SSystemGlobals()->mSupressInfoMessages || Type != kInfo)
  {
    TSystemGlobals::SSystemGlobals()->mShowMessageFunc(Message, Type);
  }
}

// -------------------------------------------------------------------------------------------------

bool TSystem::InfoMessagesAreSuppressed()
{
  return (TSystemGlobals::SSystemGlobals()->mSupressInfoMessages > 0);
}

// -------------------------------------------------------------------------------------------------

void TSystem::StandardMessageRedirector(
  const TString&        Message, 
  TSystem::TMessageType Type)
{
  if (Type == kError) 
  {
    std::cerr << Message.CString() << "\n"; 
  }
  else // Type == kWarning or kInfo
  {
    std::cout << Message.CString() << "\n"; 
  }
}

// -------------------------------------------------------------------------------------------------

void TSystem::SetShowMessageFunc(TPShowMessageFunc Func)
{
  TSystemGlobals::SSystemGlobals()->mShowMessageFunc = Func;
}

// -------------------------------------------------------------------------------------------------

void TSystem::DisableShowMessageFunc()
{
  TSystemGlobals::SSystemGlobals()->mShowMessageFunc = StandardMessageRedirector;
}
