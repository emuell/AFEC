#pragma once

#ifndef _ValueChanger_h_
#define _ValueChanger_h_

// =================================================================================================
 
/*! 
 * Set of classes for synced exception-safe changes to values
!*/

// =================================================================================================

/*!
 * Increases a value on construction and decreases it on destruction
!*/

template <typename T> 
class TIncDecValue
{
public:
  TIncDecValue(T& Value)
    : mValueRef(Value)
  {
    ++mValueRef;
  }
  
  ~TIncDecValue()
  {
    --mValueRef;
  }

private:
  T& mValueRef; 
};

// =================================================================================================

/*!
 * Decreases a value on construction and increases it on destruction
!*/

template <typename T> 
class TDecIncValue
{
public:
  TDecIncValue(T& Value)
    : mValueRef(Value)
  {
    --mValueRef;
  }
  
  ~TDecIncValue()
  {
    ++mValueRef;
  }

private:
  T& mValueRef; 
};

// =================================================================================================

/*!
 * Applies a new value when constructed, and restores to the previous 
 * value on destruction
!*/

template <typename T> 
class TSetUnsetValue
{
public:
  TSetUnsetValue(T& Value, const T& NewValue)
    : mValueRef(Value), mOldValue(Value)
  {
    mValueRef = NewValue;
  }
  
  ~TSetUnsetValue()
  {
    mValueRef = mOldValue;
  }

private:
  T& mValueRef; 
  const T mOldValue;
};

// =================================================================================================

/*!
 * Restores a value on destruction, to the previous value (value 
   it got constructed with)
!*/

template <typename T> 
class TUnsetValue
{
public:
  TUnsetValue(T& Value)
    : mValueRef(Value), mInitialValue(Value)
  {
  }
  
  ~TUnsetValue()
  {
    mValueRef = mInitialValue;
  }

private:
  T& mValueRef; 
  const T mInitialValue;
};

 
#endif // _ValueChanger_h_

