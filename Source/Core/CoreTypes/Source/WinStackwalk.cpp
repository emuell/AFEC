#include "CoreTypesPrecompiledHeader.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>

#if defined (MArch_X64)
  // winnt.h doesn't declare this for us so we'll do it ourselves
  extern "C" NTSYSAPI VOID NTAPI RtlCaptureContext (__out PCONTEXT ContextRecord);
#endif

// imagehlp.h must be compiled with packing to eight-byte-boundaries,
// but does nothing to enforce that. I am grateful to Jeff Shanholtz
// <JShanholtz@premia.com> for finding this problem.
#pragma pack(push, 8)
#include <imagehlp.h>
#pragma pack(pop)

#pragma comment(lib, "imagehlp.lib")

// GET_CURRENT_CONTEXT: unreachable code
#pragma warning(disable: 4702)
// GET_CURRENT_CONTEXT: flow in or out of inline asm code suppresses global optimization
#pragma warning(disable: 4740)

//lint -e770, -e1709 (tag 'X' defined identically somewhere else)

// =================================================================================================

#define TTBUFLEN 65536 // for a temp buffer

// =================================================================================================

namespace TStackWalkImpl
{
  // check if initialized correctly
  bool IsInitialized();
    
  void Init();
  void Exit();

  // dump the the stack into the given List, and return successs
  void GetStack(
    HANDLE                  Threadhandle, 
    CONTEXT&                Context, 
    std::vector<uintptr_t>& FramePointers,
    int                     MaxFrames, 
    int                     StartAt); 
  
  void DumpStack(
    const std::vector<uintptr_t>& FramePointers,
    std::vector<std::string>&     Stack);

  void EnumAndLoadModuleSymbols(
    HANDLE          hProcess, 
    DWORD           pid);
    
  struct TModuleEntry
  {
    std::string imageName;
    std::string moduleName;
    LPVOID baseAddress;
    size_t size;
  };
    
  typedef std::vector<TModuleEntry> TModuleList;

  bool FillModuleListTH32(TModuleList& modules, DWORD pid);
  bool FillModuleListPSAPI(TModuleList& modules, DWORD pid, HANDLE hProcess);
  
  typedef BOOL (__stdcall *TSymGetLineFromAddrProc)(
    HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
  
  TSymGetLineFromAddrProc spSymGetLineFromAddr = NULL;

  bool gIsInitialized = false;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TStackWalk::Init()
{
  // nothing to do (TStackWalk::Init() is done lazy)
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::Exit()
{
  TStackWalkImpl::Exit();
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::GetStack(
  std::vector<uintptr_t>& FramePointers,
  int                     Depth,  
  int                     StartAt)
{
  #if defined (MArch_X86)
    // GetCurrentThreadContext wont work for the running thread...
    #define GET_CURRENT_CONTEXT(c, contextFlags) \
      do { \
      ::memset(&c, 0, sizeof(CONTEXT)); \
      c.ContextFlags = contextFlags; \
      __asm call x \
      __asm x: pop eax \
      __asm mov c.Eip, eax \
      __asm mov c.Ebp, ebp \
      __asm mov c.Esp, esp \
      } while(0); 

    CONTEXT ThreadContext;
    GET_CURRENT_CONTEXT(ThreadContext, CONTEXT_CONTROL);

  #elif defined (MArch_X64)
    CONTEXT ThreadContext;
    ::memset(&ThreadContext, 0, sizeof(CONTEXT));
    // This function should work on x64 but not on 32-bit Windows according to this article:
    // http://jpassing.com/2008/03/12/walking-the-stack-of-the-current-thread/
    // (requires Win XP or later)
    ThreadContext.ContextFlags = CONTEXT_CONTROL;
    ::RtlCaptureContext(&ThreadContext);

  #else
    #error Unknown architecture
  #endif

  TStackWalkImpl::GetStack(::GetCurrentThread(), 
    ThreadContext, FramePointers, Depth, StartAt);
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::DumpStack(
  const std::vector<uintptr_t>& FramePointers, 
  std::vector<std::string>&     Stack)
{
  if (! TStackWalkImpl::IsInitialized())
  {
    TStackWalkImpl::Init();
  }
 
  if (FramePointers.size())
  {
    TStackWalkImpl::DumpStack(FramePointers, Stack);
  } 
  else
  {
    Stack.clear();
    Stack.push_back("StackWalk failed");
  }
}

// -------------------------------------------------------------------------------------------------

void TStackWalk::DumpExceptionStack(EXCEPTION_POINTERS* ep)
{
  if (! TStackWalkImpl::IsInitialized())
  {
    TStackWalkImpl::Init();
  }
  
  HANDLE ThreadHandle;
  ::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(),
    ::GetCurrentProcess(), &ThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
  
  TLog::SLog()->AddLine("CrashLog", "Handling Exception! Code : %X", 
    (unsigned int)ep->ExceptionRecord->ExceptionCode);

  std::vector<uintptr_t> FramePointers;
  TStackWalkImpl::GetStack(ThreadHandle, *(ep->ContextRecord), FramePointers, -1, 0);
  
  std::vector<std::string> Stack;
  TStackWalkImpl::DumpStack(FramePointers, Stack);
  
  for (size_t i = 0; i < Stack.size(); ++i)
  {
    TLog::SLog()->AddLineNoVarArgs("CrashLog", Stack[i].c_str());
  }

  ::CloseHandle(ThreadHandle);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TStackWalkImpl::IsInitialized()
{
  return gIsInitialized;
}

// -------------------------------------------------------------------------------------------------

void TStackWalkImpl::Init()
{
  if (::SymInitialize(::GetCurrentProcess(), NULL, FALSE))
  {
    CHAR ApplicationFileName[1024] = { 0 };
    ::GetModuleFileNameA(NULL, ApplicationFileName, sizeof(ApplicationFileName));

    if (!::SymLoadModule(::GetCurrentProcess(), NULL, 
          ApplicationFileName, NULL, 0, 0))
    {
      MTrace2("SymLoadModule(\"%s\") failed (%d)", 
        ApplicationFileName, ::GetLastError());
    }

    if (! ::SymSetOptions(SYMOPT_UNDNAME + SYMOPT_LOAD_LINES))
    {
      MTrace1("SymSetOptions failed (%d)", ::GetLastError());
    }
  }
  else
  {
    MTrace1("SymInitialize failed (%d)", ::GetLastError());
  }

  HINSTANCE  ImageHlpHnd = ::LoadLibrary(L"ImageHlp.dll");

  if (ImageHlpHnd)
  {
    spSymGetLineFromAddr = (TSymGetLineFromAddrProc) 
      ::GetProcAddress(ImageHlpHnd, "SymGetLineFromAddr64");
  }

  gIsInitialized = true;
}

// -------------------------------------------------------------------------------------------------

void TStackWalkImpl::Exit()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

void TStackWalkImpl::GetStack(
  HANDLE                  ThreadHandle, 
  CONTEXT&                ThreadContext, 
  std::vector<uintptr_t>& FramePointers,
  int                     MaxFrames, 
  int                     StartFrame)
{
  STACKFRAME64 StackFrame;
  memset(&StackFrame, 0, sizeof(StackFrame));

  DWORD MachineType = 0;

  #if defined (MArch_X86)
    MachineType = IMAGE_FILE_MACHINE_I386;
    StackFrame.AddrPC.Offset    = ThreadContext.Eip;
    StackFrame.AddrPC.Mode      = AddrModeFlat;
    StackFrame.AddrStack.Offset = ThreadContext.Esp;
    StackFrame.AddrStack.Mode   = AddrModeFlat;
    StackFrame.AddrFrame.Offset = ThreadContext.Ebp;
    StackFrame.AddrFrame.Mode   = AddrModeFlat;

  #elif defined (MArch_X64)
    MachineType = IMAGE_FILE_MACHINE_AMD64;
    StackFrame.AddrPC.Offset    = ThreadContext.Rip;
    StackFrame.AddrPC.Mode      = AddrModeFlat;
    StackFrame.AddrStack.Offset = ThreadContext.Rsp;
    StackFrame.AddrStack.Mode   = AddrModeFlat;
    StackFrame.AddrFrame.Offset = ThreadContext.Rbp;  
    StackFrame.AddrFrame.Mode   = AddrModeFlat;

  #else
    #error Unknown architecture
  #endif

  FramePointers.clear();
  
  for (int i = 0; ; i++)
  {
    BOOL ok = ::StackWalk64(MachineType,
      GetCurrentProcess(),
      ThreadHandle,
      &StackFrame,
      (MachineType == IMAGE_FILE_MACHINE_I386) ? NULL : &ThreadContext,
      NULL,
      SymFunctionTableAccess64,
      SymGetModuleBase64,
      NULL);

    if (ok && StackFrame.AddrPC.Offset != 0)
    {
      if (i >= StartFrame)
      {
        FramePointers.push_back((uintptr_t)StackFrame.AddrPC.Offset);
        
        if (MaxFrames > 0 && i >= MaxFrames)
        {
          break;
        }
      }
    }
    else
    {
      break;
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TStackWalkImpl::DumpStack(
  const std::vector<uintptr_t>& FramePointers,
  std::vector<std::string>&     Stack)
{
  static bool sLoadedSymbols = false;

  if (! sLoadedSymbols)
  {
    EnumAndLoadModuleSymbols(::GetCurrentProcess(), ::GetCurrentProcessId());
    sLoadedSymbols = true;
  }

  Stack.clear();
  
  for (std::vector<uintptr_t>::const_iterator Iter = FramePointers.begin(); 
       Iter != FramePointers.end(); 
       ++Iter)
  {
    char ImageHelpBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_PATH];

    IMAGEHLP_SYMBOL64* pImageHelpSymbol = (IMAGEHLP_SYMBOL64*)ImageHelpBuffer;
    ::memset(ImageHelpBuffer, 0, sizeof(ImageHelpBuffer));
    pImageHelpSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    pImageHelpSymbol->MaxNameLength = MAX_PATH;

    DWORD64  Offset;
   
    if (! ::SymGetSymFromAddr64(GetCurrentProcess(), *Iter, &Offset, pImageHelpSymbol))
    {
      //lint -e419 Apparent data overrun for function exceeds argument 1
      ::strcpy(pImageHelpSymbol->Name, "???");
      Offset = 0;
    }

     IMAGEHLP_LINE64  ImageHelpLine;
    ::memset(&ImageHelpLine, 0, sizeof(ImageHelpLine));

    DWORD SymbolOffset;

    if (spSymGetLineFromAddr == NULL || 
        spSymGetLineFromAddr(GetCurrentProcess(), *Iter, &SymbolOffset, &ImageHelpLine) == 0)
    {
      ImageHelpLine.FileName = NULL;
      ImageHelpLine.LineNumber = 0;
    }

    const char* pFileName = ImageHelpLine.FileName;

    if (pFileName)
    {
      int NumSlashes = 0;

      pFileName += ::strlen(pFileName);
      
      while (pFileName > ImageHelpLine.FileName)
      {
        if (pFileName[-1] == '\\')
        {
          if (++NumSlashes == 3)
          {
            break;
          }
        }

        --pFileName;
      }

      char CharBuffer[4096];
      ::_snprintf(CharBuffer, sizeof(CharBuffer), "%p: %s +%05X (%s, line %u)",
        (void*)*Iter, pImageHelpSymbol->Name, (unsigned int)Offset, 
        pFileName, (unsigned int)ImageHelpLine.LineNumber);
      
      Stack.push_back(CharBuffer);
    }
    else
    {
      char CharBuffer[4096];
      ::_snprintf(CharBuffer, sizeof(CharBuffer), "%p: %s +%05X", 
        (void*)*Iter, pImageHelpSymbol->Name, (unsigned int)Offset);
      
      Stack.push_back(CharBuffer);
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TStackWalkImpl::EnumAndLoadModuleSymbols(
    HANDLE          hProcess, 
    DWORD           pid)
{
  // fill in module list
  TModuleList Modules;
  
  // try toolhelp32 first
  if (! FillModuleListTH32(Modules, pid))
  {
    // nope? try psapi, then
    FillModuleListPSAPI(Modules, pid, hProcess);
  }

  for (TModuleList::const_iterator Iter = Modules.begin(); Iter != Modules.end(); ++Iter)
  {
    // SymLoadModule() wants writable strings
    char* pImageName = new char[Iter->imageName.size() + 1];
    ::strcpy(pImageName, Iter->imageName.c_str());
    
    char* pModuleName = new char[Iter->moduleName.size() + 1];
    ::strcpy(pModuleName, Iter->moduleName.c_str());

    ::SymLoadModule64(hProcess, 0, pImageName, 
       pModuleName, (DWORD64)Iter->baseAddress, (DWORD)Iter->size);
    
    delete[] pImageName;
    delete[] pModuleName;
  }
}

// -------------------------------------------------------------------------------------------------

// miscellaneous toolhelp32 declarations; we cannot #include the header
// because not all systems may have it

#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008

#pragma pack(push, 8)
typedef struct tagMODULEENTRY32
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32;

typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;

#pragma pack(pop)

// -------------------------------------------------------------------------------------------------

bool TStackWalkImpl::FillModuleListTH32(TModuleList& Modules, DWORD pid)
{
  // CreateToolhelp32Snapshot()
  typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
  
  // Module32First()
  typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
  
  // Module32Next()
  typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

  // I think the DLL is called tlhelp32.dll on Win9X, so we try both
  const WCHAR* DllNames[] = {L"kernel32.dll", L"tlhelp32.dll"};
  
  HINSTANCE ToolHelpHandle = NULL;
  
  tCT32S pCT32S = NULL;
  tM32F pM32F = NULL;
  tM32N pM32N = NULL;

  for (size_t i = 0; i < MCountOf(DllNames); ++ i)
  {
    ToolHelpHandle = ::LoadLibrary(DllNames[i]);
    
    if (ToolHelpHandle == 0)
    {
      continue;
    }
    
    pCT32S = (tCT32S)::GetProcAddress(ToolHelpHandle, "CreateToolhelp32Snapshot");
    pM32F = (tM32F)::GetProcAddress(ToolHelpHandle, "Module32First");
    pM32N = (tM32N)::GetProcAddress(ToolHelpHandle, "Module32Next");
    
    if (pCT32S != 0 && pM32F != 0 && pM32N != 0)
    {
      // found the functions!
      break; 
    }

    ::FreeLibrary(ToolHelpHandle);
    ToolHelpHandle = 0;
  }

  // nothing found?
  if (ToolHelpHandle == NULL || pCT32S == NULL || pM32F == NULL || pM32N == NULL)
  {
    return false;
  }

  HANDLE SnapModuleHandle = pCT32S(TH32CS_SNAPMODULE, pid);
  
  if (SnapModuleHandle == (HANDLE)(uintptr_t)-1)
  {
    return false;
  }

  MODULEENTRY32 me = {0};
  me.dwSize = sizeof(me);

  bool KeepGoing = !!pM32F(SnapModuleHandle, &me);
  
  while (KeepGoing)
  {
    TModuleEntry e;
    e.imageName = me.szExePath;
    e.moduleName = me.szModule;
    e.baseAddress = me.modBaseAddr;
    e.size = me.modBaseSize;
    Modules.push_back(e);
    
    KeepGoing = !! pM32N(SnapModuleHandle, &me);
  }

  ::CloseHandle(SnapModuleHandle);
  ::FreeLibrary(ToolHelpHandle);

  return Modules.size() != 0;
}

// -------------------------------------------------------------------------------------------------

// miscellaneous psapi declarations; we cannot #include the header
// because not all systems may have it
typedef struct _MODULEINFO {
  LPVOID lpBaseOfDll;
  DWORD SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

// -------------------------------------------------------------------------------------------------

bool TStackWalkImpl::FillModuleListPSAPI(TModuleList& Modules, DWORD pid, HANDLE hProcess)
{
  // EnumProcessModules()
  typedef BOOL (__stdcall *tEPM)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
  
  // GetModuleFileNameEx()
  typedef DWORD (__stdcall *tGMFNE)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
  
  // GetModuleBaseName() -- redundant, as GMFNE() has the same prototype, but who cares?
  typedef DWORD (__stdcall *tGMBN)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
  
  // GetModuleInformation()
  typedef BOOL (__stdcall *tGMI)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize);

  HINSTANCE hPsapi = LoadLibrary(L"psapi.dll");
    
  if (hPsapi == 0)
  {
    return false;
  }

  Modules.clear();

  tEPM pEPM = (tEPM) ::GetProcAddress(hPsapi, "EnumProcessModules");
  tGMFNE pGMFNE = (tGMFNE) ::GetProcAddress(hPsapi, "GetModuleFileNameExA");
  tGMBN pGMBN = (tGMFNE) ::GetProcAddress(hPsapi, "GetModuleBaseNameA");
  tGMI pGMI = (tGMI) ::GetProcAddress(hPsapi, "GetModuleInformation");
    
  if (pEPM == 0 || pGMFNE == 0 || pGMBN == 0 || pGMI == 0)
  {
    // yuck. Some API is missing.
    ::FreeLibrary(hPsapi);
    return false;
  }

  DWORD cbNeeded = 0;
  
  HMODULE* hMods = new HMODULE[TTBUFLEN / sizeof(HMODULE)];
    
  unsigned i;
  
  char* tt = new char[TTBUFLEN];

  // not that this is a sample. Which means I can get away with
  // not checking for errors, but you cannot. :)
  if (!pEPM(hProcess, hMods, TTBUFLEN, &cbNeeded))
  {
    TLog::SLog()->AddLine("CrashLog", "EPM failed, GetLastError() = %lu", GetLastError());
    goto cleanup;
  }

  if (cbNeeded > TTBUFLEN)
  {
    TLog::SLog()->AddLine("CrashLog", "More than %lu module handles. Huh?", 
      TTBUFLEN / sizeof(HMODULE));
      
    goto cleanup;
  }

  for (i = 0; i < cbNeeded / sizeof hMods[0]; ++ i)
  {
    // for each module, get:
    // base address, size
    MODULEINFO mi;
    pGMI(hProcess, hMods[i], &mi, sizeof mi);

    TModuleEntry e;
    e.baseAddress = mi.lpBaseOfDll;
    e.size = mi.SizeOfImage;
    
    // image file name
    tt[0] = '\0';
    pGMFNE(hProcess, hMods[i], tt, TTBUFLEN);
    e.imageName = tt;
    
    // module name
    tt[0] = '\0';
    pGMBN(hProcess, hMods[i], tt, TTBUFLEN);
    e.moduleName = tt;
    
    Modules.push_back(e);
  }

cleanup:
  if (hPsapi)
  {
    ::FreeLibrary(hPsapi);
  }

  delete[] tt;
  delete[] hMods;

  return Modules.size() != 0;
}

