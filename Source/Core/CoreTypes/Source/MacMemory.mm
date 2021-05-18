#include "CoreTypesPrecompiledHeader.h"

#include <Foundation/Foundation.h>

// =================================================================================================

// when enabled, dump leaks found by the OSX "leaks" tool at exit
// #define MDumpPlatformLeaks

// =================================================================================================

// -------------------------------------------------------------------------------------------------

#if defined(MDebug) && defined(MDumpPlatformLeaks)

static bool SExecuteShellCommand(
  const NSString* pCommand, 
  const NSArray*  pArgs, 
  NSString**      pStdOut, 
  NSString**      pStdErr)
{
  NSTask* pTask = [[NSTask alloc] init];
  [pTask setLaunchPath:(NSString*)pCommand];
  [pTask setArguments:(NSArray*)pArgs];
  
  NSPipe* pStdOutPipe = [NSPipe pipe];
  NSFileHandle* pStdOutFileHandle = [pStdOutPipe fileHandleForReading];
  
  [pTask setStandardOutput: pStdOutPipe];
  
  NSPipe* pStdErrPipe = [NSPipe pipe];
  NSFileHandle* pStdErrFileHandle = [pStdErrPipe fileHandleForReading];
  
  [pTask setStandardError: pStdErrPipe];

  [pTask launch];
  
  // [pTask waitUntilExit] would poll the current run loop, which we want 
  // to avoid when being call on exit:
  while ([pTask isRunning])
  {
    TSystem::Sleep(100);
  }

  NSMutableData* pStdOutData = [NSMutableData data];
  
  while (NSData* pData = [pStdOutFileHandle availableData])
  {
    if ([pData length])
    {
      [pStdOutData appendData:pData];
    }
    else
    {
      break;
    }
  }

  *pStdOut = [[[NSString alloc] initWithData:pStdOutData 
    encoding:NSUTF8StringEncoding] autorelease];  
  
  [pStdOutFileHandle release];
  [pStdOutPipe release];
  [pStdOutData release];
  
  NSMutableData* pStdErrData = [NSMutableData data];
  
  while (NSData* pData = [pStdErrFileHandle availableData])
  {
    if ([pData length])
    {
      [pStdErrData appendData:pData];
    }
    else
    {
      break;
    }
  }
  
  *pStdErr = [[[NSString alloc] initWithData:pStdErrData 
    encoding:NSUTF8StringEncoding] autorelease];  
  
  [pStdErrFileHandle release];
  [pStdErrPipe release];
  [pStdErrData release];
  
  if ([pTask terminationStatus] == 0)
  {
    [pTask release];
    return true;
  }
  else
  {
    [pTask release];
    return false;  
  }
}

// -------------------------------------------------------------------------------------------------

static void SDumpLeaks()
{
  NSArray* pArgs = [[NSArray arrayWithObjects:
    [NSString stringWithFormat:@"%i", getpid()], 
    @"-nocontext", 
    nil] autorelease];
    
  NSString* pError = nil;
  NSString* pOut = nil;
  
  if (SExecuteShellCommand(@"/usr/bin/leaks", pArgs, &pOut, &pError))
  {
    gTraceVar("%s", [pOut cString]);
  }
}

#endif  // defined(MDebug) && defined(MDumpPlatformLeaks)

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TMemory::Init()
{
  #if defined(MDebug)
    ::atexit(TMemory::DumpLeaks);

    #if defined(MDumpPlatformLeaks)
      ::atexit(SDumpLeaks); 
    #endif
  #endif
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TMemory::TObjCAutoReleasePool::TObjCAutoReleasePool()
{
  mpPool = [[NSAutoreleasePool alloc] init];
}

// -------------------------------------------------------------------------------------------------

TMemory::TObjCAutoReleasePool::~TObjCAutoReleasePool()
{
  [(id)mpPool release];
}

