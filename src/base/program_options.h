// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_PROGRAM_OPTIONS_H_INCLUDED
#define BASE_PROGRAM_OPTIONS_H_INCLUDED
#pragma once

#include <string>
#include <vector>
#include <iosfwd>
#include <stdexcept>

namespace base {

  class InvalidProgramOption : public std::runtime_error {
  public:
    InvalidProgramOption(const std::string& msg)
      : std::runtime_error(msg) { }
  };

  class InvalidProgramOptionsCombination : public std::runtime_error {
  public:
    InvalidProgramOptionsCombination(const std::string& msg)
      : std::runtime_error(msg) { }
  };

  class ProgramOptionNeedsValue : public std::runtime_error {
  public:
    ProgramOptionNeedsValue(const std::string& msg)
      : std::runtime_error(msg) { }
  };

  class ProgramOptions {
  public:
    class Option {
    public:
      Option(const std::string& name)
        : m_name(name)
        , m_mnemonic(0) {
      }
      // Getters
      const std::string& name() const { return m_name; }
      const std::string& alias() const { return m_alias; }
      const std::string& description() const { return m_description; }
      const std::string& getValueName() const { return m_valueName; }
      char mnemonic() const { return m_mnemonic; }
      bool doesRequireValue() const { return !m_valueName.empty(); }
      // Setters
      Option& alias(const std::string& alias) { m_alias = alias; return *this; }
      Option& description(const std::string& desc) { m_description = desc; return *this; }
      Option& mnemonic(char mnemonic) { m_mnemonic = mnemonic; return *this; }
      Option& requiresValue(const std::string& valueName) {
        m_valueName = valueName;
        return *this;
      }
    private:
      std::string m_name;        // Name of the option (e.g. "help" for "--help")
      std::string m_alias;
      std::string m_description; // Description of the option (this can be used when the help is printed).
      std::string m_valueName;   // Empty if this option doesn't require a value, or the name of the expected value.
      char m_mnemonic;           // One character that can be used in the command line to use this option.

      friend class ProgramOptions;
    };

    class Value {
    public:
      Value(Option* option, const std::string& value)
        : m_option(option)
        , m_value(value) {
      }
      const Option* option() const { return m_option; }
      const std::string& value() const { return m_value; }
    private:
      Option* m_option;
      std::string m_value;
    };

    typedef std::vector<Option*> OptionList;
    typedef std::vector<Value> ValueList;

    ProgramOptions();

    // After destructing the ProgramOptions, you cannot continue using
    // references to "Option" instances obtained through add() function.
    ~ProgramOptions();

    // Adds a option for the program. The options must be specified
    // before calling parse(). The returned reference must be used in
    // the ProgramOptions lifetime.
    Option& add(const std::string& name);

    // Detects which options where specified in the command line.
    void parse(int argc, const char* argv[]);

    // Reset all option values/flags.
    void reset();

    // Returns the list of available options for the user.
    const OptionList& options() const { return m_options; }

    // List of specified options/values in the command line.
    const ValueList& values() const { return m_values; }

    bool enabled(const Option& option) const;
    std::string value_of(const Option& option) const;

  private:
    OptionList m_options;
    ValueList m_values;
  };

} // namespace base

// Prints the program options correctly formatted to be read by
// the user. E.g. This can be used in a --help option.
std::ostream& operator<<(std::ostream& os, const base::ProgramOptions& po);

#endif
