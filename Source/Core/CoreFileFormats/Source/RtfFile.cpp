#include "CoreFileFormatsPrecompiledHeader.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cctype>

// =================================================================================================

struct TCharProperty
{
  char fBold;
  char fUnderline;
  char fItalic;
  int fFontIndex;
  int fFontCharSet;
};

enum TJustification
{
  justL, 
  justR, 
  justC, 
  justF 
};

struct TParagraphProperty
{
  int xaLeft;                 // left indent in twips
  int xaRight;                // right indent in twips
  int xaFirst;                // first line indent in twips
  TJustification just;        // justification
};

enum TSectionBreakType
{
  sbkNon, 
  sbkCol, 
  sbkEvn, 
  sbkOdd, 
  sbkPg
};

enum TPageFormatting
{
  pgDec, 
  pgURom, 
  pgLRom, 
  pgULtr, 
  pgLLtr
};

struct TSectionProperty
{
  int cCols;                  // number of columns
  TSectionBreakType sbk;                    // section break type
  int xaPgn;                  // x position of page number in twips
  int yaPgn;                  // y position of page number in twips
  TPageFormatting pgnFormat;              // how the page number is formatted

};

struct TDocumentProperty
{
  int xaPage;                 // page width in twips
  int yaPage;                 // page height in twips
  int xaLeft;                 // left margin in twips
  int yaTop;                  // top margin in twips
  int xaRight;                // right margin in twips
  int yaBottom;               // bottom margin in twips
  int pgnStart;               // starting page number in twips
  char fFacingp;              // facing pages enabled?
  char fLandscape;            // landscape or portrait??
};

enum TInternalState 
{ 
  risNorm,
  risSkip,
  risFontTable,
  risBin, 
  risHex,
  risUnicode
};

struct TRtfFont
{
  int mNumber;
  TString mName;
  int mCharset;
};

struct TCurrentState 
{
  TCurrentState()
    : mpPrevState(NULL),
      mCharProperties(),
      mParagraphProperties(),
      mSectionProperties(),
      mDocumentProperties(),
      mDestinationState(risNorm),
      mInternalState(risNorm) { }

  TCurrentState* mpPrevState;

  TCharProperty mCharProperties;
  TParagraphProperty mParagraphProperties;
  TSectionProperty mSectionProperties;
  TDocumentProperty mDocumentProperties;
  TInternalState mDestinationState;
  TInternalState mInternalState;
};

// What types of properties are there?
enum TAvailableProperties
{
  ipropFontIndex,
  ipropFontCharSet,
  ipropBold, 
  ipropItalic, 
  ipropUnderline, 
  ipropLeftInd,
  ipropRightInd, 
  ipropFirstInd, 
  ipropCols, 
  ipropPgnX,
  ipropPgnY, 
  ipropXaPage, 
  ipropYaPage, 
  ipropXaLeft,
  ipropXaRight, 
  ipropYaTop, 
  ipropYaBottom, 
  ipropPgnStart,
  ipropSbk, 
  ipropPgnFormat, 
  ipropFacingp, 
  ipropLandscape,
  ipropJust, 
  ipropPard, 
  ipropPlain, 
  ipropSectd,
  ipropMax
};

enum TAction
{
  actnSpec, 
  actnByte, 
  actnWord
};

enum TPropertyType
{
  propChp, 
  propPap, 
  propSep, 
  propDop
};

struct TPropertyInfo
{
  TAction actn;              // size of value
  TPropertyType prop;          // structure containing value
  int   offset;            // offset of value from base of structure
};

enum TInputFormat
{
  ipfnBin, 
  ipfnHex,
  ipfnUnicode,
  ipfnSkipDest 
};

enum TDestination
{
  idestSkip,
  idestFontTable
};

enum TKeyword
{
  kwdChar, 
  kwdDest, 
  kwdProp, 
  kwdSpec
};

struct TSymbol
{
  const char* szKeyword;  // RTF keyword
  int   dflt;              // default value to use
  bool fPassDflt;         // true to use default value from this table
  TKeyword  kwd;          // base action to take
  int   idx;               // index into property table if kwd == kwdProp
                          // index into destination table if kwd == kwdDest
                          // character to print if kwd == kwdChar
};

// =================================================================================================

// Property descriptions
static const TPropertyInfo rgprop [ipropMax] = {
  {actnWord,   propChp,    offsetof(TCharProperty, fFontIndex)},      // ipropFontIndex
  {actnWord,   propChp,    offsetof(TCharProperty, fFontCharSet)},    // ipropFontCharSet
  {actnByte,   propChp,    offsetof(TCharProperty, fBold)},           // ipropBold
  {actnByte,   propChp,    offsetof(TCharProperty, fItalic)},         // ipropItalic
  {actnByte,   propChp,    offsetof(TCharProperty, fUnderline)},      // ipropUnderline
  {actnWord,   propPap,    offsetof(TParagraphProperty, xaLeft)},     // ipropLeftInd
  {actnWord,   propPap,    offsetof(TParagraphProperty, xaRight)},    // ipropRightInd
  {actnWord,   propPap,    offsetof(TParagraphProperty, xaFirst)},    // ipropFirstInd
  {actnWord,   propSep,    offsetof(TSectionProperty, cCols)},        // ipropCols
  {actnWord,   propSep,    offsetof(TSectionProperty, xaPgn)},        // ipropPgnX
  {actnWord,   propSep,    offsetof(TSectionProperty, yaPgn)},        // ipropPgnY
  {actnWord,   propDop,    offsetof(TDocumentProperty, xaPage)},      // ipropXaPage
  {actnWord,   propDop,    offsetof(TDocumentProperty, yaPage)},      // ipropYaPage
  {actnWord,   propDop,    offsetof(TDocumentProperty, xaLeft)},      // ipropXaLeft
  {actnWord,   propDop,    offsetof(TDocumentProperty, xaRight)},     // ipropXaRight
  {actnWord,   propDop,    offsetof(TDocumentProperty, yaTop)},       // ipropYaTop
  {actnWord,   propDop,    offsetof(TDocumentProperty, yaBottom)},    // ipropYaBottom
  {actnWord,   propDop,    offsetof(TDocumentProperty, pgnStart)},    // ipropPgnStart
  {actnByte,   propSep,    offsetof(TSectionProperty, sbk)},          // ipropSbk
  {actnByte,   propSep,    offsetof(TSectionProperty, pgnFormat)},    // ipropPgnFormat
  {actnByte,   propDop,    offsetof(TDocumentProperty, fFacingp)},    // ipropFacingp
  {actnByte,   propDop,    offsetof(TDocumentProperty, fLandscape)},  // ipropLandscape
  {actnByte,   propPap,    offsetof(TParagraphProperty, just)},       // ipropJust
  {actnSpec,   propPap,    0},                                        // ipropPard
  {actnSpec,   propChp,    0},                                        // ipropPlain
  {actnSpec,   propSep,    0},                                        // ipropSectd
};

// Keyword descriptions
static const TSymbol rgsymRtf[] = {
  //  keyword     dflt    fPassDflt   kwd         idx
  {"b",          1,      false,     kwdProp,    ipropBold},
  {"ul",         1,      false,     kwdProp,    ipropUnderline},
  {"i",          1,      false,     kwdProp,    ipropItalic},
  {"li",         0,      false,     kwdProp,    ipropLeftInd},
  {"ri",         0,      false,     kwdProp,    ipropRightInd},
  {"fi",         0,      false,     kwdProp,    ipropFirstInd},
  {"cols",       1,      false,     kwdProp,    ipropCols},
  {"sbknone",    sbkNon, true,      kwdProp,    ipropSbk},
  {"sbkcol",     sbkCol, true,      kwdProp,    ipropSbk},
  {"sbkeven",    sbkEvn, true,      kwdProp,    ipropSbk},
  {"sbkodd",     sbkOdd, true,      kwdProp,    ipropSbk},
  {"sbkpage",    sbkPg,  true,      kwdProp,    ipropSbk},
  {"pgnx",       0,      false,     kwdProp,    ipropPgnX},
  {"pgny",       0,      false,     kwdProp,    ipropPgnY},
  {"pgndec",     pgDec,  true,      kwdProp,    ipropPgnFormat},
  {"pgnucrm",    pgURom, true,      kwdProp,    ipropPgnFormat},
  {"pgnlcrm",    pgLRom, true,      kwdProp,    ipropPgnFormat},
  {"pgnucltr",   pgULtr, true,      kwdProp,    ipropPgnFormat},
  {"pgnlcltr",   pgLLtr, true,      kwdProp,    ipropPgnFormat},
  {"qc",         justC,  true,      kwdProp,    ipropJust},
  {"ql",         justL,  true,      kwdProp,    ipropJust},
  {"qr",         justR,  true,      kwdProp,    ipropJust},
  {"qj",         justF,  true,      kwdProp,    ipropJust},
  {"paperw",     12240,  false,     kwdProp,    ipropXaPage},
  {"paperh",     15480,  false,     kwdProp,    ipropYaPage},
  {"margl",      1800,   false,     kwdProp,    ipropXaLeft},
  {"margr",      1800,   false,     kwdProp,    ipropXaRight},
  {"margt",      1440,   false,     kwdProp,    ipropYaTop},
  {"margb",      1440,   false,     kwdProp,    ipropYaBottom},
  {"pgnstart",   1,      true,      kwdProp,    ipropPgnStart},
  {"facingp",    1,      true,      kwdProp,    ipropFacingp},
  {"landscape",  1,      true,      kwdProp,    ipropLandscape},
  
  {"fonttbl",    0,      false,      kwdDest,    idestFontTable},
  {"f",          0,       false,      kwdProp,    ipropFontIndex},
  {"fcharset",   0,       false,      kwdProp,    ipropFontCharSet},

  {"line",       0,      false,     kwdChar,    0x0a},
  {"par",        0,      false,     kwdChar,    0x0a},
  {"\r",         0,      false,     kwdChar,    0x0a},
  {"\n",         0,      false,     kwdChar,    0x0a},
  {"tab",        0,      false,     kwdChar,    0x09},
  {"rquote",     0,      false,     kwdChar,    '\''},
  {"ldblquote",  0,      false,     kwdChar,    '"'},
  {"rdblquote",  0,      false,     kwdChar,    '"'},
  {"bin",        0,      false,     kwdSpec,    ipfnBin},
  {"*",          0,      false,     kwdSpec,    ipfnSkipDest},
  {"'",          0,      false,     kwdSpec,    ipfnHex},
  {"u",          0,      false,     kwdSpec,    ipfnUnicode},
  {"author",     0,      false,     kwdDest,    idestSkip},
  {"buptim",     0,      false,     kwdDest,    idestSkip},
  {"colortbl",   0,      false,     kwdDest,    idestSkip},
  {"comment",    0,      false,     kwdDest,    idestSkip},
  {"creatim",    0,      false,     kwdDest,    idestSkip},
  {"doccomm",    0,      false,     kwdDest,    idestSkip},
  {"fonttbl",    0,      false,     kwdDest,    idestSkip},
  {"footer",     0,      false,     kwdDest,    idestSkip},
  {"footerf",    0,      false,     kwdDest,    idestSkip},
  {"footerl",    0,      false,     kwdDest,    idestSkip},
  {"footerr",    0,      false,     kwdDest,    idestSkip},
  {"footnote",   0,      false,     kwdDest,    idestSkip},
  {"ftncn",      0,      false,     kwdDest,    idestSkip},
  {"ftnsep",     0,      false,     kwdDest,    idestSkip},
  {"ftnsepc",    0,      false,     kwdDest,    idestSkip},
  {"header",     0,      false,     kwdDest,    idestSkip},
  {"headerf",    0,      false,     kwdDest,    idestSkip},
  {"headerl",    0,      false,     kwdDest,    idestSkip},
  {"headerr",    0,      false,     kwdDest,    idestSkip},
  {"info",       0,      false,     kwdDest,    idestSkip},
  {"keywords",   0,      false,     kwdDest,    idestSkip},
  {"operator",   0,      false,     kwdDest,    idestSkip},
  {"pict",       0,      false,     kwdDest,    idestSkip},
  {"printim",    0,      false,     kwdDest,    idestSkip},
  {"private1",   0,      false,     kwdDest,    idestSkip},
  {"revtim",     0,      false,     kwdDest,    idestSkip},
  {"rxe",        0,      false,     kwdDest,    idestSkip},
  {"stylesheet", 0,      false,     kwdDest,    idestSkip},
  {"subject",    0,      false,     kwdDest,    idestSkip},
  {"tc",         0,      false,     kwdDest,    idestSkip},
  {"title",      0,      false,     kwdDest,    idestSkip},
  {"txe",        0,      false,     kwdDest,    idestSkip},
  {"xe",         0,      false,     kwdDest,    idestSkip},
  {"{",          0,      false,     kwdChar,    '{'},
  {"}",          0,      false,     kwdChar,    '}'},
  {"\\",         0,      false,     kwdChar,    '\\'}
};

const int isymMax = sizeof(rgsymRtf) / sizeof(TSymbol);

// =================================================================================================

// RTF parser declarations

class TRtfParser
{
public:
  typedef void (*TPParserOutput)(
    void*           Context, 
    int              Char, 
    const char*     pStringEncoding);
    
  TRtfParser(void* Context, TPParserOutput pParserOutputFunc);
  ~TRtfParser();

  enum TParserError
  {
    kOK                = 0,      // Everything's fine!
    kStackUnderflow    = 1,      // Unmatched '}'
    kStackOverflow     = 2,      // Too many '{' -- memory exhausted
    kUnmatchedBrace    = 3,      // RTF ended during an Open group.
    kInvalidHex        = 4,      // invalid hex character found in data
    kBadTable          = 5,      // RTF table (sym or prop) invalid
    kAssertion         = 6,      // Assertion failure
    kEndOfFile         = 7       // End of file reached while reading RTF
  };

  //! Isolate RTF keywords and send them to ParseKeyword;
  //! Push and pop state at the start and end of RTF groups;
  //! Send text to ParseChar for further processing.
  TParserError Parse(TFile* fp);

private:
  //! Save relevant info on a linked list of TCurrentState structures.
  TParserError PushState();

  //! If we're ending a destination (that is, the destination is changing),
  //! call EndGroupAction.
  //! Always restore relevant info from the top of the TCurrentState list.
  TParserError PopState();


  //! get a control word (and its associated value) and
  //! call TranslateKeyword to dispatch the control.
  TParserError ParseKeyword(TFile *fp);

  //! Route the character to the appropriate destination stream.
  TParserError ParseChar(int c);

  //! Search rgsymRtf for szKeyword and evaluate it appropriately.
  //! szKeyword:   The RTF control to evaluate.
  //! param:       The parameter of the RTF control.
  //! fParam:      true if the control had a parameter; (that is, if param is valid)
  //!              false if it did not.
  TParserError TranslateKeyword(char *szKeyword, int param, bool fParam);

  //! Send a character to the output file.
  TParserError PrintChar(int ch);

  //! The destination specified by mDestinationState is coming to a Close.
  //! If there's any cleanup that needs to be done, do it now.
  TParserError EndGroupAction(TInternalState mDestinationState);

  //! Set the property identified by _iprop_ to the value _val_.
  TParserError ApplyPropertyChange(TAvailableProperties iprop, int val);

  //! Change to the destination specified by idest.
  TParserError ChangeDestination(TDestination idest);

  //! Evaluate an RTF control that needs special processing.
  TParserError ParseSpecialKeyword(TInputFormat ipfn);

  //! Set a property that requires code to evaluate.
  TParserError ParseSpecialProperty(TAvailableProperties iprop, int val);

  //! return the string encoding of the current context
  const char* CurrentStringEncoding();
  
  TPParserOutput mpParserOutputFunc;
  void* mpContext;
  
  TList<TRtfFont> mFontTable;

  TOwnerPtr<TCurrentState> mpCurrentState;
  TCurrentState* mpLastState;

  int mGroupLevel;

  int mBin;
  int mParam;

  bool mSkipDestIfUnkown;
};

// -------------------------------------------------------------------------------------------------

TRtfParser::TRtfParser(void* Context, TPParserOutput pParserOutputFunc)
  : mpParserOutputFunc(pParserOutputFunc),
    mpContext(Context),
    mpCurrentState(new TCurrentState()),
    mpLastState(NULL),
    mGroupLevel(0),
    mBin(false),
    mParam(0),
    mSkipDestIfUnkown(false)
{
}

// -------------------------------------------------------------------------------------------------

TRtfParser::~TRtfParser()
{
  TCurrentState* pState = mpLastState;

  while (pState)
  {          
    TCurrentState* pPrevious = pState->mpPrevState;

    delete pState;
    pState = pPrevious;
  }
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::Parse(TFile* pFile)
{
  TParserError ec;

  char ch;
  int cNibble = 2;
  int b = 0;

  while (pFile->ReadByte(ch))
  {
    if (mGroupLevel < 0)
    {
      return kStackUnderflow;
    }

    // if we're parsing binary data, handle it directly
    if (mpCurrentState->mInternalState == risBin)                      
    {
      if ((ec = ParseChar(ch)) != kOK)
      {
        return ec;
      }
    }
    else
    {
      switch (ch)
      {
      case '{':
        if ((ec = PushState()) != kOK)
        {
          return ec;
        }
        break;
      
      case '}':
        if ((ec = PopState()) != kOK)
        {
          return ec;
        }
        break;
      
      case '\\':
        if ((ec = ParseKeyword(pFile)) != kOK)
        {
          return ec;
        }
        break;
      
      // cr and lf are noise characters...
      case 0x0d:
      case 0x0a: 
        break;
      
      default:
        if (mpCurrentState->mInternalState == risNorm)
        {
          if ((ec = ParseChar(ch)) != kOK)
          {
            return ec;
          }
        }
        else if (mpCurrentState->mInternalState == risHex)
        { 
          b = b << 4;

          if (isdigit(ch))
          {
            b += (char) ch - '0';
          }
          else
          {
            if (islower(ch))
            {
              if (ch < 'a' || ch > 'f')
              {
                return kInvalidHex;
              }

              b += 10 + (char) ch - 'a';
            }
            else
            {
              if (ch < 'A' || ch > 'F')
              {
                return kInvalidHex;
              }

              b += 10 + (char) ch - 'A';
            }
          }
          
          cNibble--;
          
          if (!cNibble)
          {
            if ((ec = ParseChar(b)) != kOK)
            {
              return ec;
            }
            
            cNibble = 2;
            b = 0;
            
            mpCurrentState->mInternalState = risNorm;
          }
        } 
        else
        {
          MAssert(mpCurrentState->mInternalState == risUnicode, "");
          
          // ch is the fallback char, mParam he unicode value..
          if ((ec = ParseChar(mParam)) != kOK)
          {
            return ec;
          }
            
          mpCurrentState->mInternalState = risNorm;
        }
        break;
      } 
    } 
  }

  if (mGroupLevel < 0)
  {
    return kStackUnderflow;
  }

  if (mGroupLevel > 0)
  {
    return kUnmatchedBrace;
  }

  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::PushState(void)
{
  TCurrentState* pNewState = new TCurrentState();

  if (pNewState == NULL)
  {
    return kStackOverflow;
  }

  pNewState->mpPrevState = mpLastState;

  pNewState->mCharProperties = mpCurrentState->mCharProperties;
  pNewState->mParagraphProperties = mpCurrentState->mParagraphProperties;
  pNewState->mSectionProperties = mpCurrentState->mSectionProperties;
  pNewState->mDocumentProperties = mpCurrentState->mDocumentProperties;
  pNewState->mDestinationState = mpCurrentState->mDestinationState;
  pNewState->mInternalState = mpCurrentState->mInternalState;

  mpLastState = pNewState;
  mpCurrentState->mInternalState = risNorm;

  ++mGroupLevel;

  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::PopState(void)
{
  if (!mpLastState)
  {
    return kStackUnderflow;
  }

  if (mpCurrentState->mDestinationState != mpLastState->mDestinationState)
  {
    TParserError ec;

    if ((ec = EndGroupAction(mpCurrentState->mDestinationState)) != kOK)
    {
      return ec;
    }
  }

  mpCurrentState->mCharProperties = mpLastState->mCharProperties;
  mpCurrentState->mParagraphProperties = mpLastState->mParagraphProperties;
  mpCurrentState->mSectionProperties = mpLastState->mSectionProperties;
  mpCurrentState->mDocumentProperties = mpLastState->mDocumentProperties;
  mpCurrentState->mDestinationState = mpLastState->mDestinationState;
  mpCurrentState->mInternalState = mpLastState->mInternalState;

  TCurrentState* pOldState = mpLastState;
  mpLastState = mpLastState->mpPrevState;
  
  --mGroupLevel;
  MAssert(mGroupLevel >= 0, "Unexpected nesting!");

  delete pOldState;
  
  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ParseKeyword(TFile* pFile)
{
  char ch;
  bool fParam = false;
  bool fNeg = false;
  int param = 0;
  char *pch;
  char szKeyword[30];
  char szParameter[20];

  szKeyword[0] = '\0';
  szParameter[0] = '\0';
  
  if (!pFile->ReadByte(ch))
  {
    return kEndOfFile;
  }

  if (!isalpha(ch))           // a control symbol; no delimiter.
  {
    szKeyword[0] = (char) ch;
    szKeyword[1] = '\0';

    return TranslateKeyword(szKeyword, 0, fParam);
  }

  for (pch = szKeyword; isalpha(ch); )
  {
    *pch++ = (char) ch;
    
    if (!pFile->ReadByte(ch))
    {
      break;
    }
  }

  *pch = '\0';
  
  if (ch == '-')
  {
    fNeg  = true;
    
    if (!pFile->ReadByte(ch))
    {
      return kEndOfFile;
    }
  }
  
  if (isdigit(ch))
  {
    fParam = true;         // a digit after the control means we have a parameter
    
    for (pch = szParameter; isdigit(ch); )
    {
      *pch++ = (char)ch;

      if (!pFile->ReadByte(ch))
      {
        break;
      }
    }

    *pch = '\0';
    
    param = atoi(szParameter);
    
    if (fNeg)
    {
      param = -param;
    }
    
    mParam = ::atoi(szParameter);
    
    if (fNeg)
    {
      param = -param;
    }
  }

  // skip ' 's behind keywords, but always include them after \u1234 
  // constructs as fallback chars
  if (ch != ' ' || ::strcmp(szKeyword, "u") == 0)
  {
    pFile->RewindByte(ch);
  }
   
  return TranslateKeyword(szKeyword, param, fParam);
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ParseChar(int ch)
{
  if (mpCurrentState->mInternalState == risBin && --mBin <= 0)
  {
    mpCurrentState->mInternalState = risNorm;
  }

  switch (mpCurrentState->mDestinationState)
  {
  case risSkip:
    // Toss this character.
    return kOK;
  
  case risFontTable:
    if (ch == ';')  
    {
      TRtfFont NewFont;
      NewFont.mNumber = mpCurrentState->mCharProperties.fFontIndex;
      NewFont.mName = "Arial"; // see below
      NewFont.mCharset = mpCurrentState->mCharProperties.fFontCharSet;
      
      bool Replaced = false;
      
      for (int i = 0; i < mFontTable.Size(); ++i)
      {
        // double definition for a font? then use the last one...  
        if (mFontTable[i].mNumber == mpCurrentState->mCharProperties.fFontIndex)
        {
          mFontTable[i] = NewFont;
          Replaced = true;
          break;
        }
      }
      
      if (!Replaced)
      {
        mFontTable.Append(NewFont);
      }
    }
    else 
    {
      // could collect the fontname here...
    }
    return kOK;
      
  case risNorm:
    // Output a character. Properties are valid at this point.
    return PrintChar(ch);
  
  default:
    // handle other destinations....
    return kOK;
  }
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::PrintChar(int ch)
{
  if (mpCurrentState->mInternalState == risUnicode || 
      mpCurrentState->mInternalState == risNorm)
  {
    (mpParserOutputFunc)(mpContext, ch, NULL);
  }
  else
  {
    MAssert(mpCurrentState->mInternalState == risBin || 
      mpCurrentState->mInternalState == risHex, "Unexpected char state");
    
    (mpParserOutputFunc)(mpContext, ch, CurrentStringEncoding());
  }
  
  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ApplyPropertyChange(TAvailableProperties iprop, int val)
{
  char *pb = NULL;

  // If we're skipping text, don't do anything.
  if (mpCurrentState->mDestinationState == risSkip)   
  {
    return kOK;
  }

  switch (rgprop[iprop].prop)
  {
  case propDop:
    pb = (char *)&mpCurrentState->mDocumentProperties;
    break;
  
  case propSep:
    pb = (char *)&mpCurrentState->mSectionProperties;
    break;
  
  case propPap:
    pb = (char *)&mpCurrentState->mParagraphProperties;
    break;
  
  case propChp:
    pb = (char *)&mpCurrentState->mCharProperties;
    break;
  
  default:
    if (rgprop[iprop].actn != actnSpec)
    {
      return kBadTable;
    }
    break;
  }

  switch (rgprop[iprop].actn)
  {
  case actnByte:
    pb[rgprop[iprop].offset] = (unsigned char) val;
    break;

  case actnWord:
    (*(int *) (pb + rgprop[iprop].offset)) = val;
    break;

  case actnSpec:
    return ParseSpecialProperty(iprop, val);

  default:
    return kBadTable;
  }

  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ParseSpecialProperty(TAvailableProperties iprop, int val)
{
  switch (iprop)
  {
  case ipropPard:
    TMemory::Zero(&mpCurrentState->mParagraphProperties, sizeof(mpCurrentState->mParagraphProperties));
    return kOK;

  case ipropPlain:
    TMemory::Zero(&mpCurrentState->mCharProperties, sizeof(mpCurrentState->mCharProperties));
    return kOK;

  case ipropSectd:
    TMemory::Zero(&mpCurrentState->mSectionProperties, sizeof(mpCurrentState->mSectionProperties));
    return kOK;

  default:
    return kBadTable;
  }
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::TranslateKeyword(char *szKeyword, int param, bool fParam)
{
  int isym;

  // search for szKeyword in rgsymRtf

  for (isym = 0; isym < isymMax; isym++)
  {
    if (::strcmp(szKeyword, rgsymRtf[isym].szKeyword) == 0)
    {
      break;
    }
  }

  // control word not found?
  if (isym == isymMax)            
  {
    // if this is a new destination
    if (mSkipDestIfUnkown)   
    {
      // skip the destination
      mpCurrentState->mDestinationState = risSkip;          
    }

    // else just discard it
    mSkipDestIfUnkown = false;

    return kOK;
  }

  // found it!  use kwd and idx to determine what to do with it.

  mSkipDestIfUnkown = false;

  switch (rgsymRtf[isym].kwd)
  {
  case kwdProp:
    if (rgsymRtf[isym].fPassDflt || !fParam)
    {
      param = rgsymRtf[isym].dflt;
    }
    return ApplyPropertyChange((TAvailableProperties)rgsymRtf[isym].idx, param);

  case kwdChar:
    return ParseChar(rgsymRtf[isym].idx);

  case kwdDest:
    return ChangeDestination((TDestination)rgsymRtf[isym].idx);

  case kwdSpec:
    return ParseSpecialKeyword((TInputFormat)rgsymRtf[isym].idx);

  default:
    return kBadTable;
  }
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ChangeDestination(TDestination idest)
{
  // if we're skipping text, don't do anything
  if (mpCurrentState->mDestinationState == risSkip) 
  {
    return kOK; 
  }

  switch (idest)
  {
  case idestFontTable:
    mpCurrentState->mDestinationState = risFontTable;
    break;
  
  default:
    // when in doubt, skip it...
    mpCurrentState->mDestinationState = risSkip;              
    break;
  }

  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::EndGroupAction(TInternalState DestinationState)
{
  return kOK;
}

// -------------------------------------------------------------------------------------------------

TRtfParser::TParserError TRtfParser::ParseSpecialKeyword(TInputFormat ipfn)
{
  // if we're skipping, and it's not the \bin keyword, ignore it.
  if (mpCurrentState->mDestinationState == risSkip && ipfn != ipfnBin)
  {
    return kOK;                        
  }

  switch (ipfn)
  {
  case ipfnBin:
    mpCurrentState->mInternalState = risBin;
    mBin = mParam;
    break;

  case ipfnSkipDest:
    mSkipDestIfUnkown = true;
    break;

  case ipfnHex:
    mpCurrentState->mInternalState = risHex;
    break;

  case ipfnUnicode:
    mpCurrentState->mInternalState = risUnicode;
    break;

  default:
    return kBadTable;
  }

  return kOK;
}

// -------------------------------------------------------------------------------------------------

const char* TRtfParser::CurrentStringEncoding()
{
  int FontCharset = -1;
  
  for (int i = 0; i < mFontTable.Size(); ++i)
  {
    if (mFontTable[i].mNumber == mpCurrentState->mCharProperties.fFontIndex)
    {
      FontCharset = mFontTable[i].mCharset;
      break;
    }
  }
  
  if (FontCharset != -1)
  {
    switch (FontCharset) 
    {
      case 0:  // ANSI_CHARSET
        return "CP1252";
      
      case 1:  // DEFAULT_CHARSET 
        return "CP1252"; 
      
      case 2:  // SYMBOL_CHARSET (not supported)
        return "CP1252"; 
      
      case 77: // MAC_CHARSET
        return "MACINTOSH";
      
      case 78: // ???
        return "SJIS";
      
      case 128: // SHIFTJIS_CHARSET
        return "CP932";
      
      case 129: // HANGEUL_CHARSET
        return "CP949";
      
      case 130: // JOHAB_CHARSET
        return "CP1361";
      
      case 134: // GB2312_CHARSET
        return "CP936";
      
      case 136: // CHINESEBIG5_CHARSET
        return "CP950";
      
      case 161: // GREEK_CHARSET
        return "CP1253";
      
      case 162: // TURKISH_CHARSET
        return "CP1254";
      
      case 163: // VIETNAMESE_CHARSET
        return "CP1258";
      
      case 177: // HEBREW_CHARSET
        return "CP1255";
      
      case 178: // ARABIC_CHARSET / ARABICSIMPLIFIED_CHARSET
      case 179: // ???
      case 180: // ???
        return "CP1256";
      
      case 181: // ???
        return "CP1255";

      case 186: // BALTIC_CHARSET
        return "CP1257";
      
      case 204: // RUSSIAN_CHARSET / CYRILLIC_CHARSET
        return "CP1251";
      
      case 222: // THAI_CHARSET
        return "CP874";
      
      case 238: // EASTEUROPE_CHARSET
        return "CP1250";
      
      case 254: // ???
        return "CP437";
      
      case 255: // OEM_CHARSET
        return "CP850";

      default:
        MInvalid("Unhandled font charset");
        return "CP1252";
    }
  }
  
  // default windows encoding...
  return "CP1252";
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool TRtfFile::SIsRtfFile(const TString& FileName)
{
  TFile File(FileName);

  if (File.Open(TFile::kRead))
  {
    char Buffer[5];
    
    if (File.ReadByte(Buffer[0]) && File.ReadByte(Buffer[1]) && 
        File.ReadByte(Buffer[2]) && File.ReadByte(Buffer[3]) && 
        File.ReadByte(Buffer[4]))
    {
      return Buffer[0] == '{' && Buffer[1] == '\\' && 
        Buffer[2] == 'r' && Buffer[3] == 't' && Buffer[4] == 'f';
    }
    else
    {
      return false;
    }
  }
  else
  {
    MInvalid("File should exist!");
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

TList<TString> TRtfFile::SSupportedExtensions()
{
  return MakeList<TString>("*.rtf");
}

// -------------------------------------------------------------------------------------------------

TRtfFile::TRtfFile(const TString& FileName)
  : mFileName(FileName),
    mpLastStringEncoding(NULL)
{
  mpParser = new TRtfParser(this, SParserOutput);
}

// -------------------------------------------------------------------------------------------------

TRtfFile::~TRtfFile()
{
  delete mpParser;
}

// -------------------------------------------------------------------------------------------------

void TRtfFile::ReadParagraphs(TList<TString>& Paragraphs)const
{
  if (!SIsRtfFile(mFileName))
  {
    throw TReadableException(MText("The given file is not an RTF file!"));
  }
  
  TFile File(mFileName);

  if (!File.Open(TFile::kRead))
  {
    throw TReadableException(MText("Failed to open the file for reading!"));
  }

  const TRtfParser::TParserError Error = mpParser->Parse(&File);

  if (Error != TRtfParser::kOK)
  {
    throw TReadableException(MText("Failed to parse the RTF file!"));
  }

  // dont forget the last paragraph...
  if (mCurrentString.Size())
  {
    const_cast<TRtfFile*>(this)->FlushCurrentLine();
  }

  // we do add a new empty line for the upcoming line, so remove it
  if (mCurrentStrings.Size())
  {
    const_cast<TRtfFile*>(this)->mCurrentStrings.DeleteLast();
  }
  
  Paragraphs = mCurrentStrings;
  const_cast<TRtfFile*>(this)->mCurrentStrings.Empty();
}

// -------------------------------------------------------------------------------------------------

void TRtfFile::WriteParagraphs(const TList<TString>& Paragraphs)
{
  TFile File(mFileName);

  if (!File.Open(TFile::kWrite))
  {
    throw TReadableException(MText("Failed to open the file for writing!"));
  }

  #define MWriteCString(CStr) File.Write(CStr, strlen(CStr))

  MWriteCString("{\\rtf\\ansi\n");

  const int NumParagraphs = Paragraphs.Size();

  for (int i = 0; i < NumParagraphs; ++i)
  {
    TString CurrentLine = Paragraphs[i];

    // Must replace backslash syntax chars
    CurrentLine.Replace("\\", "\\\\");
    CurrentLine.Replace("{", "\\{");
    CurrentLine.Replace("}", "\\}");

    if (CurrentLine.IsPureAscii())
    {
      MWriteCString(CurrentLine.CString());
    }
    else
    {
      for (int c = 0; c < CurrentLine.Size(); ++c)
      {
        const TUnicodeChar Char = CurrentLine[c];
        
        if (Char >= 127)
        {
          const TString Escaped = "\\u" + ToString((int)Char, "%d?");
          MWriteCString(Escaped.CString());
        }
        else
        {
          File.Write((char)Char);
        }
      }
    }

    MWriteCString("\\\n");
  }
  
  MWriteCString("}\n");
}

// -------------------------------------------------------------------------------------------------

void TRtfFile::FlushCurrentCharacters()
{
  if (mCurrentString.Size())
  {
    if (mCurrentString.Last() != '\0')
    {
      mCurrentString.Append('\0');
    }

    if (mpLastStringEncoding)
    {
      // Convert back to a 8 byte string and Convert to unicode
      // with the given encoding...
      
      TArray<char> CharArray(mCurrentString.Size());
      for (int i = 0; i < mCurrentString.Size(); ++i)
      {
        CharArray[i] = (char)mCurrentString[i];
      }
      
      try
      {            
        const TString UniString = TString().SetFromCStringArray( 
          CharArray, mpLastStringEncoding);
        
        if (mCurrentStrings.IsEmpty())
        {
          mCurrentStrings.Append(UniString);
        }
        else
        {
          mCurrentStrings.Last().Append(UniString);
        }
      }
      catch (const TReadableException&)
      {
        if (mCurrentStrings.IsEmpty())
        {
          mCurrentStrings.Append(mCurrentString.FirstRead());
        }
        else
        {
          mCurrentStrings.Last().Append(mCurrentString.FirstRead());
        }
      }
    }
    else
    {
      if (mCurrentStrings.IsEmpty())
      {
        mCurrentStrings.Append(mCurrentString.FirstRead());
      }
      else
      {
        mCurrentStrings.Last().Append(mCurrentString.FirstRead());
      }
    }
    
    mCurrentString.Empty();
  }
}

// -------------------------------------------------------------------------------------------------

void TRtfFile::FlushCurrentLine()
{
  if (mCurrentStrings.IsEmpty())
  {
    mCurrentStrings.Append(TString());
  }
  
  if (mCurrentString.Size())
  {
    if (mCurrentString.Last() != '\0')
    {
      mCurrentString.Append('\0');
    }

    if (mpLastStringEncoding)
    {
      // Convert back to a 8 byte string and Convert to unicode
      // with the given encoding...
      
      TArray<char> CharArray(mCurrentString.Size());
      
      for (int i = 0; i < mCurrentString.Size(); ++i)
      {
        CharArray[i] = (char)mCurrentString[i];
      }
      
      try
      {            
        const TString UniString = TString().SetFromCStringArray( 
          CharArray, mpLastStringEncoding);
        
        mCurrentStrings.Last().Append(UniString);
      }
      catch(const TReadableException&)
      {
        mCurrentStrings.Last().Append(mCurrentString.FirstRead());
      }
    }
    else
    {
      mCurrentStrings.Last().Append(mCurrentString.FirstRead());
    }
    
    mCurrentString.Empty();
    mCurrentStrings.Append(TString());
  }
  else
  {
    mCurrentStrings.Append(TString());
  }
}

// -------------------------------------------------------------------------------------------------

void TRtfFile::SParserOutput(
  void*           Context, 
  int              Char, 
  const char*     pStringEncoding)
{
  TRtfFile* pThis = (TRtfFile*)Context;
  
  if (Char == '\n')
  {
    pThis->FlushCurrentLine();
  }
  else
  {
    if (TString(pThis->mpLastStringEncoding) != TString(pStringEncoding))
    {
      pThis->FlushCurrentCharacters();
    }
    
    pThis->mpLastStringEncoding = pStringEncoding;

    const TUnicodeChar UniChar = (TUnicodeChar)Char;
    pThis->mCurrentString += (TUnicodeChar)UniChar;
  }
}

