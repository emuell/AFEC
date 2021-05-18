#include "CoreTypesPrecompiledHeader.h"

#include <limits>

// =================================================================================================

//! sine approximation via a fourth-order cosine approx. 

namespace TFastTrigonometry
{
  void Initialize();

  enum { 
    kMSBits       = 10,
    kLSBits       = 10,
    kMsTableSize  = 1 << kMSBits,
    kLsTableSize  = 1 << kLSBits,
    kBits         = kMSBits + kLSBits,
    kPowBits      = 8,
  };
  
  enum { 
    kMaxValidIndex = (2 << 20),
  };

  // Returns sin with 20 bits of precision.
  double SinQ(int Index);
  double CosQ(int Index);
  double TanQ(int Index); 

  struct TSinCos 
  { 
    float mSin;
    float mCos;
  };

  TSinCos sMsBitsTable[kMsTableSize + 1];
  TSinCos sLsBitsTable[kMsTableSize + 1];
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TFastTrigonometry::Initialize()
{
  for (int i = 0; i <= kMsTableSize; ++i) 
  {
    double phi = (M2Pi*i/kMsTableSize);
    sMsBitsTable[i].mSin = (float)sin(phi);
    sMsBitsTable[i].mCos = (float)cos(phi);
  }

  for (int i = 0; i <= kLsTableSize; ++i) 
  {
    double phi = (M2Pi*i/(kMsTableSize*1.0*kLsTableSize));
    sLsBitsTable[i].mSin = (float)::sin(phi);
    sLsBitsTable[i].mCos = (float)::cos(phi);
  }
}

// -------------------------------------------------------------------------------------------------

MForceInline double TFastTrigonometry::SinQ(int Index)
{
  // Based on the identity sin(u+v) = sinu cosv + cosu sinv
  TSinCos *pscu = sMsBitsTable +( (Index >> kLSBits) & (kMsTableSize-1));
  TSinCos *pscv = sLsBitsTable + ( (Index) & (kLsTableSize-1));

  return pscu->mSin * pscv->mCos + pscu->mCos * pscv->mSin;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TFastTrigonometry::CosQ(int Index)
{
  // based on the identity cos(u+v) = cosu cosv + sinu sinv
  TSinCos *pscu = sMsBitsTable +( (Index >> kLSBits) & (kMsTableSize-1));
  TSinCos *pscv = sLsBitsTable + ( (Index) & (kLsTableSize-1));

  return pscu->mCos * pscv->mCos - pscu->mSin * pscv->mSin;
}

// -------------------------------------------------------------------------------------------------

MForceInline double TFastTrigonometry::TanQ(int Index)  
{
  MAssert(Index >= 0 && Index < kMaxValidIndex, "");

  // based on the identity cos(u+v) = cosu cosv + sinu sinv
  TSinCos *pscu = sMsBitsTable +( (Index >> kLSBits) & (kMsTableSize-1));
  TSinCos *pscv = sLsBitsTable + ( (Index) & (kLsTableSize-1));

  const double dCos = pscu->mCos * pscv->mCos - pscu->mSin * pscv->mSin;
  const double dSin = pscu->mSin * pscv->mCos + pscu->mCos * pscv->mSin;

  return (dCos != 0.0) ? 
    dSin / dCos : 
    std::numeric_limits<double>::quiet_NaN();
}

// =================================================================================================

int TMath::sRandState = 0x16BA2118;

// -------------------------------------------------------------------------------------------------

void TMath::Init()
{
  TFastTrigonometry::Initialize();
}

// -------------------------------------------------------------------------------------------------

double TMath::FastSin(double dAngle)
{
  return TFastTrigonometry::SinQ(TMath::d2i(dAngle * 
    (TFastTrigonometry::kMsTableSize * TFastTrigonometry::kLsTableSize / M2Pi)));
}

float TMath::FastSin(float x)
{
  return (float)FastSin((double)x);
}

// -------------------------------------------------------------------------------------------------

double TMath::FastCos(double dAngle)
{
  return TFastTrigonometry::CosQ(TMath::d2i(dAngle * 
    (TFastTrigonometry::kMsTableSize * TFastTrigonometry::kLsTableSize / M2Pi)));
}

float TMath::FastCos(float x)
{
  return (float)FastCos((double)x);
}

// -------------------------------------------------------------------------------------------------

double TMath::FastTan(double dAngle)
{
  return TFastTrigonometry::TanQ(TMath::d2i(dAngle * 
    (TFastTrigonometry::kMsTableSize * TFastTrigonometry::kLsTableSize / M2Pi)));
}

float TMath::FastTan(float x)
{
  return (float)FastTan((double)x);
}

