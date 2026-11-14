/*
 * Runtime support for the "<module>CLP.h" headers written by dcmqiGenerateCLP.
 *
 * Each dcmqi command-line application includes its generated header and calls
 * PARSE_ARGS at the top of main(). The generated code declares the TCLAP
 * arguments of the module; the helpers below hold the logic that is the same
 * for every module.
 */

#ifndef dcmqiCLP_h
#define dcmqiCLP_h

// TCLAP picks std::strstream over std::stringstream when HAVE_STRSTREAM is
// defined and HAVE_SSTREAM is not. DCMTK's configuration header defines both,
// and the applications undefine HAVE_SSTREAM before including this header.
// Hide both macros from TCLAP so that it always uses <sstream>, and restore
// them for the code that follows.
#pragma push_macro("HAVE_SSTREAM")
#pragma push_macro("HAVE_STRSTREAM")
#undef HAVE_SSTREAM
#undef HAVE_STRSTREAM
#include <tclap/CmdLine.h>
#include <tclap/StdOutput.h>
#include <tclap/SwitchArg.h>
#include <tclap/ValueArg.h>
#include <tclap/ValuesConstraint.h>
#pragma pop_macro("HAVE_STRSTREAM")
#pragma pop_macro("HAVE_SSTREAM")

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// dcmqiMacroBuildCLI compiles each application into "<module>Lib" with main
// renamed ModuleEntryPoint, and the executable's main() calls it. This
// declaration precedes the application's main(), so ModuleEntryPoint gets C
// linkage and its symbol name does not depend on how the application declares
// argv: MSVC decorates "char* argv[]" and "char** argv" differently, although
// they are the same type.
extern "C" int ModuleEntryPoint(int argc, char* argv[]);

namespace dcmqiCLP
{

/// Concatenates the lines of an embedded text.
inline std::string JoinLines(const char* const lines[], std::size_t count)
{
  std::string text;
  for (std::size_t i = 0; i < count; ++i)
  {
    text += lines[i];
  }
  return text;
}

/// Splits a comma-separated list value. Empty items are dropped and the
/// items are not trimmed: ",a,,b," gives {"a", "b"} and "a b, c" gives
/// {"a b", " c"}.
inline std::vector<std::string> SplitList(const std::string& value)
{
  std::vector<std::string> items;
  std::string::size_type start = 0;
  while (start < value.size())
  {
    std::string::size_type end = value.find(',', start);
    if (end == std::string::npos)
    {
      end = value.size();
    }
    if (end > start)
    {
      items.push_back(value.substr(start, end - start));
    }
    start = end + 1;
  }
  return items;
}

/// Rewrites argv into the token list handed to TCLAP::CmdLine::parse().
///
/// dcmqi applications have always accepted command lines that plain TCLAP
/// would reject, and scripts depend on that. The rules, applied left to right
/// to argv[1..]:
///
/// 1. A token equal to one of 'valueFlags' (e.g. "--inputMetadata" or "-t")
///    takes the next token as its value, whatever that token looks like. When
///    there is no next token, the value is the empty string.
/// 2. Any other token that starts with a single '-' and is longer than two
///    characters is a cluster of short flags: "-abc" becomes "-a" "-b" "-c".
///    These flags do not take a value here.
/// 3. Each flag is kept only at its first occurrence. When a value flag is
///    repeated, its last value replaces the token that follows the first
///    occurrence; if the first occurrence is the last token so far, the
///    repeated value is dropped. Repeated flags without a value are dropped.
/// 4. Everything else ("--", "-", "", and words) is passed through unchanged.
inline std::vector<std::string> NormalizeArguments(int argc, char* argv[],
                                                   const std::set<std::string>& valueFlags)
{
  std::vector<std::string> tokens;
  tokens.push_back(argc > 0 && argv[0] ? argv[0] : "");

  // Position in 'tokens' of the first occurrence of each flag.
  std::map<std::string, std::size_t> firstPosition;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];

    if (valueFlags.count(arg))
    {
      const std::string value = (i + 1 < argc) ? std::string(argv[++i]) : std::string();
      const std::map<std::string, std::size_t>::const_iterator seen = firstPosition.find(arg);
      if (seen == firstPosition.end())
      {
        firstPosition[arg] = tokens.size();
        tokens.push_back(arg);
        tokens.push_back(value);
      }
      else if (seen->second + 1 < tokens.size())
      {
        tokens[seen->second + 1] = value;
      }
      continue;
    }

    std::vector<std::string> flags;
    if (arg.size() > 2 && arg[0] == '-' && arg[1] != '-')
    {
      for (std::string::size_type k = 1; k < arg.size(); ++k)
      {
        flags.push_back(std::string("-") + arg[k]);
      }
    }
    else if (arg.size() > 1 && arg[0] == '-' && arg != "--")
    {
      flags.push_back(arg);
    }
    else
    {
      tokens.push_back(arg);
      continue;
    }

    for (std::size_t k = 0; k < flags.size(); ++k)
    {
      if (!firstPosition.count(flags[k]))
      {
        firstPosition[flags[k]] = tokens.size();
        tokens.push_back(flags[k]);
      }
    }
  }
  return tokens;
}

/// TCLAP's standard output, except that the short usage lists the program
/// and then each argument on a line of its own, as dcmqi applications always
/// have. The generated code declares no exclusive ("xor") argument groups.
class UsageOutput : public TCLAP::StdOutput
{
public:
  void usage(TCLAP::CmdLineInterface& commandLine) override
  {
    std::cout << std::endl << "USAGE: " << std::endl << std::endl;
    this->ShortUsage(commandLine, std::cout);
    std::cout << std::endl << std::endl << "Where: " << std::endl << std::endl;
    this->_longUsage(commandLine, std::cout);
    std::cout << std::endl;
  }

  void failure(TCLAP::CmdLineInterface& commandLine, TCLAP::ArgException& e) override
  {
    std::cerr << "PARSE ERROR: " << e.argId() << std::endl
              << "             " << e.error() << std::endl
              << std::endl;
    if (commandLine.hasHelpAndVersion())
    {
      std::cerr << "Brief USAGE: " << std::endl;
      this->ShortUsage(commandLine, std::cerr);
      std::cerr << std::endl
                << "For complete USAGE and HELP type: " << std::endl
                << "   " << commandLine.getProgramName() << " " << TCLAP::Arg::nameStartString() << "help"
                << std::endl
                << std::endl;
    }
    else
    {
      this->usage(commandLine);
    }
    throw TCLAP::ExitException(1);
  }

private:
  void ShortUsage(TCLAP::CmdLineInterface& commandLine, std::ostream& os) const
  {
    this->spacePrint(os, commandLine.getProgramName(), 75, 2, 0);
    const std::list<TCLAP::Arg*>& args = commandLine.getArgList();
    for (std::list<TCLAP::Arg*>::const_iterator it = args.begin(); it != args.end(); ++it)
    {
      this->spacePrint(os, (*it)->shortID(), 75, 5, 0);
    }
  }
};

/// Arguments every dcmqi application accepts in addition to those of its XML
/// descriptor. Declared after the module arguments so that they are listed
/// first in the usage message.
struct StandardArguments
{
  UsageOutput Output;
  TCLAP::SwitchArg Echo;
  TCLAP::SwitchArg XML;
  TCLAP::ValueArg<std::string> ProcessInformationAddress;
  TCLAP::ValueArg<std::string> ReturnParameterFile;

  explicit StandardArguments(TCLAP::CmdLine& commandLine)
    : Echo("", "echo", "Echo the command line arguments (value: 0)", commandLine, false)
    , XML("", "xml", "Produce xml description of command line arguments (value: 0)", commandLine, false)
    , ProcessInformationAddress("", "processinformationaddress",
        "Address of a structure to store process information (progress, abort, etc.). (value: 0)",
        false, "0", "std::string", commandLine)
    , ReturnParameterFile("", "returnparameterfile",
        "Filename in which to write simple return parameters (int, float, int-vector, etc.) "
        "as opposed to bulk return parameters (image, geometry, transform, measurement, table).",
        false, "", "std::string", commandLine)
  {
    commandLine.setOutput(&this->Output);
  }

  /// Flags of the standard arguments that take a value.
  static void AddValueFlags(std::set<std::string>& valueFlags)
  {
    valueFlags.insert("--processinformationaddress");
    valueFlags.insert("--returnparameterfile");
  }
};

/// Prints one line of the --echo output.
template <typename T>
inline void EchoValue(const char* name, const T& value)
{
  std::cout << "    " << name << ": " << value << std::endl;
}

inline void EchoValue(const char* name, const std::vector<std::string>& values)
{
  std::cout << "    " << name << ": ";
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    std::cout << values[i] << ", ";
  }
  std::cout << std::endl;
}

} // namespace dcmqiCLP

#endif
