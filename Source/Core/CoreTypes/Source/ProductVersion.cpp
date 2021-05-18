#include "CoreTypesPrecompiledHeader.h"

#include "CoreTypes/Export/Version.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TProductVersion TProductVersion::SCurrentVersion()
{
  return TProductVersion(
    MMajorVersion, MMinorVersion, MRevisionVersion, 
    MAlphaOrBetaVersionString);
}

// -------------------------------------------------------------------------------------------------

TProductVersion::TProductVersion(
  int MajorVersion, 
  int MinorVersion, 
  int RevisionVersion, 
  const TString& AlphaBetaRcTag)
  : mMajorVersion(MajorVersion),
    mMinorVersion(MinorVersion),
    mRevisionVersion(RevisionVersion),
    mAlphaBetaRcTag(AlphaBetaRcTag)
{
  MAssert(MajorVersion >= 0 && MinorVersion >= 0 && RevisionVersion >= 0, 
    "Invalid version number");

  MAssert(AlphaBetaRcTag.IsEmpty() || 
      AlphaBetaRcTag.StartsWithCharIgnoreCase('a') || 
      AlphaBetaRcTag.StartsWithCharIgnoreCase('b') ||
      AlphaBetaRcTag.StartsWithIgnoreCase("rc"), 
    "Invalid AlphaBetaRcTag");
}

// -------------------------------------------------------------------------------------------------

int TProductVersion::MajorVersion()const
{
  return mMajorVersion;
}

// -------------------------------------------------------------------------------------------------

int TProductVersion::MinorVersion()const
{
  return mMinorVersion;
}

// -------------------------------------------------------------------------------------------------

int TProductVersion::RevisionVersion()const
{
  return mRevisionVersion;
}

// -------------------------------------------------------------------------------------------------

TString TProductVersion::AlphaBetaRcTag()const
{
  return mAlphaBetaRcTag;
}

// -------------------------------------------------------------------------------------------------

TProductVersion::TReleaseType TProductVersion::ReleaseType()const
{
  if (mAlphaBetaRcTag.StartsWithCharIgnoreCase('a'))
  {
    return kAlpha;
  }
  else if (mAlphaBetaRcTag.StartsWithCharIgnoreCase('b'))
  {
    return kBeta;
  }
  else if (mAlphaBetaRcTag.StartsWithIgnoreCase("rc"))
  {
    return kReleaseCandidate;
  }
  else
  {
    MAssert(mAlphaBetaRcTag.IsEmpty(), "Invalid tag");
    return kFinal;
  }
}

// -------------------------------------------------------------------------------------------------

int TProductVersion::ReleaseTypeVersion()const
{
  if (mAlphaBetaRcTag.StartsWithCharIgnoreCase('a'))
  {
    int Version = 0;
    if (::StringToValue(Version, TString(mAlphaBetaRcTag).RemoveFirst("a")))
    {
      return Version;
    }
    
    MInvalid("Unexpected version format");
    return 0;
  }
  else if (mAlphaBetaRcTag.StartsWithCharIgnoreCase('b'))
  {
    int Version = 0;
    if (::StringToValue(Version, TString(mAlphaBetaRcTag).RemoveFirst("b")))
    {
      return Version;
    }

    MInvalid("Unexpected version format");
    return 0;
  }
  else if (mAlphaBetaRcTag.StartsWithIgnoreCase("rc"))
  {
    int Version = 0;
    if (::StringToValue(Version, TString(mAlphaBetaRcTag).RemoveFirst("rc")))
    {
      return Version;
    }

    MInvalid("Unexpected version format");
    return 0;
  }
  else
  {
    MAssert(mAlphaBetaRcTag.IsEmpty(), "Invalid tag");
    return 0;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString ToString(const TProductVersion& Version)
{
  if (Version == TProductVersion())
  {
    return TString();
  }
  else
  {
    TString Ret;

    Ret = "V" + ::ToString(Version.MajorVersion()) + "." + 
      ::ToString(Version.MinorVersion()) + "." +
      ::ToString(Version.RevisionVersion());

    if (! Version.AlphaBetaRcTag().IsEmpty())
    {
      Ret += " " + Version.AlphaBetaRcTag();
    }

    return Ret;
  }
}

// -------------------------------------------------------------------------------------------------

bool StringToValue(TProductVersion& Version, const TString& String)
{
  if (String.IsEmpty())
  {
    Version = TProductVersion();
    return true;
  }
  else
  {
    TString RemainingString(String);
    RemainingString.Strip();

    int Major = 0, Minor = 0, Revision = 0;
    TString AlphaBetaRcTag;
  
    // extract alpha beta tag
    const int Backwards = true;
    int SpacePos = RemainingString.Find(' ', String.Size() - 1, Backwards);
    if (SpacePos != -1)
    {
      AlphaBetaRcTag = RemainingString.SubString(SpacePos + 1);
      RemainingString = RemainingString.SubString(0, SpacePos);
    }

    // extract version numbers
    const TList<TString> VersionSplit = 
      TString(RemainingString).RemoveFirst("V").SplitAt('.');
  
    if (VersionSplit.Size() != 3)
    {
      return false;
    }

    if (!::StringToValue(Major, VersionSplit[0]) ||
        !::StringToValue(Minor, VersionSplit[1]) ||
        !::StringToValue(Revision, VersionSplit[2]))
    {
      return false;
    }

    Version = TProductVersion(Major, Minor, Revision, AlphaBetaRcTag);
    return true;
  }
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool operator==(const TProductVersion& First, const TProductVersion& Second)
{
  return 
    First.MajorVersion() == Second.MajorVersion() && 
    First.MinorVersion() == Second.MinorVersion() && 
    First.RevisionVersion() == Second.RevisionVersion() && 
    First.AlphaBetaRcTag() == Second.AlphaBetaRcTag();
}

// -------------------------------------------------------------------------------------------------

bool operator!=(const TProductVersion& First, const TProductVersion& Second)
{
  return !operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator>=(const TProductVersion& First, const TProductVersion& Second)
{
  return operator>(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator>(const TProductVersion& First, const TProductVersion& Second)
{
  if (First.MajorVersion() == Second.MajorVersion())
  {
    if (First.MinorVersion() == Second.MinorVersion())
    {
      if (First.RevisionVersion() == Second.RevisionVersion())
      {
        if ((int)First.ReleaseType() == (int)Second.ReleaseType())
        {
          return ((int)First.ReleaseTypeVersion() > (int)Second.ReleaseTypeVersion());
        }
        else
        {
          return ((int)First.ReleaseType() > (int)Second.ReleaseType());
        }
      }
      else
      {
        return (First.RevisionVersion() > Second.RevisionVersion());
      }
    }
    else 
    {
      return (First.MinorVersion() > Second.MinorVersion());
    }
  }
  else
  {
    return (First.MajorVersion() > Second.MajorVersion());
  }
}

// -------------------------------------------------------------------------------------------------

bool operator<=(const TProductVersion& First, const TProductVersion& Second)
{
  return operator<(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator<(const TProductVersion& First, const TProductVersion& Second)
{
  return !operator>=(First, Second);
}

