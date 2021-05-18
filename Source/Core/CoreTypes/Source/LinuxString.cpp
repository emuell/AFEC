#include "CoreTypesPrecompiledHeader.h"

#include <cerrno>
#include <cstring>
#include <clocale>

// =================================================================================================

// always realloc in debugbuilds to let wrong use crash as poften as possible 
#if defined(MDebug)
  #define MCStringBufferReallocation
#endif

// =================================================================================================

static TStaticArray<TArray<char>, TString::kNumCStringBuffers> sCCharArrays;
static int sLastCCharArray;
  
static bool sInitialized = false;

// -------------------------------------------------------------------------------------------------

const char* SIconvEncoding(TString::TCStringEncoding Encoding)
{
  switch(Encoding)
  {
    default:
      MInvalid("Unknown encoding");
      
    case TString::kPlatformEncoding:
      return "";
    
    case TString::kAscii:
      MInvalid("Should not be used with Iconv...");
      return "ASCII";
    
    case TString::kFileSystemEncoding:
    case TString::kUtf8:
      return "UTF-8";
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TString::SInitPlatformString()
{
  sLastCCharArray = 0;
  sInitialized = true;
}

// -------------------------------------------------------------------------------------------------

void TString::SExitPlatformString()
{
  sInitialized = false;
  
  for (int i = 0; i < sCCharArrays.Size(); ++i)
  {
    sCCharArrays[i].Empty();
  }
} 

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
    else if (Encoding == TString::kUtf8 ||
             ::strcmp("UTF-8", SIconvEncoding(Encoding)) == 0)
    {
      const size_t SourceLen = ::strlen(pCString);
      const char* const pUtf8CharsEnd = pCString + SourceLen;
      
      
      // . Calc Size
      
      const char* pUtf8Chars = pCString;
      
      int DestLen = 1; // for the '\0'
      
      while (pUtf8Chars < pUtf8CharsEnd)
      {
        const int c = *pUtf8Chars++;

        if (!(c & 0x80))
        {
          ++DestLen;
        }
        else
        {
          if ((c & 0xE0) == 0xC0)
          {
            pUtf8Chars += 1;
          }
          else if ((c & 0xF0) == 0xE0)
          {
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

          ++DestLen;
        }
      }

        
      // . Convert

      mpStringBuffer = new TStringBuffer(NULL, (int)DestLen);

      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      TUnicodeChar* pDestChars = mpStringBuffer->ReadWritePtr();
      pUtf8Chars = pCString;
       
      while (pUtf8Chars < pUtf8CharsEnd)
      {
        const int c = *pUtf8Chars++;

        if (!(c & 0x80))
        {
          *pDestChars++ = (TUnicodeChar)(c);
        }
        else
        {
          TUnicodeChar Char = '?';

          if ((c & 0xE0) == 0xC0)
          {
            if (pUtf8Chars < pUtf8CharsEnd)
            {
              Char = (TUnicodeChar)(((c & 0x1F) << 6) | 
                (*pUtf8Chars++ & 0x3F));
            }
          }
          else if ((c & 0xF0) == 0xE0)
          {
            if (pUtf8CharsEnd - pUtf8Chars >= 2)
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
      // make sure we do use the default locale from the system
      const TSystem::TSetAndRestoreLocale SetLocale(LC_CTYPE, "");
      
      // intermediate encoding to UTF8
      TIconv Converter(SIconvEncoding(Encoding), "UTF-8"); 
        
      const char* pSourceChars = pCString;
      size_t SourceLen = ::strlen(pCString);
    
      // avoid reallocations, so calc worst DestBuffer size
      size_t DestBufferSize = 1; 
        
      for (size_t i = 0; i < SourceLen; ++i)
      {
        if (pSourceChars[i] & 0x80)
        {
          // We may need up to 6 bytes for the utf8 output.
          DestBufferSize += 6; 
        }
        else
        {
          DestBufferSize += 1;
        }
      }
            
      TString TempString;
        
      while (SourceLen > 0)
      {
        TArray<char> DestBuffer(MMax(16, (int)DestBufferSize));
        
        char* pDestChars = DestBuffer.FirstWrite();
        size_t DestLen = DestBuffer.Size() - 1; // -1: for the '\0'
        
        if (Converter.Convert(&pSourceChars, &SourceLen, 
              &pDestChars, &DestLen) == size_t(-1))
        {
          switch (errno)
          {
          default:
            MInvalid("Unexpected iconv error!");
            SourceLen = 0;
            break;
            
          case E2BIG:
            MInvalid("Output buffer is too small!");
            // OK (handled), but should be avoided
            break;
  
          case EILSEQ:
            // Invalid multibyte sequence in the input
          case EINVAL:
            // Incomplete multibyte sequence in the input
            ++pSourceChars; *pDestChars++ = '?';
            --DestLen; --SourceLen; 
            break;
          }
        }
      
        *pDestChars = '\0';

        // this wont use Iconv. See above (Encoding == TString::kUtf8)...
        TempString += TString((const char*)DestBuffer.FirstRead(), TString::kUtf8);
      }
      
      *this = TempString;
    }
  }
}

// -------------------------------------------------------------------------------------------------

const char* TString::CString(TCStringEncoding Encoding)const
{
  MAssert(sInitialized, "Not yet or no longer initialized!");
  
  if (IsEmpty())
  {
    return "";
  }
  else 
  {
    TArray<char>& rCurrThreadsBuffer = sCCharArrays[sLastCCharArray];
    sLastCCharArray = (sLastCCharArray + 1) % kNumCStringBuffers;
  
    
    // ... kAscii
   
    if (Encoding == TString::kAscii)
    {
      const TUnicodeChar* pSrcChars = Chars();
      const int DestLength = Size() + 1;
      
      #if defined(MCStringBufferReallocation)
        rCurrThreadsBuffer.SetSize(DestLength);
      #else
        rCurrThreadsBuffer.Grow(DestLength);
      #endif
      
      char* pDestChars = rCurrThreadsBuffer.FirstWrite();
      
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
      
      *pDestChars = '\0';
      
      return rCurrThreadsBuffer.FirstRead();
    }
    
    
    // ... kUtf8
   
    else if (Encoding == TString::kUtf8 ||
             ::strcmp("UTF-8", SIconvEncoding(Encoding)) == 0)
    {
      const TUnicodeChar* pSrcChars = Chars();
      char* pDestChars = rCurrThreadsBuffer.FirstWrite();
      
      
      // . Calc dest length
      
      int DestLength = 1; // for the '\0'
      
      while (*pSrcChars != '\0')
      {
        if (*pSrcChars & 0xff80)
        {
          if (*pSrcChars & 0xf800)
          {
            DestLength += 2;
          }
          else
          {
            DestLength += 1;
          }
          
          ++DestLength;
        }
        else
        {
          ++DestLength;
        }
        
        ++pSrcChars;
      }
        
        
      // . Convert
      
      #if defined(MCStringBufferReallocation)
        rCurrThreadsBuffer.SetSize(DestLength);
      #else
        rCurrThreadsBuffer.Grow(DestLength);
      #endif
      
      pSrcChars = Chars();
      pDestChars = rCurrThreadsBuffer.FirstWrite();
      
      while (*pSrcChars != '\0')
      {
        if (*pSrcChars & 0xff80)
        {
          if (*pSrcChars & 0xf800)
          {
            *pDestChars++ = (char)(0xe0 | (*pSrcChars >> 12));
            *pDestChars++ = (char)(0x80 | ((*pSrcChars >> 6) & 0x3f));
          }
          else
          {
            *pDestChars++ = (char)(0xc0 | ((*pSrcChars >> 6) & 0x3f));
          }
          
          *pDestChars++ = (char)(0x80 | (*pSrcChars & 0x3f));
        }
        else
        {
          *pDestChars++ = (char)(*pSrcChars);
        }
        
        ++pSrcChars;
      }

      *pDestChars = '\0';
      
      return rCurrThreadsBuffer.FirstRead();
    }
   
    
    // ... kPlatformEncoding
   
    else
    {
      // make sure we do use the default locale from the system
      const TSystem::TSetAndRestoreLocale SetLocale(LC_CTYPE, "");

      // intermediate encoding from UTF8
      TIconv Converter("UTF-8", SIconvEncoding(Encoding));
        
      const char* pSourceChars = CString(TString::kUtf8);
      size_t SourceLen = ::strlen(pSourceChars);
    
      std::string TempString;
      
      while (SourceLen > 0)
      {
        // SourceLen*1: UTF-8s are already large, so assume dest is smaller ...
        TArray<char> DestBuffer(MMax(16, (int)SourceLen + 1));
        char* pDestChars = DestBuffer.FirstWrite();
      
        size_t DestLen = DestBuffer.Size() - 1; // -1: for the '\0'
        
        if (Converter.Convert(&pSourceChars, &SourceLen, 
              &pDestChars, &DestLen) == size_t(-1))
        {
          switch (errno)
          {
          default:
            MInvalid("Unexpected iconv error!");
            SourceLen = 0;
            break;
            
          case E2BIG:
            MInvalid("Output buffer is too small!");
            // OK (handled), but should be avoided
            break;
  
          case EILSEQ:
            // Invalid multibyte sequence in the input
          case EINVAL:
            // Incomplete multibyte sequence in the input
            ++pSourceChars; *pDestChars++ = '?';
            --DestLen; --SourceLen; 
            break;
          }
        }
          
        *pDestChars = '\0';
        TempString += DestBuffer.FirstRead();
      }
      
      #if defined(MCStringBufferReallocation)
        rCurrThreadsBuffer.SetSize((int)TempString.size() + 1); 
      #else
        rCurrThreadsBuffer.Grow((int)TempString.size() + 1);
      #endif
      
      TMemory::Copy(rCurrThreadsBuffer.FirstWrite(), TempString.data(), 
        TempString.size());
      
      rCurrThreadsBuffer.FirstWrite()[TempString.size()] = '\0';
        
      return rCurrThreadsBuffer.FirstWrite();
    }
  }
}

// -------------------------------------------------------------------------------------------------

void TString::CreateCStringArray(
  TArray<char>&     Array, 
  TCStringEncoding  Encoding)const
{
  if (IsEmpty())
  {
    Array.Empty();
  }
  else 
  {
    // ... kAscii
    
    if (Encoding == TString::kAscii)
    {
      const TUnicodeChar* pSrcChars = Chars();
      
      Array.SetSize(Size());
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
    
    
    // ... kUtf8
    
    else if (Encoding == TString::kUtf8 ||
             ::strcmp("UTF-8", SIconvEncoding(Encoding)) == 0)
    {
      const TUnicodeChar* pSrcChars = Chars();
      
      // . Calc dest length
      
      int DestLength = 0;
      
      while (*pSrcChars != '\0')
      {
        if (*pSrcChars & 0xff80)
        {
          if (*pSrcChars & 0xf800)
          {
            DestLength += 2;
          }
          else
          {
            DestLength += 1;
          }
          
          ++DestLength;
        }
        else
        {
          ++DestLength;
        }
        
        ++pSrcChars;
      }
        
        
      // . Convert
      
      Array.SetSize(DestLength);
      
      pSrcChars = Chars();
      char* pDestChars = Array.FirstWrite();
      
      while (*pSrcChars != '\0')
      {
        if (*pSrcChars & 0xff80)
        {
          if (*pSrcChars & 0xf800)
          {
            *pDestChars++ = (char)(0xe0 | (*pSrcChars >> 12));
            *pDestChars++ = (char)(0x80 | ((*pSrcChars >> 6) & 0x3f));
          }
          else
          {
            *pDestChars++ = (char)(0xc0 | ((*pSrcChars >> 6) & 0x3f));
          }
          
          *pDestChars++ = (char)(0x80 | (*pSrcChars & 0x3f));
        }
        else
        {
          *pDestChars++ = (char)(*pSrcChars);
        }
        
        ++pSrcChars;
      }
    }
    
    
    // ... kPlatformEncoding
   
    else
    {
      // make sure we do use the default locale from the system
      const TSystem::TSetAndRestoreLocale SetLocale(LC_CTYPE, "");

      // intermediate encoding from UTF8
      TIconv Converter("UTF-8", SIconvEncoding(Encoding));

      TArray<char> TempUtf8Buffer;
      CreateCStringArray(TempUtf8Buffer, TString::kUtf8);

      const char* pSourceChars = TempUtf8Buffer.FirstRead();
      size_t SourceLen = TempUtf8Buffer.Size();

      std::string TempString;

      while (SourceLen > 0)
      {
        // SourceLen*1: UTF-8s are already large, so assume dest is smaller ...
        TArray<char> DestBuffer(MMax(16, (int)SourceLen + 1));
        char* pDestChars = DestBuffer.FirstWrite();

        size_t DestLen = DestBuffer.Size() - 1; // -1: for the '\0'

        if (Converter.Convert(&pSourceChars, &SourceLen, 
              &pDestChars, &DestLen) == size_t(-1))
        {
          switch (errno)
          {
          default:
            MInvalid("Unexpected iconv error!");
            SourceLen = 0;
            break;

          case E2BIG:
            MInvalid("Output buffer is too small!");
            // OK (handled), but should be avoided
            break;

          case EILSEQ:
            // Invalid multibyte sequence in the input
          case EINVAL:
            // Incomplete multibyte sequence in the input
            ++pSourceChars; *pDestChars++ = '?';
            --DestLen; --SourceLen; 
            break;
          }
        }

        *pDestChars = '\0';
        TempString += DestBuffer.FirstRead();
      }

      Array.SetSize((int)TempString.size()); 
      TMemory::Copy(Array.FirstWrite(), TempString.data(), TempString.size());
    }
  }
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetFromCStringArray(
  const TArray<char>& Array, 
  TCStringEncoding    Encoding)
{
  const int StrLen = Array.Size();
  
  if (StrLen)
  {
    TString Ret;
    
    const char* pSourceChars = Array.FirstRead();
    size_t SourceLen = Array.Size();
  
    // avoid reallocations, so calc worst DestBuffer size
    size_t DestBufferSize = 1;
      
    for (size_t i = 0; i < SourceLen; ++i)
    {
      if (pSourceChars[i] == '\0')
      {
        // null terminated. ignore the array size.
        SourceLen = i; 
        break;
      }
      else if (pSourceChars[i] & 0x80)
      {
        // We may need up to 6 bytes for the utf8 output.
        DestBufferSize += 6; 
      }
      else
      {
        ++DestBufferSize;
      }
    }
    
    if (Encoding == TString::kAscii)
    {
      mpStringBuffer = new TStringBuffer(NULL, (int)SourceLen + 1);
        
      if (mpStringBuffer->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }
        
      TUnicodeChar* pDestChars = mpStringBuffer->ReadWritePtr();
            
      for (size_t i = 0; i < SourceLen; ++i)
      {
        pDestChars[i] = TUnicodeChar(pSourceChars[i]);
      }
      
      pDestChars[SourceLen] = '\0';  
    }
    else
    {
      // make sure we do use the default locale from the system
      const TSystem::TSetAndRestoreLocale SetLocale(LC_CTYPE, "");

      // intermediate encoding to UTF8
      TIconv Converter(SIconvEncoding(Encoding), "UTF-8");
      
      TString TempString;
        
      while (SourceLen > 0)
      {
        TArray<char> DestBuffer(MMax(16, (int)DestBufferSize)); 
        char* pDestChars = DestBuffer.FirstWrite();
      
        size_t DestLen = DestBuffer.Size() - 1;
        
        if (Converter.Convert(&pSourceChars, &SourceLen, 
              &pDestChars, &DestLen) == size_t(-1))
        {
          switch (errno)
          {
          default:
            MInvalid("Unexpected iconv error!");
            SourceLen = 0;
            break;
            
          case E2BIG:
            MInvalid("Output buffer is too small!");
            // OK (handled), but should be avoided
            break;

          case EILSEQ:
            // Invalid multibyte sequence in the input
          case EINVAL:
            // Incomplete multibyte sequence in the input
            ++pSourceChars; *pDestChars++ = '?';
            --DestLen; --SourceLen; 
            break;
          }
        }
          
        *pDestChars = '\0';
      
        // this wont use Iconv. See above (Encoding == TString::kUtf8)...
        TempString += TString(DestBuffer.FirstRead(), TString::kUtf8);  
      }
      
      *this = TempString;
    }
  }
  else
  {
    Empty();
  }
  
  return *this;
}
  
