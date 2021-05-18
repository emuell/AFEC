#pragma once

#ifndef _DateAndTime_h_
#define _DateAndTime_h_

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/BaseTypes.h"

#include <ctime> // time_t

// =================================================================================================

class TDayTime
{
public:
  //! Return the current local time (NOT gmtime)
  static TDayTime SCurrentDayTime();
  
  //! Create a TDayTime from the given binary stat time, using the systems 
  //! local time settings (time zone and no daylight adjusted).
  static TDayTime SCreateFromStatTime(time_t Time);

  TDayTime();
  TDayTime(int Hours, int Minutes, int Seconds);

  //! Return hours from 0 - 24
  int Hours()const;
  //! Return minutes from 0 - 60
  int Minutes()const;
  //! Return seconds from 0 - 60
  int Seconds()const;

private:
  friend bool StringToValue(TDayTime& Value, const TString &String);
  
  int mHours;
  int mMinutes;
  int mSeconds;
};

// =================================================================================================

class TDate
{
public:
  //! Return the current local date (NOT gmtime)
  static TDate SCurrentDate();

  //! Create a TDate from the given binary stat time, using the systems 
  //! local time settings (time zone and no daylight adjusted).
  static TDate SCreateFromStatTime(time_t Time);

  TDate();
  TDate(int Day, int Month, int Year);

  //! Return the day from 1 - 31
  int Day()const;
  //! Return the month from 1 - 12
  int Month()const;
  //! Return the year from 1900 - now
  int Year()const;

  //! Total number of days (day one is 0001-01-01)
  int TotalNumberOfDays()const;
  
private:
  friend bool StringToValue(TDate& Value, const TString &String);
  
  int mYear;
  int mMonth;
  int mDay;
};

// =================================================================================================

bool operator== (const TDayTime& First, const TDayTime& Second);
bool operator!= (const TDayTime& First, const TDayTime& Second);

bool operator< (const TDayTime& First, const TDayTime& Second);
bool operator<= (const TDayTime& First, const TDayTime& Second);
bool operator> (const TDayTime& First, const TDayTime& Second);
bool operator>= (const TDayTime& First, const TDayTime& Second);

bool StringToValue(TDayTime& Value, const TString &String);
TString ToString(const TDayTime& Value);

// =================================================================================================

bool operator== (const TDate& First, const TDate& Second);
bool operator!= (const TDate& First, const TDate& Second);

bool operator< (const TDate& First, const TDate& Second);
bool operator<= (const TDate& First, const TDate& Second);
bool operator> (const TDate& First, const TDate& Second);
bool operator>= (const TDate& First, const TDate& Second);

// =================================================================================================

bool StringToValue(TDate& Value, const TString &String);
TString ToString(const TDate& Value);

//! ToString with the current locale
TString gLocalizedDateString(const TDate& Value);

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline int TDayTime::Hours()const 
{ 
  return mHours; 
}

// -------------------------------------------------------------------------------------------------

inline int TDayTime::Minutes()const 
{ 
  return mMinutes; 
}

// -------------------------------------------------------------------------------------------------

inline int TDayTime::Seconds()const 
{ 
  return mSeconds; 
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

inline int TDate::Day()const 
{ 
  return mDay; 
}

// -------------------------------------------------------------------------------------------------

inline int TDate::Month()const 
{ 
  return mMonth; 
}

// -------------------------------------------------------------------------------------------------

inline int TDate::Year()const 
{ 
  return mYear; 
}


#endif // _DateAndTime_h_

