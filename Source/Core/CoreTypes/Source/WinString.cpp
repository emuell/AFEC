#include "CoreTypesPrecompiledHeader.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// =================================================================================================

// always realloc in debugbuilds to let wrong use crash as poften as possible 
#if defined(MDebug)
  #define MCStringBufferReallocation
#endif

// =================================================================================================

namespace
{
  // -----------------------------------------------------------------------------------------------

  int CodePageFromEncoding(TString::TCStringEncoding Encoding)
  {
    switch (Encoding)
    {
    default:
      MInvalid("");
    
    case TString::kFileSystemEncoding:
    case TString::kPlatformEncoding:
      return CP_ACP;
  
    case TString::kUtf8:
      return CP_UTF8;
    }
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString::TString(const char* pCString, TCStringEncoding Encoding)
  : mpStringBuffer(spEmptyStringBuffer)
{
  if (pCString && *pCString != '\0')
  {
    if (Encoding == TString::kAscii)
    {
      // worst case: this wastes memory 
      mpStringBuffer = new TStringBuffer(NULL, (int)::strlen(pCString) + 1);
      
      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      const char* pSrcChars = pCString;
      TUnicodeChar* pDestChars = mpStringBuffer->ReadWritePtr();
      
      while (*pSrcChars != '\0')
      { 
        if (*pSrcChars >= 127)
        {
          *pDestChars++ = L'?';
          ++pSrcChars;
        }
        else
        {
          *pDestChars++ = *pSrcChars++;
        }
      }
      
      *pDestChars = '\0';
    }
    else
    {
      const int StrLen = ::MultiByteToWideChar(CodePageFromEncoding(Encoding), 
        0, pCString, -1, NULL, 0); // with the 0 byte!

      if (StrLen > 0)
      {
        mpStringBuffer = new TStringBuffer(NULL, StrLen);
        
        if (mpStringBuffer->AllocatedSize() == 0)
        {
          throw TOutOfMemoryException();
        }

        if (::MultiByteToWideChar(CodePageFromEncoding(Encoding), 0, pCString, -1,
             mpStringBuffer->ReadWritePtr(), StrLen) != -1)
        {
          mpStringBuffer->ReadWritePtr()[StrLen - 1] = '\0';
        }
        else
        {
          MInvalid("Conversion failed");
        }
      }
      else
      {
        MInvalid("Conversion failed");
      }
    }  
  }
}

// -------------------------------------------------------------------------------------------------

void TString::CreateCStringArray(
  TArray<char>&     Array, 
  TCStringEncoding  Encoding)const
{
  if (!IsEmpty())
  {
    if (Encoding == TString::kAscii)
    {
      const int Length = Size();
      Array.SetSize(Length);
      
      const TUnicodeChar* pSrcChars = Chars();
      char* pDestChars = Array.FirstWrite();
      
      while (*pSrcChars != '\0')
      {
        if (*pSrcChars >= 127)
        {
          *pDestChars++ = '?';
          ++pSrcChars;
        }
        else
        {
          *pDestChars++ = (char)*pSrcChars++;
        }
      }
    }
    else
    {
      const int StrLen = ::WideCharToMultiByte(CodePageFromEncoding(Encoding), 0, 
        mpStringBuffer->ReadPtr(), -1, NULL, 0, NULL, NULL );

      if (StrLen > 0)
      {
        Array.SetSize(StrLen - 1);
          
        if (::WideCharToMultiByte(CodePageFromEncoding(Encoding), 0, mpStringBuffer->ReadPtr(), 
              -1, Array.FirstWrite(), Array.Size(), NULL, NULL ) == -1)
        {
          MInvalid("Conversion failed");
          Array.SetSize(0);
        }
      }
      else
      {
        Array.Empty();
      }
    }
  }
  else
  {
    Array.Empty();
  }
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetFromCStringArray(
  const TArray<char>& Array, 
  TCStringEncoding    Encoding)
{
  if (Array.Size())
  {
    if (Encoding == kAscii)
    {
      mpStringBuffer = new TStringBuffer(NULL, Array.Size() + 1);
      
      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      const char* pSrcChars = Array.FirstRead();
      const char* pSrcCharsEnd = Array.FirstRead() + Array.Size();
      
      TUnicodeChar* pDestChars = mpStringBuffer->ReadWritePtr();
      
      while (pSrcChars < pSrcCharsEnd)
      { 
        if (*pSrcChars >= 127)
        {
          *pDestChars++ = L'?';
          ++pSrcChars;
        }
        else
        {
          *pDestChars++ = *pSrcChars++;
        }
      }
      
      *pDestChars = '\0';
    }
    else
    {
      const int StrLen = ::MultiByteToWideChar(CodePageFromEncoding(Encoding), 0, 
        Array.FirstRead(), Array.Size(), NULL, 0); // Array.Size(): without the 0 byte!

      if (StrLen > 0)
      {
        mpStringBuffer = new TStringBuffer(NULL, StrLen + 1);
        
        if (mpStringBuffer ->AllocatedSize() == 0)
        {
          throw TOutOfMemoryException();
        }

        if (::MultiByteToWideChar(CodePageFromEncoding(Encoding), 0, Array.FirstRead(), 
              Array.Size(), mpStringBuffer->ReadWritePtr(), StrLen) != -1) // Array.Size(): without the 0 byte!
        {
          mpStringBuffer->ReadWritePtr()[StrLen] = '\0';
        }
        else
        {
          MInvalid("Conversion failed");
          operator=("???");
        }
      }
      else
      {
        MInvalid("Conversion failed");
        operator=("???");
      }
    }
  }
  else
  {
    Empty();
  }
  
  return *this;
}
  
