#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreTypes/Export/Str.h"

#include "CoreTypes/Test/TestString.h"

#include <limits>
#include <locale.h>

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreTypesTest::String()
{
  // we do test C Lang conversions here...
  const TSystem::TSetAndRestoreLocale SetNumericalLocale(LC_NUMERIC, "C");
  const TSystem::TSetAndRestoreLocale SetCTypeLocale(LC_CTYPE, "C");
  
  // disable floating point assertions (we are using rand to create floating point values)
  M__DisableFloatingPointAssertions

  // compare
  BOOST_CHECK_NE(TString("abc"), TString("ABC"));
  BOOST_CHECK_NE(TString("abc"), "ABC");
  BOOST_CHECK_NE(TString("abc"), "abcd");

  BOOST_CHECK(TString("1") > TString("0"));
  BOOST_CHECK("1" > TString("0"));
  BOOST_CHECK(TString("1") > "0");

  // test self assignment
  TString SelfAss("SomeString");
  SelfAss = SelfAss;
  SelfAss = SelfAss.Chars();
  BOOST_CHECK_EQUAL(SelfAss, "SomeString");

  // find
  const bool Backwards = true;
  
  BOOST_CHECK_EQUAL(TString("12344321").Find('1'), 0);
  BOOST_CHECK_EQUAL(TString("12344321").Find("1"), 0);
  BOOST_CHECK_EQUAL(TString("12344321").Find('A'), -1);
  BOOST_CHECK_EQUAL(TString("12344321").Find("A"), -1);
  BOOST_CHECK_EQUAL(TString("12344321").Find('1', 1), 7);
  BOOST_CHECK_EQUAL(TString("12344321").Find('1', 1, !Backwards), 7);
  BOOST_CHECK_EQUAL(TString("12344321").Find('1', 0, Backwards), 0);
  BOOST_CHECK_EQUAL(TString("12344321").Find('3', 8, Backwards), 5);
  BOOST_CHECK_EQUAL(TString("12344321").Find("3", 8, Backwards), 5);
  BOOST_CHECK_EQUAL(TString("12344321").Find("34", 8, Backwards), 2);
  BOOST_CHECK_EQUAL(TString("12344321").Find('3', 9, Backwards), -1);
  BOOST_CHECK_EQUAL(TString("12344321").Find("3", 9, Backwards), -1);
  BOOST_CHECK_EQUAL(TString("12344321").Find("34", 9, Backwards), -1);

  BOOST_CHECK_EQUAL(TString("this is a womb, and this is whee").Find("that"), -1);
  BOOST_CHECK_EQUAL(TString("this is a womb, and this is whee").Find("this"), 0);
  BOOST_CHECK_EQUAL(TString("this is a womb, and this is whee").Find("this", 1), 20);
  BOOST_CHECK_EQUAL(TString("this is a womb, and this is whee").Find("this", 31, Backwards), 20);
  BOOST_CHECK_EQUAL(TString("this is a womb, and this is whee").Find("this", 19, Backwards), 0);

  // remove
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveFirst("s"), "entences_s");
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveFirst("S"), "entences_s");
  
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveLast("s"), "Sentences_");
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveLast("S"), "Sentences_");
  
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveAll("s"), "entence_");
  BOOST_CHECK_EQUAL(TString("Sentences_s").RemoveAll("S"), "entence_");

  // Insert, delete append prepend
  TString String("1234");
  
  String.PrependChar('0');
  BOOST_CHECK_EQUAL(String, "01234");
  
  String.Prepend("-1");
  BOOST_CHECK_EQUAL(String, "-101234");
  
  String.AppendChar('5');
  BOOST_CHECK_EQUAL(String, "-1012345");
  
  String.Append("6");
  BOOST_CHECK_EQUAL(String, "-10123456");
  
  String.InsertChar(4, '0');
  BOOST_CHECK_EQUAL(String, "-101023456");
  
  String.Insert(5, "**");
  BOOST_CHECK_EQUAL(String, "-1010**23456");
  
  String = String + "String" + String;
  BOOST_CHECK_EQUAL(String, "-1010**23456String-1010**23456");
  
  String.DeleteChar(0);
  BOOST_CHECK_EQUAL(String, "1010**23456String-1010**23456");
  
  String.DeleteChar(4);
  BOOST_CHECK_EQUAL(String, "1010*23456String-1010**23456");
  
  
  // Replace
  BOOST_CHECK_EQUAL(TString("23434").Replace("343", ""), "24");
  BOOST_CHECK_EQUAL(TString("23434").ReplaceChar('3', '1'), "21414");
  BOOST_CHECK_EQUAL(TString("232323").Replace("23", "12"), "121212");
  BOOST_CHECK_EQUAL(TString("234234234").Replace("23", "12"), "124124124");
  BOOST_CHECK_EQUAL(TString("224422442244").Replace("42", "24"), "224242424244");
  
  // Split
  const TList<TString> Splits3 = TString("This, is a string with commas, which makes no sense.").SplitAt(',');
  BOOST_CHECK_EQUAL(Splits3.Size(), 3); 

  const TList<TString> Splits4 = TString("This, is a string with commas, which makes no sense,").SplitAt(',');
  BOOST_CHECK_EQUAL(Splits4.Size(), 4); 

  const TList<TString> Splits5 = TString("This, is. a string with commas and points, which makes no sense.").
    SplitAt(MakeList<TUnicodeChar>(',', '.'));
  BOOST_CHECK_EQUAL(Splits5.Size(), 5); 

  const TList<TString> Splits6 = TString("This is a string without commas").SplitAt(',');
  BOOST_CHECK_EQUAL(Splits6.Size(), 1); 

  const TList<TString> Splits7 = TString("Word split and stuff and").SplitAt("and");
  BOOST_CHECK_EQUAL(Splits7.Size(), 3);

  const TList<TString> Splits8 = TString("Word split and stuff an").SplitAt("and");
  BOOST_CHECK_EQUAL(Splits8.Size(), 2);

  const TList<TString> Splits9 = TString("Word split and stuff and other an").SplitAt(
    MakeList<TString>("and", "an"));
  BOOST_CHECK_EQUAL(Splits9.Size(), 4);

  const TList<TString> Splits10 = TString("Word split and stuff and other an").SplitAt(
    MakeList<TString>("split", "stuff"));
  BOOST_CHECK_EQUAL(Splits10.Size(), 3);

  const TList<TString> Splits11 = TString("Word").SplitAt("");
  BOOST_CHECK_EQUAL(Splits11.Size(), 5);

  const TList<TString> Splits12 = TString("aaaa").SplitAt("a");
  BOOST_CHECK_EQUAL(Splits12.Size(), 5);

  const TList<TString> Splits13 = TString("aaaa").SplitAt('a');
  BOOST_CHECK_EQUAL(Splits13.Size(), 5);

  const TList<TString> Splits14 = TString("aaaa").SplitAt(TList<TUnicodeChar>());
  BOOST_CHECK_EQUAL(Splits14.Size(), 1);
  BOOST_CHECK_EQUAL(Splits14.First(), "aaaa");

  const TList<TString> Splits15 = TString("aaaa").SplitAt(TList<TString>());
  BOOST_CHECK_EQUAL(Splits15.Size(), 1);
  BOOST_CHECK_EQUAL(Splits15.First(), "aaaa");

  const TList<TString> Words = MakeList<TString>("This", "is", "a", "list", "of", "Strings");
  BOOST_CHECK_EQUAL(TString("This is a list of Strings").SplitAt(' '), Words);
  
  const TList<TString> WordsExtra = MakeList<TString>("This", "is", "a", "list", "of", "Strings", "");
  BOOST_CHECK_EQUAL(TString("This is a list of Strings ").SplitAt(' '), WordsExtra);
  
  // Paragrahps
  const TList<TString> Paragraphs = MakeList<TString>("This is ", "a list of", " Strings");
  BOOST_CHECK_EQUAL(TString("This is \na list of\n Strings").Paragraphs(), Paragraphs);
    
  // operator *
  BOOST_CHECK_EQUAL(TString("123") * 5, "123123123123123");
  BOOST_CHECK_EQUAL(5 * TString("123"), "123123123123123");
  
  // Wildcard match
  BOOST_CHECK(!TString("Bla 1W3 Olse").MatchesWildcard("* 1w3 *"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcardIgnoreCase("* 1w3 *"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcard("* 1W3 *"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcard("Bla 1W3 Olse"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcard("*"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcardIgnoreCase("*"));
  BOOST_CHECK(TString("Bla 1W3 Olse").MatchesWildcardIgnoreCase("????????????"));
  BOOST_CHECK(TString("7").MatchesWildcard("[1-9]"));
  BOOST_CHECK(!TString("7").MatchesWildcard("[a-Z]"));
  BOOST_CHECK(TString("<?>").MatchesWildcard("<\?>"));
  BOOST_CHECK(TString("<?>").MatchesWildcard("<?>"));
 
  
  // Unicode/Utf8 conversion
  const wchar_t UniChar[] = L"UnicodeMess:\u00c4\u00Dc\u3487\u1234\u2345End";
  
  BOOST_CHECK_EQUAL(TString(UniChar), TString(TString(UniChar).CString(
    TString::kUtf8), TString::kUtf8));
  
  TArray<char> CUtf8Array;
  TString(UniChar).CreateCStringArray(CUtf8Array, TString::kUtf8);
  
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CUtf8Array, 
    TString::kUtf8), TString(UniChar));
  
  BOOST_CHECK_NE(CUtf8Array[CUtf8Array.Size() - 1], '\0');

  
  // Unicode/FileSystem conversion
  const wchar_t FileSystemChars[] = L"NoUnicodeChars.)(!%/\\\"";
  
  BOOST_CHECK_EQUAL(TString(FileSystemChars), TString(TString(FileSystemChars).CString(
    TString::kFileSystemEncoding), TString::kFileSystemEncoding));
  
  TArray<char> CFileSystemArray;
  TString(FileSystemChars).CreateCStringArray(CFileSystemArray, TString::kFileSystemEncoding);
  
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CFileSystemArray, 
    TString::kFileSystemEncoding), TString(FileSystemChars));
  
  BOOST_CHECK_NE(CFileSystemArray[CFileSystemArray.Size() - 1], '\0');
    
  
  // Unicode/PlatformCodePage conversion
  const wchar_t AsciiChars[] = L"NoUnicodeChars.)(!%/\\\"";
  
  BOOST_CHECK_EQUAL(TString(AsciiChars), TString(TString(AsciiChars).CString(
    TString::kPlatformEncoding), TString::kPlatformEncoding));
  
  TArray<char> CPlatformArray;
  TString(AsciiChars).CreateCStringArray(CPlatformArray, TString::kPlatformEncoding);
  
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CPlatformArray, 
    TString::kPlatformEncoding), TString(AsciiChars));

  BOOST_CHECK_NE(CPlatformArray[CPlatformArray.Size() - 1], '\0');

  
  // Unicode/Ascii conversion
  BOOST_CHECK_EQUAL(TString(AsciiChars), TString(TString(AsciiChars).CString(
    TString::kAscii), TString::kAscii));
  
  TArray<char> CStringArray;
  TString(AsciiChars).CreateCStringArray(CStringArray, TString::kAscii);
  
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CStringArray, 
    TString::kAscii), TString(AsciiChars));

  BOOST_CHECK_NE(CStringArray[CStringArray.Size() - 1], '\0');

  
  // CharArray conversion
  TArray<TUnicodeChar> CharArray;
  TString(UniChar).CreateCharArray(CharArray);
  
  BOOST_CHECK_EQUAL(TString(UniChar), TString().SetFromCharArray(CharArray));

  
  // Iconv Conversion
  const wchar_t IconvString[] = L"NoUnicodeChars.)(!%/*\0xa8\0xf9\\\"";
  
  TArray<char> CIconvArray;

  TString(IconvString).CreateCStringArray(CIconvArray, "CP1252");
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CIconvArray, 
    "CP1252"), TString(IconvString));

  BOOST_CHECK_NE(CIconvArray[CIconvArray.Size() - 1], '\0');

  TString(IconvString).CreateCStringArray(CIconvArray, "UTF-8");
  BOOST_CHECK_EQUAL(TString().SetFromCStringArray(CIconvArray, 
    "UTF-8"), TString(IconvString));
    
  
  // Serialization

  const bool Bool = !!::rand();
  bool DeserializedBool = !!::rand();
  BOOST_CHECK(StringToValue(DeserializedBool, ToString(Bool)) && 
    Bool == DeserializedBool); 
 
 
  const char Char = (char)::rand();
  char DeserializedChar = (char)::rand();
  BOOST_CHECK(StringToValue(DeserializedChar, ToString(Char)) && 
    Char == DeserializedChar); 
  
  const unsigned char UChar = (unsigned char)::rand();
  unsigned char DeserializedUChar = (unsigned char)::rand();
  BOOST_CHECK(StringToValue(DeserializedUChar, ToString(UChar)) && 
    UChar == DeserializedUChar); 
  
  
  const short Short = (short)::rand();
  short DeserializedShort = (short)::rand();
  BOOST_CHECK(StringToValue(DeserializedShort, ToString(Short)) && 
    Short == DeserializedShort); 
    
  const unsigned short UShort = (unsigned short)::rand();
  unsigned short DeserializedUShort = (unsigned short)::rand();
  BOOST_CHECK(StringToValue(DeserializedUShort, ToString(UShort)) && 
    UShort == DeserializedUShort); 
  
  
  const int Int = (int)::rand();
  int DeserializedInt = (int)::rand();
  BOOST_CHECK(StringToValue(DeserializedInt, ToString(Int)) && 
    Int == DeserializedInt); 
    
  const unsigned int UInt = (unsigned int)::rand();
  unsigned int DeserializedUInt = (unsigned int)::rand();
  BOOST_CHECK(StringToValue(DeserializedUInt, ToString(UInt)) && 
    UInt == DeserializedUInt); 
  
  
  const long Long = (long)::rand();
  long DeserializedLong = (long)::rand();
  BOOST_CHECK(StringToValue(DeserializedLong, ToString(Long)) && 
    Long == DeserializedLong); 
    
  const unsigned long ULong = (unsigned long)::rand();
  unsigned long DeserializedULong = (unsigned long)::rand();
  BOOST_CHECK(StringToValue(DeserializedULong, ToString(ULong)) && 
    ULong == DeserializedULong); 
  
  for (int i = 0; i < 1000; ++i) // test a bunch of rand floats
  {
    const float Float = (float)::rand() / (float)MMax(1, ::rand());
    float DeserializedFloat;
    BOOST_CHECK(StringToValue(DeserializedFloat, ToString(Float)) && 
      ((Float == DeserializedFloat) || 
       (TMath::IsNaN(Float) && TMath::IsNaN(DeserializedFloat)))); 
  }

  const float NanFloat = std::numeric_limits<float>::quiet_NaN();
  float DeserializedNanFloat;
  BOOST_CHECK(StringToValue(DeserializedNanFloat, ToString(NanFloat)));
  BOOST_CHECK(TMath::IsNaN(NanFloat));
  BOOST_CHECK(TMath::IsNaN(DeserializedNanFloat));
  
  const float InfFloat = std::numeric_limits<float>::infinity();
  float DeserializedInfFloat = (float)::rand() / (float)MMax(1, ::rand());
  BOOST_CHECK(StringToValue(DeserializedInfFloat, ToString(InfFloat)) && 
    InfFloat == DeserializedInfFloat); 
  
  const float NegInfFloat = -std::numeric_limits<float>::infinity();
  float DeserializedNegInfFloat = (float)::rand() / (float)MMax(1, ::rand());
  BOOST_CHECK(StringToValue(DeserializedNegInfFloat, ToString(NegInfFloat)) && 
    NegInfFloat == DeserializedNegInfFloat);
    
  for (int i = 0; i < 1000; ++i) // test a bunch of rand doubles
  {
    const double Double = (double)::rand() / (double)MMax(1, ::rand());
    double DeserializedDouble;
    BOOST_CHECK(StringToValue(DeserializedDouble, ToString(Double)) && 
      ((Double == DeserializedDouble) || 
       (TMath::IsNaN(Double) && TMath::IsNaN(DeserializedDouble)))); 
  }
  
  const double NanDouble = std::numeric_limits<double>::quiet_NaN();
  double DeserializedNanDouble;
  BOOST_CHECK(StringToValue(DeserializedNanDouble, ToString(NanDouble)) && 
    TMath::IsNaN(NanDouble) && TMath::IsNaN(DeserializedNanDouble)); 
      
  const double InfDouble = std::numeric_limits<double>::infinity();
  double DeserializedInfDouble = (double)::rand() / (double)MMax(1, ::rand());
  BOOST_CHECK(StringToValue(DeserializedInfDouble, ToString(InfDouble)) && 
    InfDouble == DeserializedInfDouble); 
  
  const double NegInfDouble = -std::numeric_limits<double>::infinity();
  double DeserializedNegInfDouble = (double)::rand() / (double)MMax(1, ::rand());
  BOOST_CHECK(StringToValue(DeserializedNegInfDouble, ToString(NegInfDouble)) && 
    NegInfDouble == DeserializedNegInfDouble); 
  
      
  // Double/Float serialization (make sure we dont loose precision)
 
  {
    static const float sTestFloat = 2.5523524284362793f;
    
    float FloatValue;
    BOOST_CHECK(StringToValue(FloatValue, ToString(sTestFloat)));
    BOOST_CHECK_EQUAL(sTestFloat, FloatValue);
    BOOST_CHECK(StringToValue(FloatValue, "2.5523524284362793"));
    BOOST_CHECK_EQUAL(sTestFloat, FloatValue);
  }

  {
    static const double sTestDouble = 3.14159265358979323851280895940618620443274267017841339111328125;
   
    double DoubleValue;
    BOOST_CHECK(StringToValue(DoubleValue, ToString(sTestDouble)));
    BOOST_CHECK_EQUAL(sTestDouble, DoubleValue);
    BOOST_CHECK(StringToValue(DoubleValue, "3.14159265358979323851280895940618620443274267017841339111328125"));
    BOOST_CHECK_EQUAL(sTestDouble, DoubleValue);
  }

  for (int t = 0; t < 100; ++t)
  {
    M__DisableFloatingPointAssertions

    double DoubleValue = 1.0;
    
    do
    {
      for (size_t i = 0; i < sizeof(double); ++i)
      {
        *((char*)&DoubleValue + i) = (char)::rand();
      }
    }
    while(TMath::IsNaN(DoubleValue));
    
    double ReadValue;
    BOOST_CHECK(StringToValue(ReadValue, ToString(DoubleValue)));
    BOOST_CHECK_EQUAL(DoubleValue, ReadValue);
  }

  for (int t = 0; t < 100; ++t)
  {
    M__DisableFloatingPointAssertions

    float FloatValue = 1.0f;
    
    do
    {
      for (size_t i = 0; i < sizeof(float); ++i)
      {
        *((char*)&FloatValue + i) = (char)::rand();
      }
    }
    while(TMath::IsNaN(FloatValue));

    float ReadValue;
    BOOST_CHECK(StringToValue(ReadValue, ToString(FloatValue)));
    BOOST_CHECK_EQUAL(FloatValue, ReadValue);
  }

  // Preallocation
  TString Prealloced;
  Prealloced.PreallocateSpace(1000);
  void* pBuffer = (void*)Prealloced.Chars();

  Prealloced.Append("12345678  ");
  Prealloced.Prepend("  wertrew  ");
  Prealloced.Insert(12, "olampbufl");
  Prealloced.Replace("wer", "HosenDreck");
  Prealloced.ReplaceFirst("D", "A");
  Prealloced.RemoveAll("A");
  Prealloced.RemoveFirst("w");
  Prealloced.StripLeadingWhitespace();
  Prealloced.StripTraillingWhitespace();

  BOOST_CHECK_EQUAL(pBuffer, Prealloced.Chars()); 

  // reenabled...
  M__EnableFloatingPointAssertions
}

