#include "CoreTypesPrecompiledHeader.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/NSString.h>

// =================================================================================================

// to let wrong use of the buffers crash as often as possible
#if defined(MDebug)
  #define MCStringBufferReallocation
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static CFStringEncoding SCFStringEncoding(TString::TCStringEncoding Encoding)
{
  switch (Encoding)
  {
  default:
    MInvalid("");

  case TString::kPlatformEncoding:
    return CFStringGetSystemEncoding();

  case TString::kUtf8:
  case TString::kFileSystemEncoding:
    return kCFStringEncodingUTF8;

  case TString::kAscii:
    return kCFStringEncodingASCII;
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
    // self made UTF8 conversion, because thats faster
    else if (Encoding == TString::kUtf8)
    {
      const char* pUtf8Chars = pCString;
      const char* pEnd = pUtf8Chars + ::strlen(pUtf8Chars);

      // worst case: this wastes memory
      mpStringBuffer = new TStringBuffer(NULL, (int)::strlen(pUtf8Chars) + 1);

      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      TUnicodeChar* pDestChars = mpStringBuffer->ReadWritePtr();

      while (pUtf8Chars < pEnd)
      {
        int c = *pUtf8Chars++;

        if (!(c & 0x80))
        {
          *pDestChars++ = (TUnicodeChar)(c);
        }
        else
        {
          TUnicodeChar Char = '?';

          if ((c & 0xE0) == 0xC0)
          {
            if (pUtf8Chars < pEnd)
            {
              Char = (TUnicodeChar)(((c & 0x1F) << 6) | (*pUtf8Chars++ & 0x3F));
            }
          }
          else if ((c & 0xF0) == 0xE0)
          {
            if (pEnd - pUtf8Chars >= 2)
            {
              Char = (TUnicodeChar)(((c & 0x0F) << 12) |
                ((((int)pUtf8Chars[0]) & 0x3F) << 6) |
                (pUtf8Chars[1] & 0x3F));
            }

            pUtf8Chars += 2;
          }
          else if ((c & 0xF8) == 0xF0)
          {
            pUtf8Chars += 3;
          }
          else if ((c & 0xFC) == 0xF8)
          {
            pUtf8Chars += 4;
          }
          else if ((c & 0xFE) == 0xFC)
          {
            pUtf8Chars += 5;
          }

          *pDestChars++ = Char;
        }
      }

      *pDestChars = '\0';
    }
    else
    {
      CFStringRef StrRef = NULL;

      const size_t StrLen = ::strlen(pCString);

      if (Encoding == TString::kFileSystemEncoding)
      {
        StrRef = ::CFStringCreateWithFileSystemRepresentation(NULL, pCString);

        if (!StrRef)
        {
          MInvalid("Failed to convert a filesystem string. Is the encoding correct?");

          StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)pCString,
            StrLen, ::SCFStringEncoding(Encoding), false);
        }
      }
      else
      {
        StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)pCString,
          StrLen, ::SCFStringEncoding(Encoding), false);
      }

      if (!StrRef)
      {
        MInvalid("Failed to convert a string. Is the encoding correct?");

        // try default encoding as last fallback
        StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)pCString,
          StrLen, ::CFStringGetSystemEncoding(), false);

        if (!StrRef)
        {
          operator =("???");
          return;
        }
      }

      const int NumDestChars = (int)::CFStringGetLength(StrRef) + 1;
      mpStringBuffer = new TStringBuffer(NULL, NumDestChars);

      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      ::CFStringGetCharacters(StrRef, ::CFRangeMake(0, NumDestChars - 1),
        mpStringBuffer->ReadWritePtr());

      mpStringBuffer->ReadWritePtr()[NumDestChars - 1] = '\0';

      ::CFRelease(StrRef);
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
    const int StrLen = Size();

    const CFStringRef StringRef = ::CFStringCreateWithCharactersNoCopy(
      NULL, Chars(), StrLen, kCFAllocatorNull);

    CFIndex FilledBytes;

    if (Encoding == TString::kFileSystemEncoding)
    {
      FilledBytes = ::CFStringGetMaximumSizeOfFileSystemRepresentation(StringRef);
    }
    else
    {
      ::CFStringGetBytes(StringRef, ::CFRangeMake(0, StrLen),
        ::SCFStringEncoding(Encoding), '?', false, NULL, 0, &FilledBytes);
    }

    Array.SetSize((int)FilledBytes);

    if (Encoding == TString::kFileSystemEncoding)
    {
      ::CFStringGetFileSystemRepresentation(StringRef, 
        Array.FirstWrite(), FilledBytes);
    }
    else
    {
      ::CFStringGetBytes(StringRef, ::CFRangeMake(0, StrLen),
        ::SCFStringEncoding(Encoding), '?', false, (UInt8*)Array.FirstWrite(),
        Array.Size(), &FilledBytes);
    }

    MAssert(FilledBytes > 0, "Conversion failed");
    ::CFRelease(StringRef);

    if (Encoding == TString::kFileSystemEncoding &&
        (size_t)Array.Size() != ::strlen(Array.FirstRead()))
    {
      // CFStringGetFileSystemRepresentation is a max, not an exact value,
      // so shrink the array if needed
      const TArray<char> Temp(Array);
      Array.SetSize((int)::strlen(Array.FirstRead())); // without the 0 byte
      TMemory::Copy(Array.FirstWrite(), Temp.FirstRead(), Array.Size());
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
  int StrLen = Array.Size();

  // older version of CFStringCreateWithBytes do not look for 0 bytes...
  for (int i = 0; i < Array.Size(); ++i)
  {
    if (Array[i] == '\0')
    {
      // null terminated. ignore the array size
      StrLen = i;
      break;
    }
  }

  if (StrLen)
  {
    CFStringRef StrRef = NULL;

    if (Encoding == TString::kFileSystemEncoding)
    {
      TArray<char> TempArray(Array.Size() + 1);
      TMemory::Copy(TempArray.FirstWrite(), Array.FirstRead(), Array.Size());
      TempArray[Array.Size()] = '\0';

      StrRef = ::CFStringCreateWithFileSystemRepresentation(
        NULL, TempArray.FirstRead());

      if (!StrRef)
      {
        MInvalid("Failed to convert a filesystem string. Is the encoding correct?");

        StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)Array.FirstRead(),
          StrLen, ::SCFStringEncoding(Encoding), false);
      }
    }
    else
    {
      StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)Array.FirstRead(),
        StrLen, ::SCFStringEncoding(Encoding), false);
    }

    if (!StrRef)
    {
      MInvalid("Failed to convert a string. Is the encoding correct?");

      // try default encoding as last fallback
      StrRef = ::CFStringCreateWithBytes(NULL, (UInt8*)Array.FirstRead(),
        StrLen, ::CFStringGetSystemEncoding(), false);

      if (!StrRef)
      {
        return operator =("???");
      }
    }

    const int NumDestChars = (int)::CFStringGetLength(StrRef) + 1;
    mpStringBuffer = new TStringBuffer(NULL, NumDestChars);

    if (mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    ::CFStringGetCharacters(StrRef, ::CFRangeMake(0, NumDestChars - 1),
      mpStringBuffer->ReadWritePtr());

    mpStringBuffer->ReadWritePtr()[NumDestChars - 1] = '\0';

    ::CFRelease(StrRef);
  }
  else
  {
    Empty();
  }

  return *this;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

CFStringRef gCreateCFString(const TString& String)
{
  return ::CFStringCreateWithCharacters(NULL, String.Chars(), String.Size());
}

// -------------------------------------------------------------------------------------------------

TString gCreateStringFromCFString(CFStringRef StringRef)
{
  if (CFIndex Length = (StringRef) ? (int)::CFStringGetLength(StringRef) : 0)
  {
    TArray<TUnicodeChar> StringBuffer((int)Length + 1);

    ::CFStringGetCharacters(StringRef, ::CFRangeMake(0, Length),
      StringBuffer.FirstWrite());

    StringBuffer[(int)Length] = '\0';

    return TString(StringBuffer.FirstRead());
  }
  else
  {
    return TString();
  }
}

// -------------------------------------------------------------------------------------------------

NSString* gCreateNSString(const TString& String)
{
  return [NSString stringWithCharacters:String.Chars() length:String.Size()];
}

// -------------------------------------------------------------------------------------------------

TString gCreateStringFromNSString(NSString* pNsString)
{
  return gCreateStringFromCFString((CFStringRef)pNsString);
}

