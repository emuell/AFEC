#ifdef __Version_h__
  #error "please do not include Version.h in headers"
#endif

#define __Version_h__

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/DateAndTime.h"

// =================================================================================================

//! make sure that this header is included, via "MStaticAssert(MVersionIncluded);"
#define MVersionIncluded 1

// =================================================================================================

/*!
 * Defines a product's (executable or dll) current version and version 
 * related info. See also \class TProductVersion.
!*/

#define MProductString TProductDescription::ProductName()
#define MProductVendorString TProductDescription::ProductVendorName()
#define MProductProjectsLocation TProductDescription::ProductProjectsLocation()

#define MMajorVersion TProductDescription::MajorVersion()
#define MMinorVersion TProductDescription::MinorVersion()
#define MRevisionVersion TProductDescription::RevisionVersion()

#define MVersionString (TString("V") + ToString(MMajorVersion) + "." + \
  ToString(MMinorVersion) + "." + ToString(MRevisionVersion))

#define MAlphaOrBetaVersionString TProductDescription::AlphaOrBetaVersionString()

#define MIsAlphaOrBetaBuild !TProductDescription::AlphaOrBetaVersionString().IsEmpty()

#define MBuildNumberString (TProductDescription::AlphaOrBetaVersionString().IsEmpty() ? \
  MVersionString + " (Built: " + __DATE__ + ")" : \
  MVersionString + " " + TProductDescription::AlphaOrBetaVersionString() + " (Built: " + __DATE__ + ")")
  
#define MProductNameAndVersionString (TProductDescription::ProductName() + \
  " " + MVersionString)

#define MExpirationDate TProductDescription::ExpirationDate()

#define MSupportEMailAddress TProductDescription::SupportEMailAddress()
#define MProductHomeURL TProductDescription::ProductHomeURL()
#define MCopyrightString TProductDescription::CopyrightString()

// =================================================================================================

/*!
 * Version symbols which are externally defined by every application. 
 * Do not use them directly! Use the above defined macros instead.
!*/

namespace TProductDescription 
{ 
  //! Full name of the product
  extern TString ProductName();
  //! Full name of the product's vendor
  extern TString ProductVendorName();
  
  //! Used in developer environments only. Vendor + product path of the 
  //! product, i.e: Crawler/XCrawler or Framework/XUnitTests
  extern TString ProductProjectsLocation();

  //! Major.Minor.Revision
  extern int MajorVersion();
  extern int MinorVersion();
  extern int RevisionVersion();

  //! for example "a0", "b1" or nothing if its a final release
  extern TString AlphaOrBetaVersionString();
  
  //! only valid when 'AlphaOrBetaVersionString' is set
  extern TDate ExpirationDate();
  
  extern TString SupportEMailAddress();
  extern TString ProductHomeURL();

  extern TString CopyrightString();
}

// =================================================================================================

/*!
 * Compile demo or full versions
!*/

//! demo version 
#define kEvaluationVersion 0
//! version with all features and copy protection
#define kFullVersion 1

//! define which version we want to build (changed by the build scripts)
#define MVersion kFullVersion

// =================================================================================================

/*!
 * Compile debugging features
!*/

//! define to include some extra debugging features
#define MEnableDebuggingTools

