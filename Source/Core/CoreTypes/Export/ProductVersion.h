#ifndef _ProductVersion_h_
#define _ProductVersion_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"

// =================================================================================================

/*!
 * Helper class to deal with product version numbers.
!*/

class TProductVersion
{
public:
  //! Current version as defined by the Version.h macros.
  static TProductVersion SCurrentVersion();

  enum TReleaseType
  {
    kAlpha,
    kBeta,
    kReleaseCandidate,
    kFinal,
  };

  //! \param AlphaBetaRcTag: aX, bX, rcX or empty with X >= 0.
  //! When empty, product version is "final".
  TProductVersion(
    int             MajorVersion = 0, 
    int             MinorVersion = 0, 
    int             RevisionVersion = 0, 
    const TString&  AlphaBetaRcTag = TString()); 

  int MajorVersion()const;
  int MinorVersion()const;
  int RevisionVersion()const;

  TString AlphaBetaRcTag()const;

  TReleaseType ReleaseType()const;
  int ReleaseTypeVersion()const;

private:
  int mMajorVersion; 
  int mMinorVersion;
  int mRevisionVersion;
  TString mAlphaBetaRcTag;
};

// =================================================================================================

//! global TProductVersion serialization functions.
TString ToString(const TProductVersion& Version);
bool StringToValue(TProductVersion& Version, const TString &String);

//! global TProductVersion comparison operators
bool operator==(const TProductVersion& First, const TProductVersion& Second);
bool operator!=(const TProductVersion& First, const TProductVersion& Second);

bool operator>=(const TProductVersion& First, const TProductVersion& Second);
bool operator>(const TProductVersion& First, const TProductVersion& Second);

bool operator<=(const TProductVersion& First, const TProductVersion& Second);
bool operator<(const TProductVersion& First, const TProductVersion& Second);


#endif // _ProductVersion_h_

