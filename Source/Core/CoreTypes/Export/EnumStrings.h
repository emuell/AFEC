#pragma once

#ifndef _EnumStrings_h_
#define _EnumStrings_h_

// =================================================================================================

/*! 
 * Generic enum type -> string conversion. 
 * To use it, provide a SEnumStrings specialization for your type, in order to 
 * define serialized values for your enumeration type.
!*/

template <typename TEnumType>
struct SEnumStrings
{
  operator TList<TString>()const;
};


#endif // _EnumStrings_h_

