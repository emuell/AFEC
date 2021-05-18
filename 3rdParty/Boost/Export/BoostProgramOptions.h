#pragma once

#ifndef _BoostProgramOptions_h_
#define _BoostProgramOptions_h_

// =================================================================================================

#if defined(MCompiler_VisualCPP)
  #pragma MAddLibrary("BoostProgramOptions")
  #define BOOST_ALL_NO_LIB
#endif

// =================================================================================================

#define BOOST_TEST_NO_MAIN
#include <boost/program_options.hpp>

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/List.h"
#include "CoreTypes/Export/Array.h"

// -------------------------------------------------------------------------------------------------

// create a TString compatible boost command line parser instance

static inline boost::program_options::command_line_parser 
  CreateBoostCommandLineParser(const TList<TString>& Arguments)
{
  // convert TStrings to UTF8
  // HACK: static - to make sure they are valid after this function exists...
  static std::vector<const char*> ppArgs(Arguments.Size());
  static std::vector<std::string> ArgumentCStrings(Arguments.Size());
  for (int i = 0; i < Arguments.Size(); ++i)
  {
    ArgumentCStrings[i] = Arguments[i].CString(TString::kUtf8);
    ppArgs[i] = ArgumentCStrings[i].c_str();
  }

  // create a cstring command_line_parser
  return boost::program_options::command_line_parser(
    (int)ppArgs.size(), ppArgs.data());
}

// -------------------------------------------------------------------------------------------------

// convert a boost command line string vector argument to TStrings

static inline TString ArgumentToString(
  const boost::program_options::variable_value& Argument)
{
  // fetch argument and convert UTF8 to TStrings
  return TString(Argument.as<std::string>().c_str(), TString::kUtf8);
}

// -------------------------------------------------------------------------------------------------

// convert a boost command line string vector argument to TStrings

static inline TList<TString> ArgumentToStringList(
  const boost::program_options::variable_value& Argument)
{
  // fetch vector argument
  const std::vector<std::string> CStrings = Argument.as<std::vector<std::string>>();

  // convert UTF8 to TStrings
  TList<TString> Ret;
  Ret.PreallocateSpace((int)CStrings.size());
  for (const std::string CString : CStrings)
  {
    Ret.Append(TString(CString.c_str(), TString::kUtf8));
  }

  return Ret;
}


#endif // _BoostProgramOptions_h_

