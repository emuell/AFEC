#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Memory.h"
#include "CoreTypes/Export/Version.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cwctype>  // towlower
#include <clocale>

#include <limits>
#include <utility>

#include "../../3rdParty/Iconv/Export/Iconv.h"

// =================================================================================================

MStaticAssert(sizeof(TUnicodeChar) == 2);

#if defined(MCompiler_GCC)
  // we can use wchar_t if this fires...
  MStaticAssert(sizeof(wchar_t) > sizeof(TUnicodeChar));
#else
  MStaticAssert(sizeof(wchar_t) == sizeof(TUnicodeChar));
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)

MStaticAssert(sizeof(wchar_t) == sizeof(TUnicodeChar));

// can use the w_char variants with Visual CPP
#define ucslen wcslen
#define ucscmp wcscmp
#define ucsicmp _wcsicmp

#else
  
// -------------------------------------------------------------------------------------------------

template<typename T>
static size_t ucslen(const T* pChars)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T) == sizeof(TUnicodeChar) || sizeof(T) == sizeof(wchar_t)));

  MAssert(pChars != NULL, "");
  
  int Count = 0;
  
  while(*pChars++ != '\0')
  {
    ++Count;
  }
  
  return Count;
}

// -------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
static int ucscmp(const T1* pFirst, const T2* pSecond)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T1) == sizeof(TUnicodeChar) || sizeof(T1) == sizeof(wchar_t)) &&
    (sizeof(T2) == sizeof(TUnicodeChar) || sizeof(T2) == sizeof(wchar_t)));

  MAssert(pFirst != NULL && pSecond != NULL, "");
  
  for (MEver)
  {
    if (*pFirst != *pSecond)
    {
      if (*pFirst == '\0')
      {
        return -1;
      }
      
      return *pFirst - *pSecond;
    }
    
    if (*pFirst == '\0')
    {
      MAssert(*pSecond == '\0', "");
      return 0;
    }
    
    ++pFirst;
    ++pSecond;
  }

  MInvalid("Should never be reached"); 
  return 0;
}

// -------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
static int ucsicmp(const T1* pFirst, const T2* pSecond)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T1) == sizeof(TUnicodeChar) || sizeof(T1) == sizeof(wchar_t)) &&
    (sizeof(T2) == sizeof(TUnicodeChar) || sizeof(T2) == sizeof(wchar_t)));

  MAssert(pFirst != NULL && pSecond != NULL, "");
  
  for (MEver)
  {
    const T1 First = (T1)::towupper(*pFirst);
    const T2 Second = (T2)::towupper(*pSecond);
    
    if (First != Second)
    {
      if (*pFirst == '\0')
      {
        return -1;
      }
      
      return First - Second;
    }
    
    if (*pFirst == '\0')
    {
      MAssert(*pSecond == '\0', "");
      return 0;
    }
    
    ++pFirst;
    ++pSecond;
  }

  MInvalid("Should never be reached"); 
  return 0;
}

#endif

// -------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
static int strnatcmp_right(T1 const *a, T2 const *b)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T1) == sizeof(TUnicodeChar) || sizeof(T1) == sizeof(wchar_t)) &&
    (sizeof(T2) == sizeof(TUnicodeChar) || sizeof(T2) == sizeof(wchar_t)));

  int bias = 0;
     
  // The longest run of digits wins. That aside, the greatest
  // value wins, but we can't know that it will until we've scanned
  // both numbers to know that they have the same magnitude, so we
  // remember it in BIAS.
  
  for (;; a++, b++) 
  {
    if (!::iswdigit(*a) && !::iswdigit(*b))
    {
      return bias;
    }
    else if (!::iswdigit(*a))
    {
      return -1;
    }
    else if (!::iswdigit(*b))
    {
      return +1;
    }
    else if (*a < *b) 
    {
      if (!bias)
      {
        bias = -1;
      }
    } 
    else if (*a > *b) 
    {
      if (!bias)
      {
        bias = +1;
      }
    } 
    else if (!*a && !*b)
    {
      return bias;
    }
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
static int strnatcmp_left(T1 const* a, T2 const* b)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T1) == sizeof(TUnicodeChar) || sizeof(T1) == sizeof(wchar_t)) &&
    (sizeof(T2) == sizeof(TUnicodeChar) || sizeof(T2) == sizeof(wchar_t)));

  // Compare two left-aligned numbers: the first to have a different value wins.
  
  for (;; a++, b++) 
  {
    if (!::iswdigit(*a) && !::iswdigit(*b))
    {
      return 0;
    }
    else if (!::iswdigit(*a))
    {
      return -1;
    }
    else if (!::iswdigit(*b))
    {
      return +1;
    }
    else if (*a < *b)
    {
      return -1;
    }
    else if (*a > *b)
    {
      return +1;
    }
  }
}

// -------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
static int strnatcmp(T1 const* a, T2 const* b, bool IgnoreCase)
{
  // only accept wchar_ts and TUnicodeChar as types
  MStaticAssert((sizeof(T1) == sizeof(TUnicodeChar) || sizeof(T1) == sizeof(wchar_t)) &&
    (sizeof(T2) == sizeof(TUnicodeChar) || sizeof(T2) == sizeof(wchar_t)));
  
  MAssert(a && b, "Invalid arguments!");
  
  int ai = 0;
  int bi = 0;
  
  for (MEver) 
  {
    T1 ca = a[ai]; 
    T2 cb = b[bi];

    // where one string is a prefix of another string,
    // the shorter one should always come first
    if (ca == 0 && cb != 0)
    {
      return -1;
    }
    else if (cb == 0 && ca != 0)
    {
      return +1;
    }

    // skip over leading spaces or zeros
    while (::iswspace(ca))
    {
      ca = a[++ai];
    }
    
    while (::iswspace(cb))
    {
      cb = b[++bi];
    }
    
    // process run of digits
    if (::iswdigit(ca) && ::iswdigit(cb)) 
    {
      const int CompFractional = (ca == '0' || cb == '0');

      int Result;
  
      if (CompFractional) 
      {
        if ((Result = ::strnatcmp_left(a + ai, b + bi)) != 0)
        {
          return Result;
        }
      } 
      else 
      {
        if ((Result = strnatcmp_right(a + ai, b + bi)) != 0)
        {
          return Result;
        }
      }
    }

    if (!ca && !cb) 
    {
      // The strings compare the same. Perhaps the caller
      // will want to call strcmp to break the tie.
      return 0;
    }

    if (IgnoreCase) 
    {
      ca = (T1)::towupper(ca);
      cb = (T2)::towupper(cb);
    }
    
    // sort "puncts" before all others...
    if (::iswpunct(ca) && !::iswpunct(cb))
    {
      return -1;
    }
    else if (!::iswpunct(ca) && ::iswpunct(cb))
    {
      return +1;
    }
    
    // then do an ascii < compares
    if (ca < cb)
    {
      return -1;
    }
    else if (ca > cb)
    {
      return +1;
    }

    ++ai; 
    ++bi;
  }
}

// -------------------------------------------------------------------------------------------------

template <typename T>
static TString SToStringFormat(const TString& Format, T Value)
{
  MAssert(Format.IsPureAscii(), "Expected plain ASCII formats");

  static const int sBufferSize = sizeof(T) * 64; // a rough but good guess

  #if defined(MCompiler_VisualCPP)
    // can use the wide char snwprintf version directly
    wchar_t Buffer[sBufferSize] = {0};
    ::snwprintf(Buffer, sBufferSize, Format.Chars(), Value);

    return TString(Buffer);

  #elif defined(MCompiler_GCC)
    // convert "Format" to a c string
    const int FormatSize = Format.Size();
    const TUnicodeChar* pFormatChars = Format.Chars();

    TAllocaArray<char> FormatCString(FormatSize + 1);
    MInitAllocaArray(FormatCString);
    
    for (int i = 0; i <= FormatSize; ++i, ++pFormatChars)
    {
      FormatCString[i] = (char)MMin(*pFormatChars, 0x7E); // clip to '~'
    }

    // then use sprintf
    char Buffer[sBufferSize] = {0};
    ::snprintf(Buffer, sBufferSize, FormatCString.FirstRead(), Value);

    return TString(Buffer, TString::kAscii);

  #else
    #error "Unknown compiler"

  #endif
}

// -------------------------------------------------------------------------------------------------

template <typename T>
static int SFromStringFormat(
  const TString& StringValue, const TString& Format, T* pValue)
{
  MAssert(Format.IsPureAscii(), "Expected plain ASCII formats");
  
  MAssert(StringValue.Size() < 1024 && Format.Size() < 1024,
    "Expected 'small' string and format buffers here");
  
  #if defined(MCompiler_VisualCPP)
    // can use swscanf directly
    return ::swscanf(StringValue.Chars(), Format.Chars(), pValue);

  #elif defined(MCompiler_GCC)
    // convert "String" to a c string
    const int StringSize = StringValue.Size();
    const TUnicodeChar* pStringChars = StringValue.Chars();

    TAllocaArray<char> StringCString(StringSize + 1);
    MInitAllocaArray(StringCString);
    for (int i = 0; i <= StringSize; ++i, ++pStringChars)
    {
      StringCString[i] = (char)MMin(*pStringChars, 0x7E); // clip to '~'
    }

    // convert "Format" to a c string
    const int FormatSize = Format.Size();
    const TUnicodeChar* pFormatChars = Format.Chars();

    TAllocaArray<char> FormatCString(FormatSize + 1);
    MInitAllocaArray(FormatCString);
    for (int i = 0; i <= FormatSize; ++i, ++pFormatChars)
    {
      FormatCString[i] = (char)MMin(*pFormatChars, 0x7E); // clip to '~'
    }

    // NB: swscanf should work in theory here too, but on OSX some multi-byte
    // "StringValue"s cause swscanf to fail due to a bug in Darwin.
    return ::sscanf(StringCString.FirstRead(), 
      FormatCString.FirstRead(), pValue);

  #else
    #error "Unknown compiler"

  #endif
}

// -------------------------------------------------------------------------------------------------

static bool SWildcardMatch(
  const TUnicodeChar* pString, 
  const TUnicodeChar* pPattern, 
  bool                IgnoreCase)
{
  const TUnicodeChar* p = pPattern;
  const TUnicodeChar* s = pString;
  
  for (; *p; s++, p++) 
  {
    int ok, ex; TUnicodeChar lc;
    
    switch (*p) 
    {
    // Literal match next char
    case '\\':                
        ++p;
        // FALLTHRU
    
    // Literal match char
    default:                  
      if ((IgnoreCase && (::towlower(*s) != ::towlower(*p))) || 
            (! IgnoreCase && (*s != *p)))
      {
        return false;
      }
      continue;
      
    // Match any char  
    case '?':                 
      if (*s == '\0')
      {
        return false;
      }
      continue;
        
    // Match any chars     
    case '*':                 
      if (*++p == '\0')      
      {
        return true;
      }
        
      for (; *s; s++)
      {
        if (SWildcardMatch(s, p, IgnoreCase))
        {
          return true;
        }    
      }
      return false;
    
    // Class
    case '[':                 
      ex = (p[1] == '^' || p[1] == '!');
      
      if (ex)
      {
        ++p;
      }
      
      if (IgnoreCase)
      {
        for (lc = 0400, ok = 0; *++p && *p != ']'; lc = *p)
        {
          if (*p == '-' ? ::towlower(*s) <= ::towlower(*++p) && ::towlower(*s) >= ::towlower(lc) : 
                ::towlower(*s) == ::towlower(*p))
          {
            ok = 1;
          }    
        }
      }
      else
      {
        for (lc = 0400, ok = 0; *++p && *p != ']'; lc = *p)
        {
          if (*p == '-' ? *s <= *++p && *s >= lc : *s == *p)
          {
            ok = 1;
          }    
        }
      }
        
      if (ok == ex)
      {
        return false;
      }
      
      continue;
    }
  }
  
  return (*s == '\0');
} 

// -------------------------------------------------------------------------------------------------

static bool SIsSpace(TUnicodeChar Char)
{
  return !! ::iswspace(Char);
}

// -------------------------------------------------------------------------------------------------

static bool SIsAlnum(TUnicodeChar Char)
{
  return ::iswalnum(Char) || (Char == '_'); 
}

// -------------------------------------------------------------------------------------------------

static bool SIsPaired(TUnicodeChar Char)
{
  switch(Char)
  {
  case '(':
  case ')':
  case '[':
  case ']':
  case '{':
  case '}':
  case '\'':
  case '\"':
    return true;
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

static bool SIsPunct(TUnicodeChar Char)
{
  return ::iswpunct(Char) && (Char != '_') && !SIsPaired(Char); 
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

const char* TIconv::SDefaultPlatformEncoding()
{
  #if defined(MMac)
    return "MACROMAN";
  #elif defined(MWindows)
    return  "LATIN1";
  #elif defined(MLinux)
    return "";
  #else
    #error unknown platform
  #endif
}

// -------------------------------------------------------------------------------------------------

TIconv::TIconv(const char* pFromEncoding, const char* pToEncoding)
{
  m_iconv = ::iconv_open(pToEncoding, pFromEncoding);
  
  if (m_iconv == (iconv_t)(uintptr_t)-1)
  {
    throw TReadableException(MText(
      "Unable to convert a string from encoding '%s' to '%s'",
      TString(pFromEncoding), TString(pToEncoding)));
  }
}

// -------------------------------------------------------------------------------------------------

TIconv::~TIconv()
{
  Close();
}

// -------------------------------------------------------------------------------------------------

bool TIconv::Open(const char* pFromEncoding, const char* pToEncoding)
{
  Close();
  m_iconv = ::iconv_open(pFromEncoding, pToEncoding);
  
  return (m_iconv != (iconv_t)(uintptr_t)-1);
}

// -------------------------------------------------------------------------------------------------

size_t TIconv::Convert(
  const char**  ppInputString, 
  size_t*       pInputBytesLeft,
  char**        pOutputString, 
  size_t*       pOutputBytesLeft)
{
  return ::iconv(m_iconv, ppInputString, pInputBytesLeft, 
    (char**)pOutputString, pOutputBytesLeft);
}

// -------------------------------------------------------------------------------------------------

bool TIconv::Close()
{
  bool ret = true;
  int err = errno;

  if (m_iconv != (iconv_t)(uintptr_t)-1)
  {
    if (::iconv_close(m_iconv) == -1)
    {
      ret = false;
    }
    
    m_iconv = (iconv_t)(uintptr_t)-1;
  }

  errno = err;
  return ret;
}

// =================================================================================================

namespace TStringConsts
{
  static bool sInitialized = false;
  
  static TString sMinus1; // "-1"
  static TString s0;      // "0"
  static TString s1;      // "1"
  
  static TString sYes;    // "YES"
  static TString sNo;     // "NO"
  static TString sTrue;   // "true"
  static TString sFalse;  // "false"
  
  static TString s0Float;       // "0.0"
  static TString s0Point5Float; // "0.5"
  static TString s1Float;       // "1.0" 

  static TString sDollarM;  // "$M" 
  static TString sPercentS; // "%s" 

  static TString sInf;  // "INF" 
  static TString sMinusInf; // "-INF" 
  static TString sNaN; // "NaN" 
}

// =================================================================================================

bool TDefaultStringFormatConsts::sInitialized = false;

TString TDefaultStringFormatConsts::sd;
TString TDefaultStringFormatConsts::sld;
TString TDefaultStringFormatConsts::slld;

TString TDefaultStringFormatConsts::su;
TString TDefaultStringFormatConsts::slu;
TString TDefaultStringFormatConsts::sllu;

TString TDefaultStringFormatConsts::sf;
TString TDefaultStringFormatConsts::s3f;
TString TDefaultStringFormatConsts::sg;
TString TDefaultStringFormatConsts::s9g;

TString TDefaultStringFormatConsts::slf;
TString TDefaultStringFormatConsts::s3lf;
TString TDefaultStringFormatConsts::slg;
TString TDefaultStringFormatConsts::s17lg;

// =================================================================================================

TPtr<TString::TEmptyStringBuffer> TString::spEmptyStringBuffer;

// -------------------------------------------------------------------------------------------------

void TString::SInit()
{
  spEmptyStringBuffer = new TEmptyStringBuffer();

  SInitPlatformString();
  
  TStringConsts::sInitialized = true;
  
  TStringConsts::sMinus1 = "-1";
  TStringConsts::s0 = "0";
  TStringConsts::s1 = "1";
  
  TStringConsts::sYes = "YES";
  TStringConsts::sNo = "NO";
  TStringConsts::sTrue = "true";
  TStringConsts::sFalse = "false";
  
  TStringConsts::s0Float = "0.0";
  TStringConsts::s0Point5Float = "0.5";
  TStringConsts::s1Float = "1.0";
  
  TStringConsts::sDollarM = "$M"; 
  TStringConsts::sPercentS = "%s";

  TStringConsts::sInf = "INF";
  TStringConsts::sMinusInf = "-INF"; 
  TStringConsts::sNaN = "NaN";

  TDefaultStringFormatConsts::sInitialized = true;

  TDefaultStringFormatConsts::sd = "%d";
  TDefaultStringFormatConsts::sld = "%ld";
  TDefaultStringFormatConsts::slld = "%lld";

  TDefaultStringFormatConsts::su = "%u";
  TDefaultStringFormatConsts::slu = "%lu";
  TDefaultStringFormatConsts::sllu = "%llu";

  TDefaultStringFormatConsts::sf = "%f";
  TDefaultStringFormatConsts::s3f = "%.3f";
  TDefaultStringFormatConsts::sg = "%g";
  TDefaultStringFormatConsts::s9g = "%.9g";
  
  TDefaultStringFormatConsts::slf = "%lf";
  TDefaultStringFormatConsts::s3lf = "%.3lf";
  TDefaultStringFormatConsts::slg = "%lg";
  TDefaultStringFormatConsts::s17lg = "%.17lg";
}

// -------------------------------------------------------------------------------------------------

void TString::SExit()
{
  TStringConsts::sInitialized = false;

  TStringConsts::sMinus1.Empty();
  TStringConsts::s0.Empty();
  TStringConsts::s1.Empty();
  
  TStringConsts::sYes.Empty();
  TStringConsts::sNo.Empty();
  TStringConsts::sTrue.Empty();
  TStringConsts::sFalse.Empty();

  TStringConsts::s0Float.Empty();
  TStringConsts::s0Point5Float.Empty();
  TStringConsts::s1Float.Empty();
  
  TStringConsts::sDollarM.Empty();
  TStringConsts::sPercentS.Empty();
  
  TStringConsts::sInf.Empty();
  TStringConsts::sMinusInf.Empty(); 
  TStringConsts::sNaN.Empty();

  TDefaultStringFormatConsts::sInitialized = false;

  TDefaultStringFormatConsts::sd.Empty();
  TDefaultStringFormatConsts::sld.Empty();
  TDefaultStringFormatConsts::slld.Empty();

  TDefaultStringFormatConsts::su.Empty();
  TDefaultStringFormatConsts::slu.Empty();
  TDefaultStringFormatConsts::sllu.Empty();

  TDefaultStringFormatConsts::sf.Empty();
  TDefaultStringFormatConsts::s3f.Empty();
  TDefaultStringFormatConsts::sg.Empty();
  TDefaultStringFormatConsts::s9g.Empty();

  TDefaultStringFormatConsts::slf.Empty();
  TDefaultStringFormatConsts::s3lf.Empty();
  TDefaultStringFormatConsts::slg.Empty();
  TDefaultStringFormatConsts::s17lg.Empty();

  SExitPlatformString();

  spEmptyStringBuffer.Delete();
}

// -------------------------------------------------------------------------------------------------

bool TString::SIsSeperatorChar(TUnicodeChar Char)
{
  return ::iswspace(Char) ||
    Char == '(' || Char == ')' ||
    Char == '{' || Char == '}' ||
    Char == '[' || Char == ']' ||
    Char == '<' || Char == '>' ||
    Char == ';' || Char == ':' ||
    Char == '/' || Char == '\\' || Char == '|' ||
    Char == ',' || Char == '.' ||
    Char == '\"' || Char == '\'' ||
    Char == '@';
}

// -------------------------------------------------------------------------------------------------

TString TString::SNewLine()
{
  #if defined(MWindows)
    return "\x0D\x0A";
  
  #elif defined(MMac) || defined(MLinux)
    return "\n";
  
  #else
    #error "Unknown Platform"
  #endif
}

// -------------------------------------------------------------------------------------------------

TString::TString()
  : mpStringBuffer(spEmptyStringBuffer)
{
}

TString::TString(const char* pChars)
  : mpStringBuffer(spEmptyStringBuffer)
{
  const int Len = pChars ? (int)::strlen(pChars) : 0;
  
  if (Len > 0)
  {
    mpStringBuffer = new TStringBuffer(NULL, Len + 1);

    if (mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    TUnicodeChar* pDest = mpStringBuffer->ReadWritePtr();
    
    for (int i = 0; i <= Len; ++i)
    {
      pDest[i] = pChars[i];
    }  
    
    MAssert(IsPureAscii(), "Please construct by specifying a proper encoding!");
  }  
}

TString::TString(const TString& Other)
  : mpStringBuffer(Other.mpStringBuffer)
{
}

#if defined(MCompiler_Has_RValue_References)

TString::TString(TString&& Other)
  : mpStringBuffer(std::move(Other.mpStringBuffer))
{
}

#endif

TString::TString(const TUnicodeChar *pChars)
  : mpStringBuffer(spEmptyStringBuffer)
{
  operator=(pChars);
}

TString::TString( const TUnicodeChar* pBegin, const TUnicodeChar* pEnd )
  : mpStringBuffer( spEmptyStringBuffer )
{
  const int Len = (int)(intptr_t)(pEnd - pBegin);

  if ( Len > 0 )
  {
    mpStringBuffer = new TStringBuffer( NULL, Len + 1 );

    if ( mpStringBuffer->AllocatedSize() == 0 )
    {
      throw TOutOfMemoryException();
    }

    TMemory::Copy( mpStringBuffer->ReadWritePtr(), pBegin,
      (int)(sizeof( TUnicodeChar ) * Len) );

    mpStringBuffer->ReadWritePtr()[Len] = '\0';
  }
}

#if defined(MCompiler_GCC)

TString::TString(const wchar_t* pWString)
  : mpStringBuffer(spEmptyStringBuffer)
{
  operator=(pWString);
}

TString& TString::operator=(const wchar_t* pWideChars)
{  
  const int StrLen = (int)::wcslen(pWideChars);
  
  if (StrLen)
  {
    const int AllocatedSize = StrLen * 2 + 1; // worst case
    mpStringBuffer = new TStringBuffer(NULL, AllocatedSize);
    
    if (mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    TUnicodeChar* pDest = mpStringBuffer->ReadWritePtr();
    
    for (int i = 0; i < StrLen; ++i) 
    {
      unsigned int WChar = pWideChars[i];
      
      if (WChar > 0xffff) 
      {
        // decompose into a surrogate pair
        WChar -= 0x10000;
        
        *pDest++ = TUnicodeChar(WChar / 0x400 + 0xd800);
        
        WChar = (WChar % 0x400) + 0xdc00;
      }
      
      *pDest++ = TUnicodeChar(WChar);
    }
    
    *pDest = '\0';
  }
  else
  {
    Empty();
  }
  
  return *this;
}  
        
#endif // MCompiler_GCC

TString& TString::operator= (const TUnicodeChar *pChars)
{
  // do not allocate an empty string
  if (pChars && pChars[0] != '\0')
  {
    mpStringBuffer = new TStringBuffer(pChars, -1);

    if (mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }
  }
  else
  {
    Empty();
  }
  
  return *this;
}

TString& TString::operator= (const TString& Other)
{
  if (this != &Other)
  {  
    // share the buffer
    mpStringBuffer = Other.mpStringBuffer;
  }

  return *this;
}

#if defined(MCompiler_Has_RValue_References)

TString& TString::operator=(TString&& Other)
{
  if (mpStringBuffer != Other.mpStringBuffer)
  {
    // move buffer from other to this
    mpStringBuffer = std::move(Other.mpStringBuffer);
  }

  return *this;
}

#endif

// -------------------------------------------------------------------------------------------------

bool TString::IsPureAscii()const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int StrLen = Size();
  
  for (int i = 0; i < StrLen; ++i)
  {
    if (pChars[i] >= 127)
    {
      return false;
    }
  }
  
  return true;
}

// -------------------------------------------------------------------------------------------------

void TString::Empty()
{
  mpStringBuffer = spEmptyStringBuffer;
}

// -------------------------------------------------------------------------------------------------

void TString::Clear()
{
  if (mpStringBuffer != spEmptyStringBuffer)
  {
    MakeStringBufferUnique();
    mpStringBuffer->ReadWritePtr()[0] = '\0';
  }
}
  
// -------------------------------------------------------------------------------------------------

int TString::PreallocatedSpace()const
{
  return mpStringBuffer->AllocatedSize();
}

// -------------------------------------------------------------------------------------------------

void TString::PreallocateSpace(int NumChars)
{
  MAssert(mpStringBuffer == spEmptyStringBuffer, 
    "This will destroy the current buffer!");

  MAssert(NumChars > 0, "Nothing to preallocate");
  
  mpStringBuffer = new TStringBuffer(NULL, NumChars);

  if (NumChars > 0 && mpStringBuffer->AllocatedSize() == 0)
  {
    throw TOutOfMemoryException();
  }
}  

// -------------------------------------------------------------------------------------------------

int TString::Size()const
{
  return (int)::ucslen(mpStringBuffer->ReadPtr());
}

// -------------------------------------------------------------------------------------------------

TString TString::SubString(int CharIndexStart, int CharIndexEnd) const
{
  const int Len = Size();

  // should be an assertion ?
  if (CharIndexStart > Len || IsEmpty())
  {
    return TString();
  }

  if (CharIndexStart < 0)
  {
    CharIndexStart = 0;
  }

  if (CharIndexEnd < 0 || CharIndexEnd > Len)
  {
    CharIndexEnd = Len;
  }

  if (CharIndexStart == 0 && CharIndexEnd == Len)
  {
    return *this;
  }

  MAssert(CharIndexEnd >= CharIndexStart, "");

  if (CharIndexEnd > CharIndexStart)
  {
    const int StrLen = CharIndexEnd - CharIndexStart + 1;
    TPtr<TStringBuffer> pTemp = new TStringBuffer(NULL, StrLen);
    
    if (pTemp->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    TMemory::Copy(pTemp->ReadWritePtr(), mpStringBuffer->ReadPtr() + CharIndexStart,
      (StrLen - 1) * sizeof(TUnicodeChar));
      
    pTemp->ReadWritePtr()[StrLen - 1] = '\0';

    TString Ret;
    Ret.mpStringBuffer = pTemp;
    
    return Ret;
  }
  else
  {
    // CharIndexEnd == CharIndexStart
    return TString();
  }
}

// -------------------------------------------------------------------------------------------------

int TString::NumberOfParagraphs()const
{
  TList<TString> Ret;
   
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
   
  int i = 0, LastFoundIndex = 0;
  
  int Count = 0;
  
  while (i < Len)
  {
    bool FoundNewLine = false;
    
    // Microsoft Windows / MS-DOS                           0Dh 0Ah
    // Apple Macintosh OS 9 and earlier                   0Dh
    // Unix (e.g., Linux), also Apple OS X and higher     0Ah

    if (pChars[i] == 0x0D)      // mac or win
    {
      if (i + 1 < Len && pChars[i + 1] == 0x0A)  // win
      {
        i++;
      }
      
      FoundNewLine = true;
    }
    else if (pChars[i] == 0x0A) // unix
    {
      FoundNewLine = true;
    }

    // create  a new paragraph 
    if (FoundNewLine)
    {
      ++Count;
      LastFoundIndex = i + 1;
    }

    ++i;
  }

  // don't miss the last paragraph
  if (i >= LastFoundIndex)
  {
    ++Count;
  }
  
  return Count;
}

// -------------------------------------------------------------------------------------------------

TList<TString> TString::Paragraphs()const
{
  TList<TString> Ret;
   
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
   
  int i = 0, LastFoundIndex = 0;

  while (i < Len)
  {
    const int j = i;

    bool FoundNewLine = false;
    
    if (pChars[i] == 0x0D)      // mac or win
    {
      if (i + 1 < Len && pChars[i + 1] == 0x0A)  // win
      {
        i++;
      }
      
      FoundNewLine = true;
    }
    else if (pChars[i] == 0x0A) // unix
    {
      FoundNewLine = true;
    }

    // create  a new paragraph 
    if (FoundNewLine)
    {
      Ret.Append(SubString(LastFoundIndex, j));
      
      LastFoundIndex = i + 1;
    }

    ++i;
  }

  // don't miss the last paragraph
  if (i >= LastFoundIndex)
  {
    Ret.Append(SubString(LastFoundIndex, i));
  }
  
  // make sure NumberOfParagraphs does the same thing
  MAssert(NumberOfParagraphs() == Ret.Size(), "");
  
  return Ret;
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetFromParagraphs(
  const TList<TString>& Paragraphs,
  const TString&        Separator)
{
  // avoid querying String.Size() more than once
  const int SeparatorSize = Separator.Size();
  TArray<int> ParagraphSizes(Paragraphs.Size());
  
  // find out total needed size
  int NeededSize = 1;
  for (int i = 0; i < Paragraphs.Size(); ++i)
  {
    ParagraphSizes[i] = Paragraphs[i].Size();
    NeededSize += ParagraphSizes[i];

    if (i < Paragraphs.Size() - 1)
    {
      NeededSize += SeparatorSize;
    }
  }

  if (NeededSize == 1)
  {
    // result is empty
    Empty();

    return *this;
  }
  else
  {
    // allocate new string buffer
    TPtr<TStringBuffer> pTmp(new TStringBuffer(NULL, NeededSize));

    if (pTmp->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    // combine paragraphs with separators into new string buffer
    TUnicodeChar* pWritePtr = pTmp->ReadWritePtr();

    for (int i = 0; i < Paragraphs.Size(); ++i)
    {
      TMemory::Copy(pWritePtr, Paragraphs[i].Chars(), 
        ParagraphSizes[i] * sizeof(TUnicodeChar));

      pWritePtr += ParagraphSizes[i];

      if (SeparatorSize && i < Paragraphs.Size() - 1)
      {
        TMemory::Copy(pWritePtr, Separator.Chars(), 
          SeparatorSize * sizeof(TUnicodeChar));

        pWritePtr += SeparatorSize;
      }
    }

    // terminate new buffer
    *pWritePtr = '\0';

    // assign new buffer
    mpStringBuffer = pTmp;

    return *this;
  }
}

// -------------------------------------------------------------------------------------------------

TList<TString> TString::SplitAt(const TUnicodeChar Char)const
{
  return SplitAt(MakeList<TUnicodeChar>(Char));
}

TList<TString> TString::SplitAt(const TList<TUnicodeChar>& SplitChars)const
{
  TList<TString> Ret;
  
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();

  int i = 0, k = 0;

  while (i < Len)
  {
    bool Match = false;

    for (int c = 0; c < SplitChars.Size(); ++c)
    {
      if (pChars[i] == SplitChars[c])
      {
        Match = true;
        break;
      }
    }
    
    // create a new entry 
    if (Match)
    {
      Ret.Append(SubString(k, i));
      
      k = i + 1;
    }

    ++i;
  }

  // don't miss the rest
  if (i >= k)
  {
    Ret.Append(SubString(k, i));
  }
  
  return Ret;
}

TList<TString> TString::SplitAt(const TString& SplitString)const
{
  return SplitAt(MakeList<TString>(SplitString));
}

TList<TString> TString::SplitAt(const TList<TString>& SplitStrings)const
{
  TList<TString> Ret;
  
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();

  int i = 0, k = 0;

  while (i < Len)
  {
    bool AnyStringMatch = false;
    int CurrentSplitLen = 0;

    for (int s = 0; s < SplitStrings.Size(); ++s)
    {
      const TUnicodeChar* pSplitChars = SplitStrings[s].Chars();
      const int SplitLen = SplitStrings[s].Size();
  
      bool CurStringMatch = true;

      for (int c = 0; c < SplitLen; ++c)
      {
        if (c + i >= Len || pChars[i + c] != pSplitChars[c])
        {
          CurStringMatch = false;
          break;
        }
      }

      if (CurStringMatch)
      {
        AnyStringMatch = true;
        CurrentSplitLen = SplitLen;
        break;
      }
    }

    // create a new entry ?
    if (AnyStringMatch)
    {
      Ret.Append(SubString(k, i));

      k = i + CurrentSplitLen;
      
      if (CurrentSplitLen)
      {
        i += CurrentSplitLen - 1;
      }
    }
    
    ++i;
  }

  // don't miss the rest
  if (i >= k)
  {
    Ret.Append(SubString(k, i));
  }
  
  return Ret;
}
  
// -------------------------------------------------------------------------------------------------

int TString::Find(
  const TString&  ContentToFind, 
  int             StartOffSet, 
  bool            Backwards) const
{
  if (ContentToFind.IsEmpty() || IsEmpty())
  {
    return -1;
  }

  const int Len = Size();

  if (StartOffSet > Len || StartOffSet < 0)
  {
    // should maybe assert this?
    return -1;
  }

  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  
  const int EndPos = Backwards ? -1 : Len;
  const int Step = Backwards ? -1 : 1;

  MAssert((!Backwards && EndPos >= StartOffSet) || 
    (Backwards && EndPos <= StartOffSet), "");

  for (int Offset = StartOffSet; Offset != EndPos; Offset += Step)
  {
    bool Matches = true;
    
    for (int c = 0; c < ContentToFind.Size(); ++c)
    {
      if (pChars[Offset + c] != ContentToFind[c])
      {
        Matches = false;
        break;
      }
    }
    
    if (Matches)
    {
      return Offset;
    }
  }
  
  return -1;
}

int TString::Find(
  const TUnicodeChar  CharToFind, 
  int                 StartOffSet, 
  bool                Backwards) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();

  if (StartOffSet > Len || StartOffSet < 0)
  {
    // should maybe assert this?
    return -1;
  }

  const int EndPos = Backwards ? -1 : Len;
  const int Step = Backwards ? -1 : 1;
             
  MAssert((!Backwards && EndPos >= StartOffSet) || 
    (Backwards && EndPos <= StartOffSet), "");

  for (int Offset = StartOffSet; Offset != EndPos; Offset += Step)
  {
    if (pChars[Offset] == CharToFind)
    {
      return Offset;
    }
  }
  
  return -1;
}

// -------------------------------------------------------------------------------------------------

int TString::FindIgnoreCase(
  const TString&  ContentToFind, 
  int             StartOffSet, 
  bool            Backwards) const
{
  if (ContentToFind.IsEmpty() || IsEmpty())
  {
    return -1;
  }

  const int Len = Size();

  if (StartOffSet >= Len || StartOffSet < 0)
  {
    // should maybe assert this?
    return -1;
  }

  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  
  const int EndPos = Backwards ? -1 : Len;
  const int Step = Backwards ? -1 : 1;

  MAssert((!Backwards && EndPos >= StartOffSet) || 
    (Backwards && EndPos <= StartOffSet), "");

  for (int Offset = StartOffSet; Offset != EndPos; Offset += Step)
  {
    bool Matches = true;
    
    for (int c = 0; c < ContentToFind.Size(); ++c)
    {
      if (::towlower(pChars[Offset + c]) != ::towlower(ContentToFind[c]))
      {
        Matches = false;
        break;
      }
    }
    
    if (Matches)
    {
      return Offset;
    }
  }
  
  return -1;
}

int TString::FindIgnoreCase(
  const TUnicodeChar  CharToFind, 
  int                 StartOffSet, 
  bool                Backwards) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();

  const int EndPos = Backwards ? -1 : Len;
  const int Step = Backwards ? -1 : 1;
  
  const int ToLowerCharToFind = ::towlower(CharToFind);
             
  MAssert((!Backwards && EndPos >= StartOffSet) || 
    (Backwards && EndPos <= StartOffSet), "");

  for (int Offset = StartOffSet; Offset != EndPos; Offset += Step)
  {
    if ((signed)::towlower(pChars[Offset]) == ToLowerCharToFind)
    {
      return Offset;
    }
  }
  
  return -1;
}

// -------------------------------------------------------------------------------------------------

int TString::FindNextWordStart(int StartOffSet, bool Backwards) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
   
  if (Backwards)
  {
    if (--StartOffSet <= 0)
    {
      return 0;
    }

    // first skip whitespace
    while (StartOffSet > 0 && ::iswspace(pChars[StartOffSet]))
    {
      --StartOffSet;
    }

    // find the next whitespace
    for (int Offset = StartOffSet; Offset > 0; Offset--)
    {
      if (::iswspace(pChars[Offset]))
      {
        return Offset + 1;
      }
    }
    
    return 0;
  }

  // (!Backwards)
  else 
  {
    // skip (_not_ whitespace)
    if (StartOffSet >= Len)
    {
      return Len;
    }

    while (StartOffSet < Len && ! ::iswspace(pChars[StartOffSet]))
    {
      ++StartOffSet;
    }

    // find the next (_not_ whitespace)
    for (int Offset = StartOffSet; Offset < Len; Offset ++)
    {
      if (! ::iswspace(pChars[Offset]))
      {
        return Offset;
      }
    }

    return Len;
  }
}

// -------------------------------------------------------------------------------------------------

int TString::FindNextWordOrSeparatorStart(int StartOffSet, bool Backwards) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
   
  if (Backwards)
  {
    if (StartOffSet <= 1)
    {
      return 0;
    }

    if (::SIsPunct(pChars[StartOffSet - 1])) // punct
    {
      while (StartOffSet > 0 && ::SIsPunct(pChars[StartOffSet - 1]))
      {
        --StartOffSet;
      }

      while (StartOffSet > 0 && ::SIsSpace(pChars[StartOffSet - 1]))
      {
        --StartOffSet;
      }
    }
    else if (::SIsAlnum(pChars[StartOffSet - 1])) // alnums
    {
      while (StartOffSet > 0 && ::SIsAlnum(pChars[StartOffSet - 1]))
      {
        --StartOffSet;
      }
    }
    else if (::SIsPaired(pChars[StartOffSet - 1])) // braces
    {
      --StartOffSet;
    }
    else // space
    {
      while (StartOffSet > 0 && ::SIsSpace(pChars[StartOffSet - 1]))
      {
        --StartOffSet;
      }

      if (::SIsAlnum(pChars[StartOffSet - 1]))
      {
        while (StartOffSet > 0 && ::SIsAlnum(pChars[StartOffSet - 1]))
        {
          --StartOffSet;
        }
      }
      else if (::SIsPunct(pChars[StartOffSet - 1]))
      {
        while (StartOffSet > 0 && ::SIsPunct(pChars[StartOffSet - 1]))
        {
          --StartOffSet;
        }
      }
    }
    
    return StartOffSet;
  }

  // ! Backwards
  else 
  {
    if (StartOffSet >= Len)
    {
      return Len;
    }

    if (::SIsPunct(pChars[StartOffSet])) // punct
    {
      while (StartOffSet < Len && ::SIsPunct(pChars[StartOffSet]))
      {
        ++StartOffSet;
      }

      while (StartOffSet < Len && ::SIsSpace(pChars[StartOffSet]))
      {
        ++StartOffSet;
      }
    }
    else if (::SIsAlnum(pChars[StartOffSet])) // alnums
    {
      while (StartOffSet < Len && ::SIsAlnum(pChars[StartOffSet]))
      {
        ++StartOffSet;
      }

      while (StartOffSet < Len && ::SIsSpace(pChars[StartOffSet]))
      {
        ++StartOffSet;
      }
    }
    else if (SIsPaired(pChars[StartOffSet])) // braces
    {
      ++StartOffSet;
    }
    else // space
    {
      while (StartOffSet < Len && ::SIsSpace(pChars[StartOffSet]))
      {
        ++StartOffSet;
      }
    }
    
    return StartOffSet;
  }
}

// -------------------------------------------------------------------------------------------------

bool TString::StartsWith(const TString& String)const
{
  const int Len = Size();
  const int OtherLen = String.Size();
  
  MAssert(OtherLen, "Sure that this is intended?");

  if (OtherLen > Len)
  {
    return false;
  }
  else if (Len && OtherLen)
  {
    const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
    const TUnicodeChar* pOtherChars = String.mpStringBuffer->ReadPtr();
    
    for (int i = 0; i < OtherLen; ++i)
    {
      if (pChars[i] != pOtherChars[i])
      {
        return false;
      }
    }
    
    return true;
  }
  else
  {
    return false; // really?
  }
}

// -------------------------------------------------------------------------------------------------

bool TString::StartsWithIgnoreCase(const TString& String)const
{
  const int Len = Size();
  const int OtherLen = String.Size();
  
  MAssert(OtherLen, "Sure that this is intended?");

  if (OtherLen > Len)
  {
    return false;
  }
  else if (Len && OtherLen)
  {
    const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
    const TUnicodeChar* pOtherChars = String.mpStringBuffer->ReadPtr();
    
    for (int i = 0; i < OtherLen; ++i)
    {
      if (::towlower(pChars[i]) != ::towlower(pOtherChars[i]))
      {
        return false;
      }
    }
    
    return true;
  }
  else
  {
    return false; // really?
  }
}

// -------------------------------------------------------------------------------------------------

bool TString::EndsWith(const TString& String)const
{
  const int Len = Size();
  const int OtherLen = String.Size();
  
  MAssert(OtherLen, "Sure that this is intended?");

  if (OtherLen > Len)
  {
    return false;
  }
  else if (Len && OtherLen)
  {
    const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
    const TUnicodeChar* pOtherChars = String.mpStringBuffer->ReadPtr();
    
    for (int i = OtherLen - 1, j = Len - 1; i >= 0; --i, --j)
    {
      if (pOtherChars[i] != pChars[j])
      {
        return false;
      }
    }
    
    return true;
  }
  else
  {
    return false; // really?
  }
}

// -------------------------------------------------------------------------------------------------

bool TString::EndsWithIgnoreCase(const TString& String)const
{
  const int Len = Size();
  const int OtherLen = String.Size();
  
  MAssert(OtherLen, "Sure that this is intended?");

  if (OtherLen > Len)
  {
    return false;
  }
  else if (Len && OtherLen)
  {
    const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
    const TUnicodeChar* pOtherChars = String.mpStringBuffer->ReadPtr();
    
    for (int i = OtherLen - 1, j = Len - 1; i >= 0; --i, --j)
    {
      if (::towlower(pOtherChars[i]) != ::towlower(pChars[j]))
      {
        return false;
      }
    }
    
    return true;
  }
  else
  {
    return false; // really?
  }
}

// -------------------------------------------------------------------------------------------------

bool TString::StartsWithChar(TUnicodeChar Char)const
{
  return !IsEmpty() && mpStringBuffer->ReadPtr()[0] == Char; 
}
  
// -------------------------------------------------------------------------------------------------

bool TString::StartsWithCharIgnoreCase(TUnicodeChar Char)const
{
  return !IsEmpty() && ::towlower(mpStringBuffer->ReadPtr()[0]) == ::towlower(Char); 
}
 
// -------------------------------------------------------------------------------------------------

bool TString::EndsWithChar(TUnicodeChar Char)const
{
  return !IsEmpty() && mpStringBuffer->ReadPtr()[Size() - 1] == Char;
}

// -------------------------------------------------------------------------------------------------

bool TString::EndsWithCharIgnoreCase(TUnicodeChar Char)const
{
  return !IsEmpty() && ::towlower(mpStringBuffer->ReadPtr()[Size() - 1]) == ::towlower(Char);
}

// -------------------------------------------------------------------------------------------------

bool TString::Contains(const TString& String)const
{
  return (Find(String) > -1);
}

// -------------------------------------------------------------------------------------------------

bool TString::ContainsIgnoreCase(const TString& String)const
{
  return (FindIgnoreCase(String) > -1);
}

// -------------------------------------------------------------------------------------------------

bool TString::ContainsChar(TUnicodeChar Char)const
{
  return (Find(Char) > -1);
}

// -------------------------------------------------------------------------------------------------

bool TString::ContainsCharIgnoreCase(TUnicodeChar Char)const
{
  return (FindIgnoreCase(Char) > -1);
}

// -------------------------------------------------------------------------------------------------

bool TString::MatchesWildcard(const TString& Pattern)const
{
  const bool IgnoreCase = false;
  return ::SWildcardMatch(Chars(), Pattern.Chars(), IgnoreCase);
}

// -------------------------------------------------------------------------------------------------

bool TString::MatchesWildcardIgnoreCase(const TString& Pattern)const
{
  const bool IgnoreCase = true;
  return ::SWildcardMatch(Chars(), Pattern.Chars(), IgnoreCase);
}

// -------------------------------------------------------------------------------------------------

int TString::CountChar(TUnicodeChar CharToCount) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
  
  int Count = 0;
  
  for (int i = 0; i < Len; ++i)
  {
    if (pChars[i] == CharToCount)
    {
      ++Count;
    }
  }
    
  return Count;
}

// -------------------------------------------------------------------------------------------------

int TString::CountCharIgnoreCase(TUnicodeChar CharToCount) const
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();
  
  const TUnicodeChar LowerCharToCount = (TUnicodeChar)::towlower(CharToCount);
  
  int Count = 0;
  
  for (int i = 0; i < Len; ++i)
  {
    if (::towlower(pChars[i]) == LowerCharToCount)
    {
      ++Count;
    }
  }
    
  return Count;
}

// -------------------------------------------------------------------------------------------------

int TString::Count(const TString& Substring)const
{
  const int SubStringLen = Substring.Size();

  int Count = 0;
  int Pos = Find(Substring, 0);

  while (Pos != -1)
  {
    ++Count;
    Pos = Find(Substring, Pos + SubStringLen);
  }
  
  return Count;
}

// -------------------------------------------------------------------------------------------------

int TString::CountIgnoreCase(const TString& Substring)const
{
  const int SubStringLen = Substring.Size();

  int Count = 0;
  int Pos = FindIgnoreCase(Substring, 0);

  while (Pos != -1)
  {
    ++Count;
    Pos = FindIgnoreCase(Substring, Pos + SubStringLen);
  }
  
  return Count;
}
  
// -------------------------------------------------------------------------------------------------

TUnicodeChar TString::operator[] (int Index)const
{
  MAssert(Index >= 0 && Index <= Size(), "");
  return Chars()[Index];
}

// -------------------------------------------------------------------------------------------------

TString& TString::Prepend(const TString &Other)
{
  Insert(0, Other);
  return *this;
}

TString& TString::Prepend(const TUnicodeChar* pString)
{
  Insert(0, pString);
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::PrependChar(const TUnicodeChar Char)
{
  InsertChar(0, Char);
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::Append(const TString &Other)
{
  Insert(Size(), Other);
  return *this;
}

TString& TString::Append(const TUnicodeChar* pString)
{
  Insert(Size(), pString);
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::AppendChar(const TUnicodeChar Char)
{
  InsertChar(Size(), Char);
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::operator+= (const TString &Other)
{
  return Append(Other);
}

TString& TString::operator+= (const TUnicodeChar *pChars)
{
  return Append(pChars);
}

#if defined(MCompiler_GCC)
  TString& TString::operator+= (const wchar_t* pChars)
  {
    return Append(pChars);
  }
#endif

// -------------------------------------------------------------------------------------------------

TString& TString::Insert(int CharIndex, const TString& String)
{
  Insert(CharIndex, String.Chars());
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::Insert(int CharIndex, const TUnicodeChar* pChars)
{
  MAssert(CharIndex <= Size(), "");

  // do we need to reallocate memory?
  const int InsertSize = (int)::ucslen(pChars);

  if (InsertSize)
  {
    const int OldSize = Size(); // without null
    const int NewSize = OldSize + InsertSize + 1; // with null

    if (NewSize > mpStringBuffer->AllocatedSize())
    {
      TPtr<TStringBuffer> pTmp(new TStringBuffer(NULL, NewSize));

      if (pTmp->AllocatedSize() == 0)
      {
        throw TOutOfMemoryException();
      }

      // copy portion of original string before insertion position
      TMemory::Copy(pTmp->ReadWritePtr(), mpStringBuffer->ReadPtr(),
        CharIndex * sizeof(TUnicodeChar));

      // copy string to insert
      TMemory::Copy(pTmp->ReadWritePtr() + CharIndex, pChars,
        InsertSize * sizeof(TUnicodeChar));

      // copy portion of original string after insertion position
      if (CharIndex < OldSize)
      {
        TMemory::Copy(pTmp->ReadWritePtr() + CharIndex + InsertSize,
          mpStringBuffer->ReadPtr() + CharIndex,
          (OldSize - CharIndex + 1) * sizeof(TUnicodeChar));
      }

      // terminate
      pTmp->ReadWritePtr()[OldSize + InsertSize] = '\0';

      // get rid of the old buffer and use the new one
      mpStringBuffer = pTmp;
    }

    else // NeededSize <= mpStringBuffer->AllocatedSize())
    {
      MakeStringBufferUnique();

      // move portion of original string after insertion pos
      TMemory::Move(mpStringBuffer->ReadWritePtr() + CharIndex + InsertSize,
        mpStringBuffer->ReadPtr() + CharIndex,
        (OldSize - CharIndex + 1) * sizeof(TUnicodeChar));

      // copy string to insert
      TMemory::Copy(mpStringBuffer->ReadWritePtr() + CharIndex,
        pChars, InsertSize * sizeof(TUnicodeChar));

      // terminate
      mpStringBuffer->ReadWritePtr()[OldSize + InsertSize] = '\0';
    }
  }

  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::InsertChar(int CharIndex, const TUnicodeChar Char)
{
  const TUnicodeChar Str[] = { Char, '\0' };
  return Insert(CharIndex, Str);
}

// -------------------------------------------------------------------------------------------------

TString& TString::DeleteChar(int CharIndex)
{
  MAssert(CharIndex >= 0 && CharIndex < Size(), "Invalid Index");
  
  MakeStringBufferUnique();
  
  TMemory::Move(mpStringBuffer->ReadWritePtr() + CharIndex,
    mpStringBuffer->ReadPtr() + CharIndex + 1, 
    (Size() - CharIndex) * sizeof(TUnicodeChar));

  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::DeleteRange(int CharIndexStart, int CharIndexEnd)
{
  if (CharIndexEnd == CharIndexStart)
  {
    return *this;
  }

  MakeStringBufferUnique();
  
  MAssert(CharIndexStart >= 0 && CharIndexStart < Size(), "Invalid Index");
  MAssert(CharIndexEnd > 0 && CharIndexEnd <= Size(), "Invalid Index");
  MAssert(CharIndexEnd >= CharIndexStart, "Invalid Index");

  TMemory::Move(mpStringBuffer->ReadWritePtr() + CharIndexStart,
    mpStringBuffer->ReadPtr() + CharIndexEnd, 
    (Size() - CharIndexEnd + 1) * sizeof(TUnicodeChar));

  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ReplaceChar(
  const TUnicodeChar CharToReplace, 
  const TUnicodeChar ReplaceWithChar)
{
  if (CharToReplace != ReplaceWithChar)
  {
    if (mpStringBuffer != spEmptyStringBuffer)
    {
      MakeStringBufferUnique();

      TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
      const int Len = Size();

      for (int i = 0; i < Len; i++)
      {
        if (pChars[i] == CharToReplace)
        {
          pChars[i] = ReplaceWithChar;
        }
      }
    }
  }
    
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ReplaceCharIgnoreCase(
  const TUnicodeChar CharToReplace, 
  const TUnicodeChar ReplaceWithChar)
{
  if (CharToReplace != ReplaceWithChar)
  {
    if (mpStringBuffer != spEmptyStringBuffer)
    {
      MakeStringBufferUnique();
      
      const TUnicodeChar ToLowerCharToReplace = (TUnicodeChar)::towlower(CharToReplace);
      const int Len = Size();
      
      TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
      
      for (int i = 0; i < Len; i++)
      {
        if (::towlower(pChars[i]) == ToLowerCharToReplace)
        {
          pChars[i] = ReplaceWithChar;
        }
      }
    }  
  }
    
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::Replace(
  const TString& ToReplaceString, 
  const TString& ReplaceWithString)
{
  if (ToReplaceString != ReplaceWithString)
  {
    MAssert(!ToReplaceString.IsEmpty(), "Can not replace nothing");
    
    const int ToReplaceStringLen = ToReplaceString.Size();
    const int ReplaceWithStringLen = ReplaceWithString.Size();
  
    int Pos = Find(ToReplaceString, 0);

    while (Pos != -1)
    {
      DeleteRange(Pos, Pos + ToReplaceStringLen);
      Insert(Pos, ReplaceWithString);

      Pos = Find(ToReplaceString, Pos + ReplaceWithStringLen);
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ReplaceFirst(
  const TString& ToReplaceString, 
  const TString& ReplaceWithString)
{
  if (ToReplaceString != ReplaceWithString)
  {
    MAssert(!ToReplaceString.IsEmpty(), "Can not replace nothing");
    
    const int Pos = Find(ToReplaceString, 0);

    if (Pos != -1)
    {
      DeleteRange(Pos, Pos + ToReplaceString.Size());
      Insert(Pos, ReplaceWithString);
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ReplaceIgnoreCase(
  const TString& ToReplaceString, 
  const TString& ReplaceWithString)
{
  if (ToReplaceString != ReplaceWithString)
  {
    MAssert(!ToReplaceString.IsEmpty(), "Can not replace nothing");
  
    const int ToReplaceStringLen = ToReplaceString.Size();
    const int ReplaceWithStringLen = ReplaceWithString.Size();
  
    int Pos = FindIgnoreCase(ToReplaceString, 0);

    while (Pos != -1)
    {
      DeleteRange(Pos, Pos + ToReplaceStringLen);
      Insert(Pos, ReplaceWithString);

      Pos = FindIgnoreCase(ToReplaceString, Pos + ReplaceWithStringLen);
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ReplaceFirstIgnoreCase(
  const TString& ToReplaceString, 
  const TString& ReplaceWithString)
{
  if (ToReplaceString != ReplaceWithString)
  {
    MAssert(!ToReplaceString.IsEmpty(), "Can not replace nothing");
  
    const int Pos = FindIgnoreCase(ToReplaceString, 0);

    if (Pos != -1)
    {
      DeleteRange(Pos, Pos + ToReplaceString.Size());
      Insert(Pos, ReplaceWithString);
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetChar(int Index, const TUnicodeChar NewChar)
{
  MAssert(Index >= 0 && Index <= Size(), "");

  if (mpStringBuffer->ReadPtr()[Index] != NewChar)
  {
    MakeStringBufferUnique();
    mpStringBuffer->ReadWritePtr()[Index] = NewChar;
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ToLower()
{
  if (mpStringBuffer != spEmptyStringBuffer)
  {
    MakeStringBufferUnique();

    TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
    const int Len = Size();
    
    for (int i = 0; i < Len; ++i)
    {
      pChars[i] = (TUnicodeChar)::towlower(pChars[i]);
    }
  }
   
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::ToUpper()
{
  if (mpStringBuffer != spEmptyStringBuffer)
  {
    MakeStringBufferUnique();

    TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
    const int Len = Size();
    
    for (int i = 0; i < Len; ++i)
    {
      pChars[i] = (TUnicodeChar)::towupper(pChars[i]);
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetFromCharArray(const TArray<TUnicodeChar>& Array)
{
  // do not allocate an empty string
  if (Array.Size())
  {
    mpStringBuffer = new TStringBuffer(NULL, Array.Size() + 1);
    
    if (mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }

    TMemory::Copy(mpStringBuffer->ReadWritePtr(),
      Array.FirstRead(), Array.Size() * sizeof(TUnicodeChar));
    
    mpStringBuffer->ReadWritePtr()[Array.Size()] = '\0';
  }
  else
  {
    Empty();
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------
   
void TString::CreateCharArray(TArray<TUnicodeChar>& Array)const
{
  Array.SetSize(Size());
  TMemory::Copy(Array.FirstWrite(), Chars(), Size() * sizeof(TUnicodeChar));
}

// -------------------------------------------------------------------------------------------------

void TString::CreateCStringArray(
  TArray<char>& Array, 
  const char*   pIconvEncoding,
  bool          AllowLossyConversion)const
{
  MUnused(AllowLossyConversion);
    
  // avoid overhead (CString(kUtf8) already invokes Iconv)
  if (::strcmp(pIconvEncoding, "UTF-8") == 0)
  {
    const char* pSourceChars = CString(kUtf8);
    const size_t SourceLen = ::strlen(pSourceChars);
      
    Array.SetSize((int)SourceLen);
    TMemory::Copy(Array.FirstWrite(), pSourceChars, SourceLen);  
  }
  else
  {
    Array.Empty();
    
    TIconv Converter("UTF-8", pIconvEncoding); // throws error
    
    const char* pSourceChars = CString(kUtf8);
    size_t SourceLen = ::strlen(pSourceChars);

    while (SourceLen > 0)
    {
      // assume dest encoding is not bigger than UTF8 (will rerun if its bigger)
      TArray<char> DestBuffer(MMax(16, (int)SourceLen)); 
      char* pDestChars = DestBuffer.FirstWrite();

      size_t DestLen = DestBuffer.Size();
      
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
          MInvalid("Output buffer is too small");
          // OK (handled) but should be avoided
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
      
      if (Array.Size())
      {
        // expand the array
        const TArray<char> Temp(Array);
        Array.SetSize((int)(Temp.Size() + (DestBuffer.Size() - DestLen)));
      
        TMemory::Copy(Array.FirstWrite(), Temp.FirstRead(), 
          Temp.Size());
      
        TMemory::Copy(Array.FirstWrite() + Temp.Size(), 
          DestBuffer.FirstRead(), DestBuffer.Size() - DestLen);
      }
      else
      {
        // initial copy (maybe also the last)
        Array.SetSize((int)(DestBuffer.Size() - DestLen));
        
        TMemory::Copy(Array.FirstWrite(), DestBuffer.FirstRead(), 
          DestBuffer.Size() - DestLen);
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

TString& TString::SetFromCStringArray(
  const TArray<char>& Array, 
  const char*         pIconvEncoding,
  bool                AllowLossyConversion)
{
  MUnused(AllowLossyConversion);
  
  if (!Array.Size())
  {
    Empty();
    return *this;
  }
  
  const char* pSourceChars = Array.FirstRead();
  size_t SourceLen = Array.Size();

  // avoid reallocations, so calc worst DestBuffer size
  size_t DestBufferSize = 1;
    
  for (size_t i = 0; i < SourceLen; ++i)
  {
    if (pSourceChars[i] == '\0')
    {
      // null terminated. Ignore the array size.
      SourceLen = i; 
      break;
    }
    else if (pSourceChars[i] & 0x80)
    {
      // We may need up to 6 bytes for the utf8 output
      DestBufferSize += 6; 
    }
    else
    {
      ++DestBufferSize;
    }
  }
  
  // avoid overhead (TString(kUtf8) already invokes Iconv)
  if (::strcmp(pIconvEncoding, "UTF-8") == 0)
  {
    TArray<char> DestBuffer((int)(SourceLen + 1)); 
    TMemory::Copy(DestBuffer.FirstWrite(), Array.FirstRead(), SourceLen);
    DestBuffer[(int)SourceLen] = 0;  
      
    *this = TString(DestBuffer.FirstRead(), kUtf8);
  }
  else
  {
    TIconv Converter(pIconvEncoding, "UTF-8"); // throws error
    
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
          MInvalid("Output buffer is too small");
          // OK (handled), but should not happen
          break;

        case EILSEQ:
          // Invalid multibyte sequence in the input
        case EINVAL:
          // Incomplete multibyte sequence in the input
          ++pSourceChars; *pDestChars++ = '?';
          --SourceLen;             
          break;
        }
      }
      
      *pDestChars = '\0';
      
      TempString += TString((const char*)DestBuffer.FirstRead(), TString::kUtf8);
    }

    *this = TempString;
  }
    
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::RemoveAll(const TString &PartToRemove)
{
  const int PartToRemoveLen = PartToRemove.Size();
  
  for (MEver)
  {
    const int StartPos = FindIgnoreCase(PartToRemove);

    if (StartPos == -1)
    {
      return *this;
    }

    DeleteRange(StartPos, StartPos + PartToRemoveLen);
  }
}

// -------------------------------------------------------------------------------------------------

TString& TString::RemoveFirst(const TString& PartToRemove)
{
  const int StartPos = FindIgnoreCase(PartToRemove);

  if (StartPos == -1)
  {
    return *this;
  }

  return DeleteRange(StartPos, StartPos + PartToRemove.Size());
}

// -------------------------------------------------------------------------------------------------

TString& TString::RemoveLast(const TString& PartToRemove)
{                                 
  const int StartPos = FindIgnoreCase(PartToRemove, Size() - 1, true);

  if (StartPos == -1)
  {
    return *this;
  }

  return DeleteRange(StartPos, StartPos + PartToRemove.Size());
}

// -------------------------------------------------------------------------------------------------

TString& TString::RemoveNewlines()
{
  if (mpStringBuffer != spEmptyStringBuffer)
  {
    MakeStringBufferUnique();

    TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
    const int Len = Size();
    
    for (int i = 0; i < Len; ++i)
    {
      if (pChars[i] == 0x0A || pChars[i] == 0x0D)
      {
        pChars[i] = ' ';
      }
    }
  }
    
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::TabsToSpaces(int NumSpaces)
{
  return Replace("\t", TString(" ") * NumSpaces);
}

// -------------------------------------------------------------------------------------------------

TString& TString::SpacesToTabs(int NumSpaces)
{
  return Replace(TString(" ") * NumSpaces, "\t");
}

// -------------------------------------------------------------------------------------------------

TString& TString::StripLeadingWhitespace()
{
  const TUnicodeChar* pChars = mpStringBuffer->ReadPtr();
  const int Len = Size();

  int FirstNonWhiteCharPos = 0;

  while (FirstNonWhiteCharPos < Len && 
         (::iswspace(pChars[FirstNonWhiteCharPos]) || pChars[FirstNonWhiteCharPos] == '\t'))
  {
    ++FirstNonWhiteCharPos;
  }

  if (FirstNonWhiteCharPos)
  {
    DeleteRange(0, FirstNonWhiteCharPos);
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::StripTraillingWhitespace()
{
  if (mpStringBuffer != spEmptyStringBuffer)
  {
    MakeStringBufferUnique();

    TUnicodeChar* pChars = mpStringBuffer->ReadWritePtr();
    const int Len = Size();

    for (int i = Len - 1; i >= 0; --i)
    {
      if (::iswspace(pChars[i]) || pChars[i] == '\t')
      {
        pChars[i] = '\0';
      }
      else
      {
        break;
      }
    }
  }
  
  return *this;
}

// -------------------------------------------------------------------------------------------------

TString& TString::Strip()
{
  return TabsToSpaces(2).StripTraillingWhitespace().StripLeadingWhitespace();
}
  
// -------------------------------------------------------------------------------------------------

void TString::MakeStringBufferUnique()
{
  if (! mpStringBuffer->IsUnique())
  {
    const int RequestedSize = mpStringBuffer->AllocatedSize();

    mpStringBuffer = new TStringBuffer(
      mpStringBuffer->ReadPtr(), mpStringBuffer->AllocatedSize());

    if ((RequestedSize > 0 || mpStringBuffer->ReadPtr()) && 
        mpStringBuffer->AllocatedSize() == 0)
    {
      throw TOutOfMemoryException();
    }
  }
}

// =================================================================================================

MImplementSmallObjDebugNewAndDelete(TString::TStringBuffer)

// -------------------------------------------------------------------------------------------------

TUnicodeChar TString::TStringBuffer::sEmptyString[1] = { '\0' };

// -------------------------------------------------------------------------------------------------

TString::TStringBuffer::TStringBuffer(const TUnicodeChar* pChars, int AllocatedSize)
{ 
  const int StrLen = pChars ? (int)::ucslen(pChars) + 1 : 1;  
  const int RequestedLen = (AllocatedSize == -1) ? StrLen : AllocatedSize;
    
  MAssert(RequestedLen > 1, 
    "Should not allocate memory for an empty string (only the 0 byte). "
    "Use sEmptyStringbuffer instead.");

  try
  {
    mpChars = (TUnicodeChar*)TMemory::AllocSmallObject(
      "#StringBuffer", RequestedLen * sizeof(TUnicodeChar));
    
    mAllocatedSize = RequestedLen;

    if (pChars)
    {
      TMemory::Copy(mpChars, pChars, StrLen * sizeof(TUnicodeChar));
    }
    else
    {
      // a "preallocated" but initial empty string
      mpChars[0] = '\0';
    }
  }
  catch (const TOutOfMemoryException&)
  {
    // can not throw in the constructor. TString must (re)throw.
    mpChars = sEmptyString;
    mAllocatedSize = 0;
  }
}

TString::TStringBuffer::TStringBuffer()
  : mpChars(sEmptyString),
    mAllocatedSize(0)
{
}

// -------------------------------------------------------------------------------------------------
  
TString::TStringBuffer::~TStringBuffer()
{
  if (mpChars != sEmptyString)
  {
    TMemory::FreeSmallObject(mpChars);
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString::TEmptyStringBuffer::TEmptyStringBuffer()
  : TStringBuffer()
{
}

// -------------------------------------------------------------------------------------------------

void* TString::TEmptyStringBuffer::operator new(size_t size)
{
  // bypass TMemory for spEmptyString - it's leaked on purpose

  void* Ptr = ::malloc(sizeof(TString::TEmptyStringBuffer));

  if (Ptr == NULL)
  {
    throw TOutOfMemoryException();
  }

  return Ptr;
}

// -------------------------------------------------------------------------------------------------

void TString::TEmptyStringBuffer::operator delete(void* pObject)
{
  // see operator new
  ::free(pObject);
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool gStringsEqual(const TString& First, const TString& Second)
{
  return gSortStringCompare(First, Second) == 0;
}

bool gStringsEqual(const TUnicodeChar* pFirst, const TString& Second)
{
  return gSortStringCompare(pFirst, Second) == 0;
}

bool gStringsEqual(const TString& First, const TUnicodeChar* pSecond)
{
  return gSortStringCompare(First, pSecond) == 0;
}

bool gStringsEqual(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  return gSortStringCompare(pFirst, pSecond) == 0;
}

#if defined(MCompiler_GCC)
  bool gStringsEqual(const wchar_t* pFirst, const TString& Second)
  {
    return gSortStringCompare(pFirst, Second) == 0;
  }
  
  bool gStringsEqual(const TString& First, const wchar_t* pSecond)
  {
    return gSortStringCompare(First, pSecond) == 0;
  }
#endif

// -------------------------------------------------------------------------------------------------

bool gStringsEqualIgnoreCase(const TString& First, const TString& Second)
{
  return gSortStringCompareIgnoreCase(First, Second) == 0;
}

bool gStringsEqualIgnoreCase(const TUnicodeChar* pFirst, const TString& Second)
{
  return gSortStringCompareIgnoreCase(pFirst, Second) == 0;
}

bool gStringsEqualIgnoreCase(const TString& First, const TUnicodeChar* pSecond)
{
  return gSortStringCompareIgnoreCase(First, pSecond) == 0;
}

bool gStringsEqualIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  return gSortStringCompareIgnoreCase(pFirst, pSecond) == 0;
}

#if defined(MCompiler_GCC)
  bool gStringsEqualIgnoreCase(const wchar_t* pFirst, const TString& Second)
  {
    return gSortStringCompareIgnoreCase(pFirst, Second) == 0;
  }
  
  bool gStringsEqualIgnoreCase(const TString& First, const wchar_t* pSecond)
  {
    return gSortStringCompareIgnoreCase(First, pSecond) == 0;
  }
#endif

// -------------------------------------------------------------------------------------------------

int gSortStringCompare(const TString& First, const TString& Second)
{
  return ::ucscmp(First.Chars(), Second.Chars());
}

int gSortStringCompare(const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucscmp(pFirst, Second.Chars());
}

int gSortStringCompare(const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucscmp(First.Chars(), pSecond);
}

int gSortStringCompare(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  return ::ucscmp(pFirst, pSecond);
}

#if defined(MCompiler_GCC)
  int gSortStringCompare(const wchar_t* pFirst, const TString& Second)
  {
    return ::ucscmp(pFirst, Second.Chars());
  }
  
  int gSortStringCompare(const TString& First, const wchar_t* pSecond)
  {
    return ::ucscmp(First.Chars(), pSecond);
  }
#endif

// -------------------------------------------------------------------------------------------------

int gSortStringCompareIgnoreCase(const TString& First, const TString& Second)
{
  return ::ucsicmp(First.Chars(), Second.Chars());
}

int gSortStringCompareIgnoreCase(const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucsicmp(pFirst, Second.Chars());
}

int gSortStringCompareIgnoreCase(const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucsicmp(First.Chars(), pSecond);
}

int gSortStringCompareIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  return ::ucsicmp(pFirst, pSecond);
}

#if defined(MCompiler_GCC)
  int gSortStringCompareIgnoreCase(const wchar_t* pFirst, const TString& Second)
  {
    return ::ucsicmp(pFirst, Second.Chars());
  }
  
  int gSortStringCompareIgnoreCase(const TString& First, const wchar_t* pSecond)
  {
    return ::ucsicmp(First.Chars(), pSecond);
  }
#endif

// -------------------------------------------------------------------------------------------------

int gSortStringCompareNatural(const TString& First, const TString& Second)
{
  const bool IgnoreCase = false;
  return ::strnatcmp(First.Chars(), Second.Chars(), IgnoreCase);
}

int gSortStringCompareNatural(const TUnicodeChar* pFirst, const TString& Second)
{
  const bool IgnoreCase = false;
  return ::strnatcmp(pFirst, Second.Chars(), IgnoreCase);
}

int gSortStringCompareNatural(const TString& First, const TUnicodeChar* pSecond)
{
  const bool IgnoreCase = false;
  return ::strnatcmp(First.Chars(), pSecond, IgnoreCase);
}

int gSortStringCompareNatural(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  const bool IgnoreCase = false;
  return ::strnatcmp(pFirst, pSecond, IgnoreCase);
}

#if defined(MCompiler_GCC)
  int gSortStringCompareNatural(const wchar_t* pFirst, const TString& Second)
  {
    const bool IgnoreCase = false;
    return ::strnatcmp(pFirst, Second.Chars(), IgnoreCase);
  }
  
  int gSortStringCompareNatural(const TString& First, const wchar_t* pSecond)
  {
    const bool IgnoreCase = false;
    return ::strnatcmp(First.Chars(), pSecond, IgnoreCase);
  }
#endif

// -------------------------------------------------------------------------------------------------

int gSortStringCompareNaturalIgnoreCase(const TString& First, const TString& Second)
{
  const bool IgnoreCase = true;
  return ::strnatcmp(First.Chars(), Second.Chars(), IgnoreCase);
}

int gSortStringCompareNaturalIgnoreCase(const TUnicodeChar* pFirst, const TString& Second)
{
  const bool IgnoreCase = true;
  return ::strnatcmp(pFirst, Second.Chars(), IgnoreCase);
}

int gSortStringCompareNaturalIgnoreCase(const TString& First, const TUnicodeChar* pSecond)
{
  const bool IgnoreCase = true;
  return ::strnatcmp(First.Chars(), pSecond, IgnoreCase);
}

int gSortStringCompareNaturalIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond)
{
  const bool IgnoreCase = true;
  return ::strnatcmp(pFirst, pSecond, IgnoreCase);
}

#if defined(MCompiler_GCC)
  int gSortStringCompareNaturalIgnoreCase(const wchar_t* pFirst, const TString& Second)
  {
    const bool IgnoreCase = true;
    return ::strnatcmp(pFirst, Second.Chars(), IgnoreCase);
  }
  
  int gSortStringCompareNaturalIgnoreCase(const TString& First, const wchar_t* pSecond)
  {
    const bool IgnoreCase = true;
    return ::strnatcmp(First.Chars(), pSecond, IgnoreCase);
  }
#endif

// -------------------------------------------------------------------------------------------------

bool operator== (const TString& First, const TString& Second)
{
  return ::ucscmp(First.Chars(), Second.Chars()) == 0;
}

bool operator== (const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucscmp(pFirst, Second.Chars()) == 0;
}

bool operator== (const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucscmp(First.Chars(), pSecond) == 0;
}

#if defined(MCompiler_GCC)
  bool operator== (const wchar_t* pFirst, const TString& Second)
  {
    return ::ucscmp(pFirst, Second.Chars()) == 0;
  }
  
  bool operator== (const TString& First, const wchar_t* pSecond)
  {
    return ::ucscmp(First.Chars(), pSecond) == 0;
  }
#endif


bool operator!= (const TString& First, const TString& Second)
{
  return ::ucscmp(First.Chars(), Second.Chars()) != 0;
}

bool operator!= (const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucscmp(pFirst, Second.Chars()) != 0;
}

bool operator!= (const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucscmp(First.Chars(), pSecond) != 0;
}

#if defined(MCompiler_GCC)
  bool operator!= (const wchar_t* pFirst, const TString& Second)
  {
    return ::ucscmp(pFirst, Second.Chars()) != 0;
  }
  
  bool operator!= (const TString& First, const wchar_t* pSecond)
  {
    return ::ucscmp(First.Chars(), pSecond) != 0;
  }
#endif


bool operator< (const TString& First, const TString& Second)
{
  return ::ucscmp(First.Chars(), Second.Chars()) < 0;
}

bool operator< (const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucscmp(pFirst, Second.Chars()) < 0;
}

bool operator< (const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucscmp(First.Chars(), pSecond) < 0;
}

#if defined(MCompiler_GCC)
  bool operator< (const wchar_t* pFirst, const TString& Second)
  {
    return ::ucscmp(pFirst, Second.Chars()) < 0;
  }
  
  bool operator< (const TString& First, const wchar_t* pSecond)
  {
    return ::ucscmp(First.Chars(), pSecond) < 0;
  }
#endif


bool operator> (const TString& First, const TString& Second)
{
  return ::ucscmp(First.Chars(), Second.Chars()) > 0;
}

bool operator> (const TUnicodeChar* pFirst, const TString& Second)
{
  return ::ucscmp(pFirst, Second.Chars()) > 0;
}

bool operator> (const TString& First, const TUnicodeChar* pSecond)
{
  return ::ucscmp(First.Chars(), pSecond) > 0;
}

#if defined(MCompiler_GCC)
  bool operator> (const wchar_t* pFirst, const TString& Second)
  {
    return ::ucscmp(pFirst, Second.Chars()) > 0;
  }
  
  bool operator> (const TString& First, const wchar_t* pSecond)
  {
    return ::ucscmp(First.Chars(), pSecond) > 0;
  }
#endif

// -------------------------------------------------------------------------------------------------

TString operator+ (const TString& First, const TString& Second)
{
  return TString(First).Append(Second);
}

TString operator+ (const TUnicodeChar* pFirst, const TString& Second)
{
  return TString(Second).Prepend(pFirst);
}

TString operator+ (const TString& First, const TUnicodeChar* pSecond)
{
  return TString(First).Append(pSecond);
}

#if defined(MCompiler_GCC)
  TString operator+ (const wchar_t* pFirst, const TString& Second)
  {
    return TString(Second).Prepend(pFirst);
  }
  
  TString operator+ (const TString& First, const wchar_t* pSecond)
  {
    return TString(First).Append(pSecond);
  }
#endif

#if defined(MCompiler_Has_RValue_References)
  TString operator+ (TString&& First, const TString& Second)
  {
    First.Append(Second);
    return std::move(First);
  }
  TString operator+ (TString&& First, const TUnicodeChar* pSecond)
  {
    First.Append(pSecond);
    return std::move(First);
  }
  #if defined(MCompiler_GCC)
    TString operator+ (TString&& First, const wchar_t* pSecond)
    {
      First.Append(pSecond);
      return std::move(First);
    }
  #endif
#endif

// -------------------------------------------------------------------------------------------------

TString operator* (const TString& First, int Count)
{
  MAssert(Count >= 0, "Invalid count");

  if (Count == 0)
  {
    return TString();
  }
  else if (Count == 1)
  {
    return First;
  }
  else
  {
    TList<TString> Paragraphs;
    Paragraphs.PreallocateSpace(Count);

    for (int i = 0; i < Count; ++i)
    {
      Paragraphs.Append(First);
    }

    return TString().SetFromParagraphs(Paragraphs, TString());
  }
}

TString operator* (int Count, const TString& Second)
{
  return Second * Count;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

int StringToInt(const TString& String, const TString& Format)
{
  int Value = 0;
  if (! StringToValue(Value, String, Format))
  {
    MInvalid("Not convertible or wrong format");
    return 0;
  }
  
  return Value;
}

// -------------------------------------------------------------------------------------------------

long StringToLong(const TString& String, const TString& Format)
{
  long Value = 0;
  if (! StringToValue(Value, String, Format))
  {
    MInvalid("Not convertible or wrong format");
    return 0L;
  }
  
  return Value;
}

// -------------------------------------------------------------------------------------------------

float StringToFloat(const TString& String, const TString& Format)
{
  float Value = 0;
  if (! StringToValue(Value, String, Format))
  {
    MInvalid("Not convertible or wrong format");
    return 0.0f;
  }
  
  return Value;
}

// -------------------------------------------------------------------------------------------------

double StringToDouble(const TString& String, const TString& Format)
{
  double Value = 0;
  if (! StringToValue(Value, String, Format))
  {
    MInvalid("Not convertible or wrong format");
    return 0.0;
  }
  
  return Value;
}

// -------------------------------------------------------------------------------------------------

bool StringToValue(bool& Value, const TString& String, const TString& Format)
{
  MAssert(Format.IsEmpty(), "Expected/supporting no format specifier");
  
  if (TStringConsts::sInitialized)
  {
    // try true,false YES,NO first. Thats how we serialize them
    if (String == TStringConsts::sTrue)
    {
      Value = true;
      return true;
    }
    else if (String == TStringConsts::sFalse)
    {
      Value = false;
      return true;
    }
    else if (String == TStringConsts::sYes)
    {
      Value = true;
      return true;
    }
    else if (String == TStringConsts::sNo)
    {
      Value = false;
      return true;
    }
  }

  if (gStringsEqualIgnoreCase(String, L"yes") || 
      gStringsEqualIgnoreCase(String, L"true") ||
      gStringsEqualIgnoreCase(String, L"1"))
  {
    Value = true;
    return true;
  }
  
  if (gStringsEqualIgnoreCase(String, L"no") || 
      gStringsEqualIgnoreCase(String, L"false") ||
      gStringsEqualIgnoreCase(String, L"0"))
  {
    Value = false;
    return true;
  }
  
  MInvalid("Not convertible or wrong format");
  return false;
}

bool StringToValue(char& Value, const TString& String, const TString& Format)
{
  // we use a char as a byte and not as character
  int Int = Value;
  if (SFromStringFormat(String, Format, &Int) > 0)
  {
    MAssert(Int >= std::numeric_limits<char>::min() && 
      Int <= std::numeric_limits<char>::max(), "");

    Value = (char)Int;

    return true;
  }
  else
  {
    return false;
  }
}

bool StringToValue(unsigned char& Value, const TString& String, const TString& Format)
{
  unsigned int Int = Value; 
  if (SFromStringFormat(String, Format, &Int) > 0)
  {
    MAssert(Int >= std::numeric_limits<unsigned char>::min() && 
      Int <= std::numeric_limits<unsigned char>::max(), "");

    Value = (unsigned char)Int;

    return true;
  }
  else
  {
    return false;
  }
}

#if defined(MCompiler_GCC)
  bool StringToValue(wchar_t& Value, const TString& String, const TString& Format)
  {
    MStaticAssert(sizeof(wchar_t) == sizeof(int));
    return StringToValue((int&)Value, String, Format);
  }

#else
  bool StringToValue(TUnicodeChar& Value, const TString& String, const TString& Format)
  {
    MStaticAssert(sizeof(TUnicodeChar) == sizeof(unsigned short));
    return StringToValue((unsigned short&)Value, String, Format);
  }

#endif

bool StringToValue(short& Value, const TString& String, const TString& Format)
{
  int Int = Value;
  if (SFromStringFormat(String, Format, &Int) > 0)
  {
    MAssert(Int >= std::numeric_limits<short>::min() && 
      Int <= std::numeric_limits<short>::max(), "");

    Value = (short)Int;   

    return true;
  }
  else
  {
    return false;
  }
}

bool StringToValue(unsigned short& Value, const TString& String, const TString& Format)
{
  unsigned int Int = Value;
  if (SFromStringFormat(String, Format, &Int) > 0)
  {
    MAssert(Int >= std::numeric_limits<unsigned short>::min() && 
      Int <= std::numeric_limits<unsigned short>::max(), "");

    Value = (unsigned short)Int;

    return true;
  }
  else
  {
    return false;
  }
}

bool StringToValue(int& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized &&
      Format == TDefaultStringFormats<int>::SToString())
  {
    if (String == TStringConsts::sMinus1)
    {
      Value = -1;
      return true;
    }
    else if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(unsigned int& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned int>::SToString())
  {
    if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(long& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<long>::SToString())
  {
    if (String == TStringConsts::sMinus1)
    {
      Value = -1;
      return true;
    }
    else if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(unsigned long& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned long>::SToString())
  {
    if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(long long& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<long long>::SToString())
  {
    if (String == TStringConsts::sMinus1)
    {
      Value = -1;
      return true;
    }
    else if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(unsigned long long& Value, const TString& String, const TString& Format)
{
  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned long long>::SToString())
  {
    if (String == TStringConsts::s0)
    {
      Value = 0;
      return true;
    }
    else if (String == TStringConsts::s1)
    {
      Value = 1;
      return true;
    }
  }
  
  return (SFromStringFormat(String, Format, &Value) > 0);
}

bool StringToValue(float& Value, const TString& String, const TString& Format)
{
  lconv* pLocale = ::localeconv();

  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<float>::SToString() &&
      pLocale->decimal_point[0] == '.')
  {
    if (String == TStringConsts::s0Float)
    {
      Value = 0.0f;
      return true;
    }
    else if (String == TStringConsts::s1Float)
    {
      Value = 1.0f;
      return true;
    }
    else if (String == TStringConsts::s0Point5Float)
    {
      Value = 0.5f;
      return true;
    }
  }
  
  #if defined(MCompiler_VisualCPP)  // visuals sscanf impl doesn't handle them correctly
    if (String.StartsWithIgnoreCase(TStringConsts::sMinusInf))
    {
      Value = -std::numeric_limits<float>::infinity();
      return true;
    }
    else if (String.StartsWithIgnoreCase(TStringConsts::sInf))
    {
      Value = std::numeric_limits<float>::infinity();
      return true;
    }
    else if (String.StartsWithIgnoreCase(TStringConsts::sNaN))
    {
      Value = std::numeric_limits<float>::quiet_NaN();
      return true;
    }
  #endif
  
  // support both decimal_point values, whatever locale we are in
  MAssert(::strcmp(pLocale->decimal_point, ".") == 0 || 
    ::strcmp(pLocale->decimal_point, ",") == 0, "Unexpected decimal point");
    
  if (pLocale->decimal_point[0] == ',' && String.Find('.') != -1)
  {
    const TString TempString(TString(String).ReplaceChar('.', ','));
    return (SFromStringFormat(TempString, Format, &Value) > 0);
  }
  else if (pLocale->decimal_point[0] == '.' && String.Find(',') != -1)
  {
    const TString TempString(TString(String).ReplaceChar(',', '.'));
    return (SFromStringFormat(TempString, Format, &Value) > 0);
  }
  else
  {
    return (SFromStringFormat(String, Format, &Value) > 0);
  }
}

bool StringToValue(double& Value, const TString& String, const TString& Format)
{
  lconv* pLocale = ::localeconv();

  // direct, fast conversion for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<double>::SToString() &&
      pLocale->decimal_point[0] == '.')
  {
    if (String == TStringConsts::s0Float)
    {
      Value = 0.0;
      return true;
    }
    else if (String == TStringConsts::s1Float)
    {
      Value = 1.0;
      return true;
    }
    else if (String == TStringConsts::s0Point5Float)
    {
      Value = 0.5;
      return true;
    }
  }
  
  // visuals sscanf impl doesn't handle them correctly
  #if defined(MCompiler_VisualCPP)  
    if (String.StartsWithIgnoreCase(TStringConsts::sMinusInf))
    {
      Value = -std::numeric_limits<double>::infinity();
      return true;
    }
    else if (String.StartsWithIgnoreCase(TStringConsts::sInf))
    {
      Value = std::numeric_limits<double>::infinity();
      return true;
    }
    else if (String.StartsWithIgnoreCase(TStringConsts::sNaN))
    {
      Value = std::numeric_limits<double>::quiet_NaN();
      return true;
    }
  #endif

  // support both decimal_point values, whatever locale we are in
  MAssert(::strcmp(pLocale->decimal_point, ".") == 0 || 
    ::strcmp(pLocale->decimal_point, ",") == 0, "Unexpected decimal point");

  if (pLocale->decimal_point[0] == ',' && String.Find('.') != -1)
  {
    const TString TempString(TString(String).ReplaceChar('.', ','));
    return (SFromStringFormat(TempString, Format, &Value) > 0);
  }
  else if (pLocale->decimal_point[0] == '.' && String.Find(',') != -1)
  {
    const TString TempString(TString(String).ReplaceChar(',', '.'));
    return (SFromStringFormat(TempString, Format, &Value) > 0);
  }
  else
  {
    return (SFromStringFormat(String, Format, &Value) > 0);
  }
}

bool StringToValue(TString& String, const TString &Value, const TString& Format)
{
  MAssert(Format.IsEmpty(), "Formats are not supported");
  
  String = Value;
  return true;
}

// -------------------------------------------------------------------------------------------------

TString ToString(bool Value, const TString& Format)
{
  MAssert(Format.IsEmpty(), "Expected/supporting no format specifier");
  
  // use cached strings to avoid memory allocation overhead
  if (TStringConsts::sInitialized)
  {
    return (Value) ? TStringConsts::sTrue : TStringConsts::sFalse;
  }
  else
  {
    return (Value) ? TString("true") : TString("false");
  }
}

TString ToString(char Value, const TString& Format)
{
  int Int = Value;
  return SToStringFormat(Format, Int);
}

TString ToString(unsigned char Value, const TString& Format)
{
  unsigned int Int = Value;
  return SToStringFormat(Format, Int);
}

#if defined(MCompiler_GCC)
  TString ToString(wchar_t Value, const TString& Format)
  {
    int Int = Value;
    return SToStringFormat(Format, Int);
  }

#else
  TString ToString(TUnicodeChar Value, const TString& Format)
  {
    int Int = Value;
    return SToStringFormat(Format, Int);
  }
#endif

TString ToString(short Value, const TString& Format)
{
  int Int = Value;
  return SToStringFormat(Format, Int);
}

TString ToString(unsigned short Value, const TString& Format)
{
  unsigned int Int = Value;
  return SToStringFormat(Format, Int);
}

TString ToString(int Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<int>::SToString())
  {
    if (Value == -1)
    {
      return TStringConsts::sMinus1;
    }
    else if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }
 
  return SToStringFormat(Format, Value);
}

TString ToString(unsigned int Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned int>::SToString())
  {
    if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(long Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<long>::SToString())
  {
    if (Value == -1)
    {
      return TStringConsts::sMinus1;
    }
    else if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(unsigned long Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned long>::SToString())
  {
    if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }
 
  return SToStringFormat(Format, Value);
}

TString ToString(long long Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<long long>::SToString())
  {
    if (Value == -1)
    {
      return TStringConsts::sMinus1;
    }
    else if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(unsigned long long Value, const TString& Format)
{
  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<unsigned long long>::SToString())
  {
    if (Value == 0)
    {
      return TStringConsts::s0;
    }
    else if (Value == 1)
    {
      return TStringConsts::s1;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(float Value, const TString& Format)
{
  // for the NaN check...
  M__DisableFloatingPointAssertions

  // XML compatible special floats...
  if (TMath::IsNaN(Value))
  {
    return TStringConsts::sInitialized ? TStringConsts::sNaN : "NaN";
  }
  else if (Value == std::numeric_limits<float>::infinity())
  {
    return TStringConsts::sInitialized ? TStringConsts::sInf : "INF";
  }
  else if (Value == -std::numeric_limits<float>::infinity())
  {
    return TStringConsts::sInitialized ? TStringConsts::sMinusInf : "-INF";
  }
  
  M__EnableFloatingPointAssertions

  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<float>::SToString() && 
      ::localeconv()->decimal_point[0] == '.')
  {
    // make sure we don't loose precision with the default conversion rule
    MStaticAssert(std::numeric_limits<float>::digits10 + 3 == 9);

    if (Value == 0.0f)
    {
      return TStringConsts::s0Float;
    }
    else if (Value == 1.0f)
    {
      return TStringConsts::s1Float;
    }
    else if (Value == 0.5f)
    {
      return TStringConsts::s0Point5Float;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(double Value, const TString& Format)
{
  // for the NaN check...
  M__DisableFloatingPointAssertions

  // XML compatible special floats...
  if (TMath::IsNaN(Value))
  {
    return TStringConsts::sInitialized ? TStringConsts::sNaN : "Nan";
  }
  else if (Value == std::numeric_limits<double>::infinity())
  {
    return TStringConsts::sInitialized ? TStringConsts::sInf : "INF";
  }
  else if (Value == -std::numeric_limits<double>::infinity())
  {
    return TStringConsts::sInitialized ? TStringConsts::sMinusInf : "-INF";
  }
  
  M__EnableFloatingPointAssertions

  // use cached strings for often used values to speed up things
  if (TStringConsts::sInitialized && 
      Format == TDefaultStringFormats<double>::SToString() &&
      ::localeconv()->decimal_point[0] == '.')
  {
    // make sure we don't loose precision with the default conversion rule
    MStaticAssert(std::numeric_limits<double>::digits10 + 2 == 17);

    if (Value == 0.0)
    {
      return TStringConsts::s0Float;
    }
    else if (Value == 1.0)
    {
      return TStringConsts::s1Float;
    }
    else if (Value == 0.5)
    {
      return TStringConsts::s0Point5Float;
    }
  }

  return SToStringFormat(Format, Value);
}

TString ToString(const TString& Other, const TString& Format)
{
  MAssert(Format.IsEmpty(), "Formats are not supported");
  return Other;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TStringConverter::ToBase64(const TArray<char>& Source, TArray<char>& Dest)
{
  static const char sAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  if (Source.Size() == 0)
  {
    Dest.SetSize(1);
    Dest.FirstWrite()[0] = 0;
    return;
  }

  const int PreAllocatedSize = Source.Size() * 4 / 3 + 3;

  TList<char> DestList;
  DestList.PreallocateSpace(PreAllocatedSize);

  int SourceReadPos = 0;

  while (SourceReadPos + 3 <= Source.Size())
  {
    const unsigned char b[3] = {
      (unsigned char)Source[SourceReadPos + 0],
      (unsigned char)Source[SourceReadPos + 1],
      (unsigned char)Source[SourceReadPos + 2]
    };
  
    unsigned tmp = (b[0] << 16) | (b[1] << 8) | b[2];

    DestList += sAlphabet[(tmp >> 18) & 0x3F];
    DestList += sAlphabet[(tmp >> 12) & 0x3F];
    DestList += sAlphabet[(tmp >> 6) & 0x3F];
    DestList += sAlphabet[tmp & 0x3F];
                       
    SourceReadPos += 3;
  }

  switch (Source.Size() - SourceReadPos)
  {
  case 2:
  {
    const unsigned char b[2] = {
      (unsigned char)Source[SourceReadPos + 0],
      (unsigned char)Source[SourceReadPos + 1]
    };
  
    unsigned tmp = (b[0] << 16) | (b[1] << 8);

    DestList += sAlphabet[(tmp >> 18) & 0x3F];
    DestList += sAlphabet[(tmp >> 12) & 0x3F];
    DestList += sAlphabet[(tmp >> 6) & 0x3F];
    DestList += '=';
      
  } break;
  
  case 1:
  {                      
    const unsigned char b[1] = {
      (unsigned char)Source[SourceReadPos + 0]
    };

    unsigned tmp = b[0] << 16;
            
    DestList += sAlphabet[(tmp >> 18) & 0x3F];
    DestList += sAlphabet[(tmp >> 12) & 0x3F];
    DestList += '=';

  } break;

  }

  DestList += '\0';
  
  MAssert(DestList.Size() <= PreAllocatedSize, "");
  Dest.SetSize(DestList.Size());
  
  TMemory::Copy(Dest.FirstWrite(), DestList.FirstRead(), DestList.Size());
}

// -------------------------------------------------------------------------------------------------

void TStringConverter::FromBase64(const TArray<char>& Source, TArray<char>& Dest)
{
  if (Source.Size() == 0)
  {
    Dest.Empty();
    return;
  }

  unsigned n = 0;
  unsigned tmp2 = 0;
  
  const int PreAllocatedSize = Source.Size() * 3 / 4;

  TList<char> DestList;
  DestList.PreallocateSpace(PreAllocatedSize);

  const int SourceSize = Source.Size();

  int SourceReadPos = 0;

  while (SourceReadPos < SourceSize) 
  {
    const char c = Source.FirstRead()[SourceReadPos++];

    char tmp = 0;
    
    if ((c >= 'A') && (c <= 'Z')) 
    {
      tmp = (char)(c - 'A');
    } 
    else if ((c >= 'a') && (c <= 'z')) 
    {
      tmp = (char)(26 + (c - 'a'));
    } 
    else if ((c >= '0') && (c <= 57)) 
    {
      tmp = (char)(52 + (c - '0'));
    } 
    else if (c == '+') 
    {
      tmp = 62;
    } 
    else if (c == '/') 
    {
      tmp = 63;
    } 
    else if ((c == '\r') || (c == '\n')) 
    {
      continue;
    } 
    else if (c == '=') 
    {
      if (n == 3) 
      {
        DestList.Append((char)((tmp2 >> 10) & 0xff));
        DestList.Append((char)((tmp2 >> 2) & 0xff));
      } 
      else if (n == 2) 
      {
        DestList += (char)((tmp2 >> 4) & 0xff);
      }
      break;
    }
    else
    {
      if (c == 0)
      {
        MAssert(SourceReadPos == SourceSize, "Invalid Base64Char!");
      }
      else
      {
        MInvalid("Invalid Base64Char!");
      }
    }
    
    tmp2 = ((tmp2 << 6) | (tmp & 0xff));
    
    n++;
    
    if (n == 4) 
    {
      DestList.Append((char)((tmp2 >> 16) & 0xff));
      DestList.Append((char)((tmp2 >> 8) & 0xff));
      DestList.Append((char)(tmp2 & 0xff));
      
      tmp2 = 0;
      n = 0;
    }                          
  }

  MAssert(DestList.Size() <= PreAllocatedSize, "");
  Dest.SetSize(DestList.Size());

  TMemory::Copy(Dest.FirstWrite(), DestList.FirstRead(), DestList.Size());
}

// =================================================================================================

static TString sCurrentLanguage;

// -------------------------------------------------------------------------------------------------

const TList<TString> TStringTranslation::AvailableLanguages()
{
  return TList<TString>();
}

// -------------------------------------------------------------------------------------------------

TString TStringTranslation::CurrentLanguage()
{
  return sCurrentLanguage;
}

// -------------------------------------------------------------------------------------------------

void TStringTranslation::SetCurrentLanguage(const TString& LanguageString)
{
  sCurrentLanguage = LanguageString;
}

// -------------------------------------------------------------------------------------------------

TString TStringTranslation::Translate(const TString& Text)
{
  // ... replace Version defines only
  
  const TString DollarM = (TStringConsts::sInitialized) ? 
    TStringConsts::sDollarM : "%M";

  if (Text.Find(DollarM) != -1)
  {
    TString Result(Text);
  
    Result.Replace("$MProductString", MProductString);
    Result.Replace("$MProductNameAndVersionString", MProductNameAndVersionString);
    Result.Replace("$MVersionString", MVersionString);
    Result.Replace("$MAlphaOrBetaVersionString", MAlphaOrBetaVersionString);
    Result.Replace("$MBuildNumberString", MBuildNumberString);
    Result.Replace("$MSupportEMailAddress", MSupportEMailAddress);
    Result.Replace("$MProductHomeURL", MProductHomeURL);
    
    MAssert(Result.Find("$M") == -1, "Unknown Version String!");

    return Result;
  }
  else
  {
    return Text;
  }
}

TString TStringTranslation::Translate(
  const TString& Text, 
  const TString& Arg1,
  const TString& Arg2,
  const TString& Arg3,
  const TString& Arg4,
  const TString& Arg5)
{
  // ... replace Version defines
  
  TString Result(Translate(Text));
  

  // ... replace '%s's
  
  const TString PercentS = (TStringConsts::sInitialized) ? 
    TStringConsts::sPercentS : "%s";

  const TString* const pArgs[] = {&Arg1, &Arg2, &Arg3, &Arg4, &Arg5};
  
  for (size_t i = 0; i < MCountOf(pArgs); ++i)
  {
    if (Result.Find(PercentS) == -1) // '$s'
    {
      break;
    }
    else
    {
      MAssert(!pArgs[i]->IsEmpty(), "Empty arguments are not allowed!");
      Result.ReplaceFirst(PercentS, *(pArgs[i]));
    }
  }
  
  MAssert(Result.Find(PercentS) == -1, 
    "Number of arguments doesnt match the '%s's");
  
  return Result; 
}

