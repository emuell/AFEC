#pragma once

#ifndef _RtfFile_H
#define _RtfFile_H

// =================================================================================================

#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Str.h"

#include "CoreTypes/Export/File.h"

// =================================================================================================

class TRtfFile
{
public:
  //! returns true if the given file is maybe a valid RTF file
  static bool SIsRtfFile(const TString& FileName);

  //! return a list of fileextensions that rtf files might be marked with
  static TList<TString> SSupportedExtensions();

  TRtfFile(const TString& FileName);
  ~TRtfFile();
  
  //! convert from/to unicode String Paragraphs, loosing any RFT meta information
  void ReadParagraphs(TList<TString>& Paragraphs)const;
  void WriteParagraphs(const TList<TString>& Paragraphs);

private:   
  void FlushCurrentLine();
  void FlushCurrentCharacters();

  static void SParserOutput(
    void*       Context, 
    int         Char, 
    const char* pStringEncoding);

  class TRtfParser* mpParser;
  TString mFileName;
  const char* mpLastStringEncoding;

  TList<TString> mCurrentStrings;
  TList<TUnicodeChar> mCurrentString;
};


#endif // _RtfFile_H

