#pragma once

#ifndef _String_h_
#define _String_h_

// =================================================================================================

#include "CoreTypes/Export/BaseTypes.h"
#include "CoreTypes/Export/CompilerDefines.h"
#include "CoreTypes/Export/Pointer.h"
#include "CoreTypes/Export/ReferenceCountable.h"
#include "CoreTypes/Export/Array.h"
#include "CoreTypes/Export/List.h"

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  typedef wchar_t TUnicodeChar;

#elif defined(MCompiler_GCC)
  typedef unsigned short TUnicodeChar;

#else
  #error "Unknown Compiler"
#endif

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #if (_MSC_VER >= 1800)
    // snprintf is present in VisualCPP 2015
    #define snwprintf _snwprintf
  #else
    #define snprintf _snprintf
    #define snwprintf _snwprintf
  #endif
#endif

// =================================================================================================

/*!
 * A mutable, wide char, efficient shared buffer string class.
 *
 * Note: Any non const operation of TString will make the string buffer unique,
 * (create a new unshared string buffer) even when the resulting String 
 * is equal to the original string...
!*/

class TString
{
public:
  
  // ===============================================================================================
    
  enum TCStringEncoding
  {
    //! Probably lossy encoding, only valid for the current codepage
    kPlatformEncoding,
    
    //! Encoding ::fopen and other stdc file functions expect. This will
    //! be a POSIX compatible encoding on OSX, and platform codepage
    //! encoding on windows 
    kFileSystemEncoding,
    
    //! Super lossy non checked encoding to standard US-ASCII:
    //! chars > 127 will be converted to a '?'
    kAscii,
    
    //! Non lossy standard UTF-8 encoding
    kUtf8
  };

  // ===============================================================================================
  
  //! allow up to kNumCStringBuffers to be used at once (as arguments
  //! in functions for example)
  enum  {kNumCStringBuffers = 4 };
  
  // ===============================================================================================
  
  //! Project Init/Exit
  static void SInit();
  static void SExit();
  
  //! returns true if the given char is one of 
  //! ' ', '(', ')', '{', '}', '[', ']', '<', '>', 
  //! ';', ':', '/', '\\', '|', ',', '.', '@'
  static bool SIsSeperatorChar(TUnicodeChar Char);

  //! returns the current platforms default newline char sequence
  static TString SNewLine();
          

  //! create an empty string
  TString();
  
  //! create a string from another string
  TString(const TString& Other);

  //! move string data from one string to another
  #if defined(MCompiler_Has_RValue_References)
    TString(TString&& Other);
  #endif

  //! create a string from Unicode chars
  TString(const TUnicodeChar* pChars);
  //! create a string from a Unicode char range
  TString(const TUnicodeChar* pBegin, const TUnicodeChar* pEnd);

  #if defined(MCompiler_GCC)
    //! create a string from a wchar_t string literal
    TString(const wchar_t* pChars);
  #endif
  
  //! implicit conversion from a c string literal. Assumes and assert
  //! that the given chars are plain ASCII. Use TString(pChars, kAscii)
  //! if you want lossy conversion without asserts.
  TString(const char* pChars);
  
  //! create a TString from a byte string, using the specified encoding
  TString(const char* pChars, TCStringEncoding Encoding);
  

  //@{ ... Assignment
  
  TString& operator= (const TString& Other);
  TString& operator= (const TUnicodeChar* pChars);
  
  #if defined(MCompiler_Has_RValue_References)
    TString& operator= (TString&& Other);
  #endif

  #if defined(MCompiler_GCC)
    TString& operator= (const wchar_t* pChars);
  #endif
  
  //! Clear the string, releasing any allocated buffers
  void Empty();

  //! Init the string to "", NOT releasing allocated buffers (handy if you want to 
  // (re)use the preallocated buffer for performance reasons)
  void Clear();
  //@}
  

  //@{ ... Allocation

  //! Size of the internal string buffer in TUnicodeChars. 
  //! Not necessarily the string's size!!!
  int PreallocatedSpace()const;

  //! Prellocate a string buffer, allocating space for at least @param NumChars
  //! The string must be empty, because reallocation will destroy the current buffer.
  //! Should only be used for performance issues, where appending strings costs too
  //! much allocation time.
  void PreallocateSpace(int NumChars);
  //@}


  //@{ ... Basic Properties

  //! without the \0 byte, so a "" string will have the size 0
  int Size()const;

  bool IsEmpty()const;
  bool IsNotEmpty()const;

  //! returns true if we are using chars in the ASCII range (0-126) only
  bool IsPureAscii()const;
  //@}


  //@{ ... Const String operations
  
  //! returns the string between [CharIndexStart, CharIndexEnd),
  //! if CharIndexEnd is -1, the string length will be used
  TString SubString(int CharIndexStart, int CharIndexEnd = -1)const;

  //! return a list of strings, split at the specified char(s)
  TList<TString> SplitAt(const TUnicodeChar Char)const;
  TList<TString> SplitAt(const TList<TUnicodeChar>& Chars)const;
  
  //! return a list of strings, split at the specified string(s)
  TList<TString> SplitAt(const TString& SplitString)const;
  TList<TString> SplitAt(const TList<TString>& SplitStrings)const;
  
  //! returns -1 when the String was not found, else the offset of the String 
  //! @param StartOffSet: the starting offset in n chars when searching 
  //!        for the StringToFind
  //! @param Backwards: search direction
  int Find(
    const TString& StringToFind, 
    int             StartOffSet = 0, 
    bool            Backwards = false)const;
    
  int FindIgnoreCase(
    const TString&  StringToFind, 
    int             StartOffSet = 0, 
    bool            Backwards = false)const;
  
  int Find(
    const TUnicodeChar  CharToFind, 
    int                 StartOffSet = 0, 
    bool                Backwards = false)const;
    
  int FindIgnoreCase(
    const TUnicodeChar  CharToFind, 
    int                 StartOffSet = 0, 
    bool                Backwards = false)const;
  
  //! return the char index of the next words start
  int FindNextWordStart(int StartOffSet, bool Backwards)const;
  int FindNextWordOrSeparatorStart(int StartOffSet, bool Backwards)const;

  //! find shortcuts
  bool StartsWith(const TString& String)const;
  bool StartsWithIgnoreCase(const TString& String)const;
  
  bool EndsWith(const TString& String)const;
  bool EndsWithIgnoreCase(const TString& String)const;
  
  bool StartsWithChar(TUnicodeChar Char)const;
  bool StartsWithCharIgnoreCase(TUnicodeChar Char)const;
  
  bool EndsWithChar(TUnicodeChar Char)const;
  bool EndsWithCharIgnoreCase(TUnicodeChar Char)const;

  bool Contains(const TString& String)const;
  bool ContainsIgnoreCase(const TString& String)const;

  bool ContainsChar(TUnicodeChar Char)const;
  bool ContainsCharIgnoreCase(TUnicodeChar Char)const;
  
  //! returns true if this string matches the given wildcard expression
  //! *,?,\,[abc] are supported
  bool MatchesWildcard(const TString& Pattern)const;
  bool MatchesWildcardIgnoreCase(const TString& Pattern)const;
  
  //! return how often the char CharToCount is present in this string
  int CountChar(TUnicodeChar CharToCount)const;
  int CountCharIgnoreCase(TUnicodeChar CharToCount)const;

  //! return how often the string is present in this string
  int Count(const TString& Substring)const;
  int CountIgnoreCase(const TString& Substring)const;

  //! const access to a char. Non const version is available via SetCar,
  //! to avoid that always this one is chosen
  TUnicodeChar operator[] (int Index)const;
  
  //! const access to the char buffer. Will always return a valid pointer,
  //! even if the string is empty.
  const TUnicodeChar* Chars()const;
  
  //! return a temporary, converted copy of this wide char string into
  //! a single byte c_str. Do NOT hold/store the pointer some where,
  //! because the returned pointer will be autoreleased as soon as it goes out  
  //! of scope! If you need a persistent copy of the string, make a copy!
  const char* CString(TCStringEncoding Encoding = kPlatformEncoding)const;
  //@}
  

  //@{ ... Paragraph conversions

  //! return the number of lines in this string (faster than Paragraphs().Size(), 
  //! if you don't need the strings)
  int NumberOfParagraphs()const;
  //! return a list of strings, splited at newlines characters (Unix, Mac or Windows EOL)
  TList<TString> Paragraphs()const;
  
  //! Combine multiple paragraphs to a single string with the given separator. 
  TString& SetFromParagraphs(
    const TList<TString>& Paragraphs,
    const TString&        Separator = TString("\n"));
  //@}


  //@{ ... Array conversions (non \0 terminated)

  // create from/to a non zero terminated char array
  void CreateCharArray(TArray<TUnicodeChar>& Array)const;
  TString& SetFromCharArray(const TArray<TUnicodeChar>& Array);
  
  //! convert to/from a non 0 terminated cstring buffer
  void CreateCStringArray(
    TArray<char>&     Array, 
    TCStringEncoding  Encoding = kPlatformEncoding)const;

  TString& SetFromCStringArray(
    const TArray<char>& Array, 
    TCStringEncoding    Encoding = kPlatformEncoding);
  
  //! convert to/from a non \0 terminated cstring buffer sung Iconv as converter
  //! When \param AllowLossyConversion is true, non convertible characters
  //! will be replaced with "?"s, else an exception is thrown as soon as the 
  //! conversion for a char failed.
  void CreateCStringArray(
    TArray<char>& Array, 
    const char*   pIconvEncoding,
    bool          AllowLossyConversion = true)const;
    
  TString& SetFromCStringArray(
    const TArray<char>& Array, 
    const char*         pIconvEncoding,
    bool                AllowLossyConversion = true);
  //@}
  
  
  //@{ ... Case change
  
  TString& ToLower();
  TString& ToUpper();
  //@}
  
  
  //@{ ... Append, Prepend, Insert, Replace or Delete Chars/Substrings
  
  //! prepend a string or character
  TString& Prepend(const TString& Other);
  TString& Prepend(const TUnicodeChar* pChars);
  TString& PrependChar(const TUnicodeChar Char);
  
  //! append a string or character
  TString& Append(const TString& Other);
  TString& Append(const TUnicodeChar* pChars);
  TString& AppendChar(const TUnicodeChar Char);

  //! append a string
  TString& operator+= (const TString& Other);
  TString& operator+= (const TUnicodeChar* pChars);
  #if defined(MCompiler_GCC)
    TString& operator+= (const wchar_t* pChars);
  #endif

  //! modify a single character in the string
  TString& SetChar(int Index, const TUnicodeChar NewChar);

  //! Insert a string or a character before the given position
  TString& Insert(int CharIndex, const TString& String);
  TString& Insert(int CharIndex, const TUnicodeChar* pString);
  TString& InsertChar(int CharIndex, const TUnicodeChar Char);
  
  //! Delete a single character or range or character indices
  TString& DeleteChar(int CharIndex);
  TString& DeleteRange(int CharIndexStart, int CharIndexEnd);

  //! substitute all chars with another chars
  TString& ReplaceChar(
    const TUnicodeChar CharToReplace, 
    const TUnicodeChar ReplaceWithChar);
    
  //! same as ReplaceChars, but case insensitive
  TString& ReplaceCharIgnoreCase(
    const TUnicodeChar CharToReplace, 
    const TUnicodeChar ReplaceWithChar);
  
  //! Substitute all occurrences of the given string with another string
  TString& Replace(
    const TString& ToReplaceString, 
    const TString& ReplaceWithString);
  
  //! Substitute the first occurrence of the given string with another string  
  TString& ReplaceFirst(
    const TString& ToReplaceString, 
    const TString& ReplaceWithString);
    
  //! same as Replace, but case insensitive
  TString& ReplaceIgnoreCase(
    const TString& ToReplaceString, 
    const TString& ReplaceWithString);
 
  //! same as ReplaceFirst, but case insensitive  
  TString& ReplaceFirstIgnoreCase(
    const TString& ToReplaceString, 
    const TString& ReplaceWithString);  
  
  //! Removes all occurrences of \param PartToRemove. !! case insensitive !!
  TString& RemoveAll(const TString& PartToRemove);
  
  //! Removes the first occurrence of \param PartToRemove. !! case insensitive !!
  TString& RemoveFirst(const TString& PartToRemove);
  
  //! Removes the last occurrence of \param PartToRemove. !! case insensitive !!
  TString& RemoveLast(const TString& PartToRemove);

  //! replace all newline characters with a space
  TString& RemoveNewlines();
  //@}
  
  
  //@{ ... Common replace helpers
  
  //! convert all tab characters to the given amount of newlines
  TString& TabsToSpaces(int NumSpaces);
  //! convert all "num spaces" to tab characters
  //! This will not strip leading whitespace. 
  TString& SpacesToTabs(int NumSpaces);

  //! remove all spaces at the start of the string
  TString& StripLeadingWhitespace();
  //! remove all spaces at the end of the string
  TString& StripTraillingWhitespace();
  //! remove all spaces at the end and start of the string
  TString& Strip();
  //@}

private:
  
  // ===============================================================================================
  
  /*!
   *  A string buffer which can be shared/used by one or many TStrings
  !*/
  
  class TStringBuffer : public TReferenceCountable
  {
  public:
    //! Constructor makes a copy of the given string.
    //! pString can be NULL or AllocatedSize can be -1.
    //! When pString != NULL, strlen(pString) + 1 num chars are allocated 
    TStringBuffer(const TUnicodeChar* pString, int AllocatedSize);
    
    //! destructor releases the memory
    ~TStringBuffer();
    
    //! const access to the buffer
    int AllocatedSize()const { return mAllocatedSize; }
    const TUnicodeChar* ReadPtr()const { return mpChars; }
    
    //! returns true if this buffer is not shared
    bool IsUnique()const { return RefCount() == 1; }
    
    //! non const access to the buffer. Only allowed when 
    //! not shared (the refcount is 1)!
    TUnicodeChar* ReadWritePtr() { MAssert(IsUnique(), ""); return mpChars; }
    
    MDeclareDebugNewAndDelete

  protected:
    // used by TEmptyStringBuffer only. initializes to sEmptyString
    TStringBuffer();

  private:
    TUnicodeChar* mpChars;
    int mAllocatedSize;

    static TUnicodeChar sEmptyString[1];
  };

  // ===============================================================================================
  
  class TEmptyStringBuffer : public TStringBuffer
  {
  public:
    TEmptyStringBuffer();

    MDeclareDebugNewAndDelete
  };

  // ===============================================================================================

  static void SInitPlatformString();
  static void SExitPlatformString();

  //! release the current buffer and create a new buffer with the same content
  void MakeStringBufferUnique();
  
  TPtr<TStringBuffer> mpStringBuffer;
  
  static TPtr<TEmptyStringBuffer> spEmptyStringBuffer;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline bool TString::IsEmpty()const
{
  return mpStringBuffer->ReadPtr()[0] == 0;
}

// -------------------------------------------------------------------------------------------------

inline bool TString::IsNotEmpty()const
{
  return mpStringBuffer->ReadPtr()[0] != 0;
}

// -------------------------------------------------------------------------------------------------

inline const TUnicodeChar* TString::Chars()const
{
  return mpStringBuffer->ReadPtr();
}

// =================================================================================================

/*!
 * Base64 string encoding tools
!*/

namespace TStringConverter
{
  void ToBase64(const TArray<char>& Source, TArray<char>& Dest);
  void FromBase64(const TArray<char>& Source, TArray<char>& Dest);
}

// =================================================================================================

/*!
 * Text wildcard replacing and markup for translation
!*/

#define MText TStringTranslation::Translate

namespace TStringTranslation
{
  //! Returns identifiers for installed languages.
  const TList<TString> AvailableLanguages();
  
  //! By default the local language.
  TString CurrentLanguage();
  void SetCurrentLanguage(const TString& LanguageString);
  
  //! Return translated text in the currently set language
  TString Translate(const TString& Text);
  //! Return translated text in the currently set language and replace '%s's 
  //! with the specified arguments.
  TString Translate(
    const TString& Text, 
    const TString& Arg1,
    const TString& Arg2 = TString(),
    const TString& Arg3 = TString(),
    const TString& Arg4 = TString(),
    const TString& Arg5 = TString());
}

// =================================================================================================

/*!
 * Iconv string conversions
!*/

class TIconv
{ 
public:
  static const char* SDefaultPlatformEncoding();
  
  TIconv(const char* pFromEncoding, const char* pToEncoding);
  ~TIconv();
  
  bool Open(const char* pFromEncoding, const char* pToEncoding);

  size_t Convert(
    const char**  ppInputString, 
    size_t*       pInputBytesLeft,
    char**        pOutputString, 
    size_t*       pOutputBytesLeft);

  bool Close();
  
private:
  void* m_iconv;
};

// =================================================================================================

/*!
 * CFString/NSString <-> TString conversion
!*/

#if defined(MMac)

  struct __CFString; // Cast from/to NSString*, when used in Obj-C++
  
  //! return a CFString to a TString. CFString can be NULL.
  TString gCreateStringFromCFString(const __CFString* pCFStringRef);
  //! NB: returned string must be *CFRelease'ed* !!
  const __CFString* gCreateCFString(const TString& String);
  
  #ifdef __OBJC__
    @class NSString;
    
    //! convert a NSString to a TString. NSString can be NULL.
    TString gCreateStringFromNSString(NSString* pNsString);
    //! NB: returned NSStrings are autoreleased. retain if you need a strong ref.
    NSString* gCreateNSString(const TString& String);
  #endif
  
#endif // MMac

// =================================================================================================

/*!
 * global TString comparison functions
!*/

// -------------------------------------------------------------------------------------------------

bool gStringsEqual(const TString& First, const TString& Second);
bool gStringsEqual(const TUnicodeChar* pFirst, const TString& Second);
bool gStringsEqual(const TString& First, const TUnicodeChar* pSecond);
bool gStringsEqual(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool gStringsEqual(const wchar_t* pFirst, const TString& Second);
  bool gStringsEqual(const TString& First, const wchar_t* pSecond);
#endif

bool gStringsEqualIgnoreCase(const TString& First, const TString& Second);
bool gStringsEqualIgnoreCase(const TUnicodeChar* pFirst, const TString& Second);
bool gStringsEqualIgnoreCase(const TString& First, const TUnicodeChar* pSecond);
bool gStringsEqualIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool gStringsEqualIgnoreCase(const wchar_t* pFirst, const TString& Second);
  bool gStringsEqualIgnoreCase(const TString& First, const wchar_t* pSecond);
#endif

// -------------------------------------------------------------------------------------------------

//! using !non! "natural" compare rules (as strcmp putting "a10" before "a1")
int gSortStringCompare(const TString& First, const TString& Second);
int gSortStringCompare(const TUnicodeChar* pFirst, const TString& Second);
int gSortStringCompare(const TString& First, const TUnicodeChar* pSecond);
int gSortStringCompare(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  int gSortStringCompare(const wchar_t* pFirst, const TString& Second);
  int gSortStringCompare(const TString& First, const wchar_t* pSecond);
#endif

//! using !non! "natural" compare rules (as strcmp putting "a10" before "a1")
int gSortStringCompareIgnoreCase(const TString& First, const TString& Second);
int gSortStringCompareIgnoreCase(const TUnicodeChar* pFirst, const TString& Second);
int gSortStringCompareIgnoreCase(const TString& First, const TUnicodeChar* pSecond);
int gSortStringCompareIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  int gSortStringCompareIgnoreCase(const wchar_t* pFirst, const TString& Second);
  int gSortStringCompareIgnoreCase(const TString& First, const wchar_t* pSecond);
#endif

// -------------------------------------------------------------------------------------------------

//! using "natural" compare rules (putting "a10" after "a1" when sorting)
int gSortStringCompareNatural(const TString& First, const TString& Second);
int gSortStringCompareNatural(const TUnicodeChar* pFirst, const TString& Second);
int gSortStringCompareNatural(const TString& First, const TUnicodeChar* pSecond);
int gSortStringCompareNatural(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  int gSortStringCompareNatural(const wchar_t* pFirst, const TString& Second);
  int gSortStringCompareNatural(const TString& First, const wchar_t* pSecond);
#endif

//! using "natural" compare rules (putting "a10" after "a1" when sorting)
int gSortStringCompareNaturalIgnoreCase(const TString& First, const TString& Second);
int gSortStringCompareNaturalIgnoreCase(const TUnicodeChar* pFirst, const TString& Second);
int gSortStringCompareNaturalIgnoreCase(const TString& First, const TUnicodeChar* pSecond);
int gSortStringCompareNaturalIgnoreCase(const TUnicodeChar* pFirst, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  int gSortStringCompareNaturalIgnoreCase(const wchar_t* pFirst, const TString& Second);
  int gSortStringCompareNaturalIgnoreCase(const TString& First, const wchar_t* pSecond);
#endif

// -------------------------------------------------------------------------------------------------

/*!
 * global TString (comparison) operators
!*/

// case-sensitive (using gSortStringCompare)
bool operator== (const TString& First, const TString& Second);
bool operator== (const TUnicodeChar* pFirst, const TString& Second);
bool operator== (const TString& First, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool operator== (const wchar_t* pFirst, const TString& Second);
  bool operator== (const TString& First, const wchar_t* pSecond);
#endif

// case-sensitive (using gSortStringCompare)
bool operator!= (const TString& First, const TString& Second);
bool operator!= (const TUnicodeChar* pFirst, const TString& Second);
bool operator!= (const TString& First, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool operator!= (const wchar_t* pFirst, const TString& Second);
  bool operator!= (const TString& First, const wchar_t* pSecond);
#endif

// case-sensitive (using gSortStringCompare)
bool operator< (const TString& First, const TString& Second);
bool operator< (const TUnicodeChar* pFirst, const TString& Second);
bool operator< (const TString& First, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool operator< (const wchar_t* pFirst, const TString& Second);
  bool operator< (const TString& First, const wchar_t* pSecond);
#endif

// case-sensitive (using gSortStringCompare)
bool operator> (const TString& First, const TString& Second);
bool operator> (const TUnicodeChar* pFirst, const TString& Second);
bool operator> (const TString& First, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  bool operator> (const wchar_t* pFirst, const TString& Second);
  bool operator> (const TString& First, const wchar_t* pSecond);
#endif

TString operator+ (const TString& First, const TString& Second);
TString operator+ (const TUnicodeChar* pFirst, const TString& Second);
TString operator+ (const TString& First, const TUnicodeChar* pSecond);
#if defined(MCompiler_GCC)
  TString operator+ (const wchar_t* pFirst, const TString& Second);
  TString operator+ (const TString& First, const wchar_t* pSecond);
#endif
#if defined(MCompiler_Has_RValue_References)
  TString operator+ (TString&& First, const TString& Second);
  TString operator+ (TString&& First, const TUnicodeChar* pSecond);
  #if defined(MCompiler_GCC)
    TString operator+ (TString&& First, const wchar_t* pSecond);
  #endif
#endif

TString operator* (const TString& First, int Count);
TString operator* (int Count, const TString& Second);

// =================================================================================================

/*!
 * std::algorithm compatible binary string compare functions.
!*/

struct TStringSortCompare {
  bool operator() (const TString& First, const TString& Second)const
  {
    return gSortStringCompare(First, Second) < 0;
  }
};

struct TStringSortCompareIgnoreCase {
  bool operator() (const TString& First, const TString& Second)const
  {
    return gSortStringCompareIgnoreCase(First, Second) < 0;
  }
};

struct TStringSortCompareNatural {
  bool operator() (const TString& First, const TString& Second)const
  {
    return gSortStringCompareNatural(First, Second) < 0;
  }
};

struct TStringSortCompareNaturalIgnoreCase {
  bool operator() (const TString& First, const TString& Second)const
  {
    return gSortStringCompareNaturalIgnoreCase(First, Second) < 0;
  }
};

// =================================================================================================

/*! 
 * Base class for TDefaultStringFormats. Implementation detail: uses 
 * preallocated TStrings to avoid allocating temp TStrings and thus avoids 
 * unnecessary overhead when (un)serializing document values.
!*/

class TDefaultStringFormatConsts
{
protected:
  friend class TString;

  static bool sInitialized;

  // ints (int, long, long long)
  static TString sd, sld, slld;

  // unsigned ints (int, long, long long)
  static TString su, slu, sllu;

  // floats
  static TString sf, s3f, sg, s9g;

  // doubles
  static TString slf, s3lf, slg, s17lg;
};

// -------------------------------------------------------------------------------------------------

/*! 
 * Helper class to get default string conversion rules for a base type.
!*/

template <typename T> 
class TDefaultStringFormats : private TDefaultStringFormatConsts
{
public:
  typedef TDefaultStringFormatConsts TConsts;

  static TString SToString();
  static TString SToStringDisplay() { return SToString(); }

  static TString SFromString();
};

// -------------------------------------------------------------------------------------------------

// boolean -> None
template<> inline TString TDefaultStringFormats<bool>::SToString() 
{ return TString(); }
template<> inline TString TDefaultStringFormats<bool>::SFromString() 
{ return TString(); }

// char -> "%d"  (by default serialized as an int and NOT a character)
template<> inline TString TDefaultStringFormats<char>::SToString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }
template<> inline TString TDefaultStringFormats<char>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }

// unsigned char -> "%u"  (by default serialized as an int and NOT a character)
template<> inline TString TDefaultStringFormats<unsigned char>::SToString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }
template<> inline TString TDefaultStringFormats<unsigned char>::SFromString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }

#if defined(MCompiler_GCC)
  // wchar_t -> "%d" (by default serialized as an int and NOT a character)
  template<> inline TString TDefaultStringFormats<wchar_t>::SToString() 
  { return TConsts::sInitialized ? TConsts::sd : "%d"; }
  template<> inline TString TDefaultStringFormats<wchar_t>::SFromString() 
  { return TConsts::sInitialized ? TConsts::sd : "%d"; }
#else
  // TUnicodeChar -> "%d" (by default serialized as an int and NOT a character)
  template<> inline TString TDefaultStringFormats<TUnicodeChar>::SToString() 
  { return TConsts::sInitialized ? TConsts::sd : "%d"; }
  template<> inline TString TDefaultStringFormats<TUnicodeChar>::SFromString() 
  { return TConsts::sInitialized ? TConsts::sd : "%d"; }
#endif

// short -> "%d"
template<> inline TString TDefaultStringFormats<short>::SToString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }
template<> inline TString TDefaultStringFormats<short>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }

// unsigned short: "%u"
template<> inline TString TDefaultStringFormats<unsigned short>::SToString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }
template<> inline TString TDefaultStringFormats<unsigned short>::SFromString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }

// int -> "%d"
template<> inline TString TDefaultStringFormats<int>::SToString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }
template<> inline TString TDefaultStringFormats<int>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sd : "%d"; }

// unsigned int -> "%u"
template<> inline TString TDefaultStringFormats<unsigned int>::SToString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }
template<> inline TString TDefaultStringFormats<unsigned int>::SFromString() 
{ return TConsts::sInitialized ? TConsts::su : "%u"; }

// long -> "%ld"
template<> inline TString TDefaultStringFormats<long>::SToString() 
{ return TConsts::sInitialized ? TConsts::sld : "%ld"; }
template<> inline TString TDefaultStringFormats<long>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sld : "%ld"; }

// unsigned long -> "%lu"
template<> inline TString TDefaultStringFormats<unsigned long>::SToString() 
{ return TConsts::sInitialized ? TConsts::slu : "%lu"; }
template<> inline TString TDefaultStringFormats<unsigned long>::SFromString() 
{ return TConsts::sInitialized ? TConsts::slu : "%lu"; }

// long long -> "%lld"
template<> inline TString TDefaultStringFormats<long long>::SToString() 
{ return TConsts::sInitialized ? TConsts::slld : "%lld"; }
template<> inline TString TDefaultStringFormats<long long>::SFromString() 
{ return TConsts::sInitialized ? TConsts::slld : "%lld"; }

// unsigned long long -> "%llu"
template<> inline TString TDefaultStringFormats<unsigned long long>::SToString() 
{ return TConsts::sInitialized ? TConsts::sllu : "%llu"; }
template<> inline TString TDefaultStringFormats<unsigned long long>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sllu : "%llu"; }

// float -> "%.9g" by default, "%.3f" for displays
template<> inline TString TDefaultStringFormats<float>::SToString() 
{ return TConsts::sInitialized ? TConsts::s9g : "%.9g"; }
template<> inline TString TDefaultStringFormats<float>::SToStringDisplay() 
{ return TConsts::sInitialized ? TConsts::s3f : "%.3f"; }
template<> inline TString TDefaultStringFormats<float>::SFromString() 
{ return TConsts::sInitialized ? TConsts::sg : "%g"; }

// double -> "%.17lg" by default, "%.3lf" for displays
template<> inline TString TDefaultStringFormats<double>::SToString() 
{ return TConsts::sInitialized ? TConsts::s17lg : "%.17lg"; }
template<> inline TString TDefaultStringFormats<double>::SToStringDisplay() 
{ return TConsts::sInitialized ? TConsts::s3lf : "%.3lf"; }
template<> inline TString TDefaultStringFormats<double>::SFromString() 
{ return TConsts::sInitialized ? TConsts::slg : "%.lg"; }

// const char* -> None
template<> inline TString TDefaultStringFormats<const char*>::SToString() 
{ return TString(); }
template<> inline TString TDefaultStringFormats<const char*>::SFromString() 
{ return TString(); }

#if defined(MCompiler_GCC)
  // const wchar_t* -> None
  template<> inline TString TDefaultStringFormats<const wchar_t*>::SToString() 
  { return TString(); }
  template<> inline TString TDefaultStringFormats<const wchar_t*>::SFromString() 
  { return TString(); }
#else
  // const TUnicodeChar* -> None
  template<> inline TString TDefaultStringFormats<const TUnicodeChar*>::SToString() 
  { return TString(); }
  template<> inline TString TDefaultStringFormats<const TUnicodeChar*>::SFromString() 
  { return TString(); }
#endif

// TString -> None
template<> inline TString TDefaultStringFormats<TString>::SToString() 
{ return TString(); }
template<> inline TString TDefaultStringFormats<TString>::SFromString() 
{ return TString(); }

// =================================================================================================

/*!
 * String -> Basetype value conversions
!*/

bool StringToValue(bool& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<bool>::SFromString());

bool StringToValue(char& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<char>::SFromString()); 

bool StringToValue(unsigned char& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<unsigned char>::SFromString());

#if defined(MCompiler_GCC)
  bool StringToValue(wchar_t& Value, const TString& String, 
    const TString& Format = TDefaultStringFormats<wchar_t>::SFromString()); 
#else
  bool StringToValue(TUnicodeChar& Value, const TString& String, 
    const TString& Format = TDefaultStringFormats<TUnicodeChar>::SFromString()); 
#endif

bool StringToValue(short& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<short>::SFromString());

bool StringToValue(unsigned short& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<unsigned short>::SFromString());

bool StringToValue(int& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<int>::SFromString());

bool StringToValue(unsigned int& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<unsigned int>::SFromString());

bool StringToValue(long& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<long>::SFromString());

bool StringToValue(unsigned long& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<unsigned long>::SFromString());

bool StringToValue(long long& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<long long>::SFromString());

bool StringToValue(unsigned long long& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<unsigned long long>::SFromString());

bool StringToValue(float& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<float>::SFromString());

bool StringToValue(double& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<double>::SFromString());

bool StringToValue(TString& Value, const TString& String, 
  const TString& Format = TDefaultStringFormats<TString>::SFromString());

// =================================================================================================

/*!
 * Basetype value -> string conversions
!*/

TString ToString(bool Value, 
  const TString& Format = TDefaultStringFormats<bool>::SToString());

TString ToString(char Value,
  const TString& Format = TDefaultStringFormats<char>::SToString());

TString ToString(unsigned char Value, 
  const TString& Format = TDefaultStringFormats<unsigned char>::SToString());

#if defined(MCompiler_GCC)
  TString ToString(wchar_t Value, 
    const TString& Format = TDefaultStringFormats<wchar_t>::SToString());
#else
  TString ToString(TUnicodeChar Value, 
    const TString& Format = TDefaultStringFormats<TUnicodeChar>::SToString());
#endif

TString ToString(short Value, 
  const TString& Format = TDefaultStringFormats<short>::SToString());

TString ToString(unsigned short Value, 
  const TString& Format = TDefaultStringFormats<unsigned short>::SToString());

TString ToString(int Value, 
  const TString& Format = TDefaultStringFormats<int>::SToString());

TString ToString(unsigned int Value, 
  const TString& Format = TDefaultStringFormats<unsigned int>::SToString());

TString ToString(long Value, 
  const TString& Format = TDefaultStringFormats<long>::SToString());

TString ToString(unsigned long Value, 
  const TString& Format = TDefaultStringFormats<unsigned long>::SToString());

TString ToString(long long Value, 
  const TString& Format = TDefaultStringFormats<long long>::SToString());

TString ToString(unsigned long long Value, 
  const TString& Format = TDefaultStringFormats<unsigned long long>::SToString());

TString ToString(float Value, 
  const TString& Format = TDefaultStringFormats<float>::SToString());

TString ToString(double Value, 
  const TString& Format = TDefaultStringFormats<double>::SToString());

TString ToString(const TString& Other, 
  const TString& Format = TDefaultStringFormats<TString>::SToString());


//! explicit StringToValue conversions
int StringToInt(const TString& String, 
  const TString& Format = TDefaultStringFormats<int>::SToString());

long StringToLong(const TString& String, 
  const TString& Format = TDefaultStringFormats<long>::SToString());

float StringToFloat(const TString& String, 
  const TString& Format = TDefaultStringFormats<float>::SToString());

double StringToDouble (const TString& String, 
  const TString& Format = TDefaultStringFormats<double>::SToString());


#if defined(MCompiler_GCC)

// Hack: These are needed for GCC 4, because it tries to resolve the
// ToString calls while parsing the TObservableBaseType templates

class TVideoScene;
class TEnvelopePoint;
class TPatternSequencePos;
class TPatternLineNoteColumn;
class TPatternLineEffectColumn;
class TInstrumentPluginFxAlias;

TString ToString(const TVideoScene& Other);
bool StringToValue(TVideoScene& Value, const TString& String);

TString ToString(const TEnvelopePoint& Other);
bool StringToValue(TEnvelopePoint& Value, const TString& String);

TString ToString(const TPatternSequencePos& Value);
bool StringToValue(TPatternSequencePos& Value, const TString &String);

TString ToString(const TPatternLineNoteColumn& Other);
bool StringToValue(TPatternLineNoteColumn& Value, const TString& String);

TString ToString(const TPatternLineEffectColumn& Other);
bool StringToValue(TPatternLineEffectColumn& Value, const TString& String);

TString ToString(const TInstrumentPluginFxAlias& Value);
bool StringToValue(TInstrumentPluginFxAlias& Value, const TString& String);

#endif // MCompiler_GCC


#endif // _String_h_

