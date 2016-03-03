// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/program_options.h"

#include "base/base.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace base {

using namespace std;

struct same_name {
  const string& name;
  same_name(const string& name) : name(name) { }
  bool operator()(const ProgramOptions::Option* a) {
    return (a->name() == name ||
            a->alias() == name);
  }
};

struct same_mnemonic {
  char mnemonic;
  same_mnemonic(char mnemonic) : mnemonic(mnemonic) { }
  bool operator()(const ProgramOptions::Option* a) {
    return a->mnemonic() == mnemonic;
  }
};

ProgramOptions::ProgramOptions()
{
}

ProgramOptions::~ProgramOptions()
{
  for (OptionList::const_iterator
         it=m_options.begin(), end=m_options.end(); it != end; ++it)
    delete *it;
}

ProgramOptions::Option& ProgramOptions::add(const string& name)
{
  Option* option = new Option(name);
  m_options.push_back(option);
  return *option;
}

void ProgramOptions::parse(int argc, const char* argv[])
{
  for (int i=1; i<argc; ++i) {
    string arg(argv[i]);

    // n = number of dashes ('-') at the beginning of the argument.
    size_t n = 0;
    for (; arg[n] == '-'; ++n)
      ;
    size_t len = arg.size()-n;

    // Ignore process serial number argument (-psn...) when the app is run from command line
#if __APPLE__
    if (arg.size() >= 4 && arg.substr(0, 4) == "-psn")
      continue;
#endif

    if ((n > 0) && (len > 0)) {
      // Use mnemonics
      if (n == 1) {
        char usedBy = 0;

        for (size_t j=1; j<arg.size(); ++j) {
          OptionList::iterator it =
            find_if(m_options.begin(), m_options.end(), same_mnemonic(arg[j]));

          if (it == m_options.end()) {
            stringstream msg;
            msg << "Invalid option '-" << arg[j] << "'";
            throw InvalidProgramOption(msg.str());
          }

          Option* option = *it;
          std::string optionValue;

          if (option->doesRequireValue()) {
            if (usedBy != 0) {
              stringstream msg;
              msg << "You cannot use '-" << option->mnemonic()
                  << "' and '-" << usedBy << "' "
                  << "together, both options need one extra argument";
              throw InvalidProgramOptionsCombination(msg.str());
            }

            if (i+1 >= argc) {
              stringstream msg;
              msg << "Option '-" << option->mnemonic()
                  << "' needs one extra argument";
              throw ProgramOptionNeedsValue(msg.str());
            }

            // Set the value specified for this argument
            optionValue = argv[++i];
            usedBy = option->mnemonic();
          }

          m_values.push_back(Value(option, optionValue));
        }
      }
      // Use name
      else {
        string optionName;
        string optionValue;
        size_t equalSignPos = arg.find('=', n);

        if (equalSignPos != string::npos) {
          optionName = arg.substr(n, equalSignPos-n);
          optionValue = arg.substr(equalSignPos+1);
        }
        else {
          optionName = arg.substr(n);
        }

        OptionList::iterator it =
          find_if(m_options.begin(), m_options.end(), same_name(optionName));

        if (it == m_options.end()) {
          stringstream msg;
          msg << "Invalid option '--" << optionName << "'";
          throw InvalidProgramOption(msg.str());
        }

        Option* option = *it;

        if (option->doesRequireValue()) {
          // If the option was specified without '=', we can get the
          // value from the next argument.
          if (equalSignPos == string::npos) {
            if (i+1 >= argc) {
              stringstream msg;
              msg << "Missing value in '--" << optionName
                  << "=" << option->getValueName() << "' option specification";
              throw ProgramOptionNeedsValue(msg.str());
            }
            optionValue = argv[++i];
          }
        }

        m_values.push_back(Value(option, optionValue));
      }
    }
    // Add values without a related option.
    else {
      m_values.push_back(Value(NULL, arg));
    }
  }
}

void ProgramOptions::reset()
{
  m_values.clear();
}

bool ProgramOptions::enabled(const Option& option) const
{
  for (const auto& value : m_values) {
    if (value.option() == &option)
      return true;
  }
  return false;
}

std::string ProgramOptions::value_of(const Option& option) const
{
  for (const auto& value : m_values) {
    if (value.option() == &option)
      return value.value();
  }
  return "";
}

} // namespace base

std::ostream& operator<<(std::ostream& os, const base::ProgramOptions& po)
{
  std::size_t maxOptionWidth = 0;
  for (base::ProgramOptions::OptionList::const_iterator
         it=po.options().begin(), end=po.options().end(); it != end; ++it) {
    const base::ProgramOptions::Option* option = *it;
    std::size_t optionWidth =
      6+MAX(option->name().size(), option->alias().size())+1+
      (option->doesRequireValue() ? option->getValueName().size()+1: 0);

    if (maxOptionWidth < optionWidth)
      maxOptionWidth = optionWidth;
  }

  for (base::ProgramOptions::OptionList::const_iterator
         it=po.options().begin(), end=po.options().end(); it != end; ++it) {
    const base::ProgramOptions::Option* option = *it;
    std::size_t optionWidth = 6+option->name().size()+1+
      (option->doesRequireValue() ? option->getValueName().size()+1: 0);

    if (option->mnemonic() != 0)
      os << std::setw(3) << '-' << option->mnemonic() << ", ";
    else
      os << std::setw(6) << ' ';
    os << "--" << option->name();
    if (option->doesRequireValue())
      os << " " << option->getValueName();

    // Show alias
    if (!option->alias().empty()) {
      os << " or\n"
         << std::setw(6) << ' '
         << "--" << option->alias();
      if (option->doesRequireValue())
        os << " " << option->getValueName();

      optionWidth = 6+option->alias().size()+1+
        (option->doesRequireValue() ? option->getValueName().size()+1: 0);
    }

    if (!option->description().empty()) {
      bool multilines = (option->description().find('\n') != std::string::npos);

      if (!multilines) {
        os << std::setw(maxOptionWidth - optionWidth + 1) << ' ' << option->description()
           << "\n";
      }
      else {
        std::istringstream s(option->description());
        std::string line;
        if (std::getline(s, line)) {
          os << std::setw(maxOptionWidth - optionWidth + 1) << ' ' << line << '\n';
          while (std::getline(s, line)) {
            os << std::setw(maxOptionWidth+2) << ' ' << line << '\n';
          }
        }
      }
    }
    else
      os << "\n";
  }

  return os;
}
