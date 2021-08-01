#include "CoreTypesPrecompiledHeader.h"

#include <clocale> // LC_CTYPE

#if defined(MWindows)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h> // for ::GetLocalTime
#endif

#if defined(MMac)
  #include <CoreFoundation/CoreFoundation.h> // for the gLocalizedDateString stuff
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDayTime TDayTime::SCreateFromStatTime(time_t Time)
{
  tm LocalTime;
  if (::localtime_s(&LocalTime, &Time) == 0)
  {
    return TDayTime(LocalTime.tm_hour, LocalTime.tm_min, LocalTime.tm_sec);
  }
  else
  {
    MInvalid("::localtime failed");
    return TDayTime::SCurrentDayTime();
  }
}

// -------------------------------------------------------------------------------------------------

TDayTime TDayTime::SCurrentDayTime()
{
  #if defined(MMac) || defined(MLinux)
    time_t Time = ::time(NULL);
    tm LocalTime;
    if (::localtime_s(&LocalTime, &Time) == 0)
    {
      return TDayTime(LocalTime.tm_hour, LocalTime.tm_min, LocalTime.tm_sec);
    }
    else
    {
      MInvalid("::localtime failed");
      return TDayTime();
    } 
  
  #elif defined(MWindows)
    SYSTEMTIME Time;
    ::GetLocalTime(&Time);
    
    return TDayTime(Time.wHour, Time.wMinute, Time.wSecond);
  
  #else
    #error unknown platform
  #endif
}

// -------------------------------------------------------------------------------------------------

TDayTime::TDayTime()
  : mHours(0),
    mMinutes(0),
    mSeconds(0)
{
}

TDayTime::TDayTime(int Hours, int Minutes, int Seconds)
  : mHours(Hours),
    mMinutes(Minutes),
    mSeconds(Seconds)
{
  MAssert(Hours >= 0 && Hours <= 23, "");
  MAssert(Minutes >= 0 && Minutes <= 59, "");
  MAssert(Seconds >= 0 && Seconds <= 59, "");
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDate TDate::SCreateFromStatTime(time_t Time)
{
  tm LocalTime;
  if (::localtime_s(&LocalTime, &Time) == 0)
  {
    return TDate(LocalTime.tm_mday, LocalTime.tm_mon + 1, LocalTime.tm_year + 1900);
  }
  else
  {
    MInvalid("::localtime failed");
    return TDate::SCurrentDate();
  }
}

// -------------------------------------------------------------------------------------------------

TDate TDate::SCurrentDate()
{
  #if defined(MMac) || defined(MLinux)
    time_t Time = ::time(NULL);
    tm LocalTime;
    if (::localtime_s(&LocalTime, &Time) == 0)
    {
      return TDate(LocalTime.tm_mday, LocalTime.tm_mon + 1, LocalTime.tm_year + 1900);
    }
    else
    {
      MInvalid("::localtime failed");
      return TDate();
    }
    
  #elif defined(MWindows)
    SYSTEMTIME Time;
    ::GetLocalTime(&Time);
    
    return TDate(Time.wDay, Time.wMonth, Time.wYear);
  
  #else
    #error unknown platform
  #endif
}

// -------------------------------------------------------------------------------------------------

TDate::TDate()
 : mYear(0), mMonth(0), mDay(0)
{
}

TDate::TDate(int Day, int Month, int Year)
 : mYear(Year), mMonth(Month), mDay(Day)
{
  MAssert(Day >= 1 && Day <= 31, "");
  MAssert(Month >= 1 && Month <= 12, "");
  MAssert(Year >= 0, "");
}

// -------------------------------------------------------------------------------------------------

int TDate::TotalNumberOfDays()const 
{
  // Rata Die days (day one is 0001-01-01)
  int y = mYear;
  int m = mMonth;
  int d = mDay;

  if (m < 3) 
  {
    y--;
    m += 12;
  }

  return 365*y + y/4 - y/100 + y/400 + (153*m - 457)/5 + d - 306;
};

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool operator== (const TDayTime& First, const TDayTime& Second)
{
  return First.Seconds() == Second.Seconds() && 
    First.Minutes() == Second.Minutes() &&
    First.Hours() == Second.Hours();
}

// -------------------------------------------------------------------------------------------------

bool operator!= (const TDayTime& First, const TDayTime& Second)
{
  return !(First == Second);
}

// -------------------------------------------------------------------------------------------------

bool operator< (const TDayTime& First, const TDayTime& Second)
{
  if (First.Hours() < Second.Hours())
  {
    return true;
  }
  else if (First.Hours() == Second.Hours())
  {
    if (First.Minutes() < Second.Minutes())
    {
      return true;
    }
    else if (First.Minutes() == Second.Minutes())
    {
      return First.Seconds() < Second.Seconds();
    }
    else
    {
      return false;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool operator<= (const TDayTime& First, const TDayTime& Second)
{
  return operator<(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator> (const TDayTime& First, const TDayTime& Second)
{
  return !operator<(First, Second) && !operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator>= (const TDayTime& First, const TDayTime& Second)
{
  return operator>(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool StringToValue(TDayTime& Value, const TString &String)
{
  if (String.IsEmpty())
  {
    Value = TDayTime();
    return true;
  }

  const TList<TString> Splits = String.SplitAt(':');

  if (Splits.Size() != 3)
  {
    MInvalid("Invalid source string for a TDayTime");
    return false;
  }

  TDayTime Temp;
  
  if (!StringToValue(Temp.mHours, Splits[0]) || 
      !StringToValue(Temp.mMinutes, Splits[1]) ||
      !StringToValue(Temp.mSeconds, Splits[2]))
  {
    TMathT<int>::Clip(Temp.mHours, 0, 23);
    TMathT<int>::Clip(Temp.mMinutes, 0, 59);
    TMathT<int>::Clip(Temp.mSeconds, 0, 59);
    
    return false;
  }

  Value = Temp;
  return true;
}

// -------------------------------------------------------------------------------------------------

TString ToString(const TDayTime& Value)
{
  if (Value == TDayTime())
  {
    return TString();
  }
  else
  {
    return ToString(Value.Hours(), "%02d") + ":" + 
      ToString(Value.Minutes(), "%02d") + ":" + 
      ToString(Value.Seconds(), "%02d");
  }
}

// -------------------------------------------------------------------------------------------------

TString gLocalizedDateString(const TDate& Date)
{
  #if defined(MMac)
    // OSX's wcsftime does not use the locale settigns from the system 
    // preferences, so use core foundation for OSX instead:
    
    CFGregorianDate GregDate;
    GregDate.year = (SInt32)Date.Year();
    GregDate.month = (SInt8)Date.Month();
    GregDate.day = (SInt8)Date.Day();
    GregDate.hour = 0;
    GregDate.minute = 0;
    GregDate.second = 0.0;
    
    CFDateRef DateRef = ::CFDateCreate(kCFAllocatorDefault, 
      ::CFGregorianDateGetAbsoluteTime(GregDate, NULL));
    
    CFLocaleRef LocaleRef = ::CFLocaleCopyCurrent();
    
    CFDateFormatterRef DateFormatterRef = CFDateFormatterCreate(kCFAllocatorDefault, 
      LocaleRef, kCFDateFormatterMediumStyle, kCFDateFormatterNoStyle); 
    
    CFStringRef DateStringRef = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, 
      DateFormatterRef, DateRef);
    
    const TString Result = gCreateStringFromCFString(DateStringRef);
    
    ::CFRelease(DateStringRef);
    ::CFRelease(DateFormatterRef);
    ::CFRelease(LocaleRef);
    ::CFRelease(DateRef);
    
    return Result;

  #else // Linux or Windows
    const TSystem::TSetAndRestoreLocale SetLocale(LC_TIME, "");

    struct tm TimeStruct;
    TMemory::Zero(&TimeStruct, sizeof(TimeStruct));
    TimeStruct.tm_sec = 0;
    TimeStruct.tm_min = 0;
    TimeStruct.tm_hour = 0;
    TimeStruct.tm_mday = Date.Day();
    TimeStruct.tm_mon = Date.Month() - 1;
    TimeStruct.tm_year = Date.Year() - 1900;

    wchar_t DateStringBuffer[256] = {0};

    if (::wcsftime(DateStringBuffer, sizeof(DateStringBuffer) / 
          sizeof(wchar_t), L"%x", &TimeStruct) > 0)
    {
      return TString(DateStringBuffer);
    }
    else
    {
      MInvalid("wcsftime failed");
      return ToString(Date);
    }
    
  #endif
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

bool operator== (const TDate& First, const TDate& Second)
{
  return First.Year() == Second.Year() && 
    First.Month() == Second.Month() && 
    First.Day() == Second.Day();
}

// -------------------------------------------------------------------------------------------------

bool operator!= (const TDate& First, const TDate& Second)
{
  return !operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator< (const TDate& First, const TDate& Second)
{
  if (First.Year() < Second.Year())
  {
    return true;
  }
  else if (First.Year() == Second.Year())
  {
    if (First.Month() < Second.Month())
    {
      return true;
    }
    else if (First.Month() == Second.Month())
    {
      return First.Day() < Second.Day();
    }
    else
    {
      return false;
    }
  }

  return false;
}

// -------------------------------------------------------------------------------------------------

bool operator<= (const TDate& First, const TDate& Second)
{
  return operator<(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator> (const TDate& First, const TDate& Second)
{
  return !operator<(First, Second) && !operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool operator>= (const TDate& First, const TDate& Second)
{
  return operator>(First, Second) || operator==(First, Second);
}

// -------------------------------------------------------------------------------------------------

bool StringToValue(TDate& Value, const TString& String)
{
  if (String.IsEmpty())
  {
    Value = TDate();
    return true;
  }

  TList<TString> Splits = String.SplitAt('-');
  TDate Temp;
  
  if (Splits.Size() != 3)
  {
    // old '/' format...
    Splits = String.SplitAt('/');

    if (Splits.Size() != 3)
    {
      MInvalid("Invalid source string for a TDate");
      return false;
    }

    if (!StringToValue(Temp.mMonth, Splits[0]) || 
        !StringToValue(Temp.mDay, Splits[1]) ||
        !StringToValue(Temp.mYear, Splits[2]))
    {
      MInvalid("Invalid source string for a TDate");
      return false;
    }
  }
  else
  {
    // new '-' format
    if (!StringToValue(Temp.mYear, Splits[0]) || 
        !StringToValue(Temp.mMonth, Splits[1]) ||
        !StringToValue(Temp.mDay, Splits[2]))
    {
      MInvalid("Invalid source string for a TDate");
      return false;
    }
  }
  
  TMathT<int>::Clip(Temp.mMonth, 1, 12);
  TMathT<int>::Clip(Temp.mDay, 1, 31);
  TMathT<int>::Clip(Temp.mYear, 0, 3000);
  
  Value = Temp;
  return true;
}

// -------------------------------------------------------------------------------------------------

TString ToString(const TDate& Value)
{
  if (Value == TDate())
  {
    return TString();
  }
  else
  {
    return ToString(Value.Year(), "%04d") + "-" +
      ToString(Value.Month(), "%02d") + "-" + 
      ToString(Value.Day(), "%02d");
  }
}

