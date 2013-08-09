// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_PROGRAM_OPTIONS_H_INCLUDED
#define BASE_PROGRAM_OPTIONS_H_INCLUDED

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
        , m_mnemonic(0)
        , m_enabled(false) {
      }
      // Getters
      const std::string& name() const { return m_name; }
      const std::string& description() const { return m_description; }
      const std::string& value() const { return m_value; }
      const std::string& getValueName() const { return m_valueName; }
      char mnemonic() const { return m_mnemonic; }
      bool enabled() const { return m_enabled; }
      bool doesRequireValue() const { return !m_valueName.empty(); }
      // Setters
      Option& description(const std::string& desc) { m_description = desc; return *this; }
      Option& mnemonic(char mnemonic) { m_mnemonic = mnemonic; return *this; }
      Option& requiresValue(const std::string& valueName) {
        m_valueName = valueName;
        return *this;
      }
    private:
      void setValue(const std::string& value) { m_value = value; }
      void setEnabled(bool enabled) { m_enabled = enabled; }

      std::string m_name;        // Name of the option (e.g. "help" for "--help")
      std::string m_description; // Description of the option (this can be used when the help is printed).
      std::string m_value;       // The value specified by the user in the command line.
      std::string m_valueName;   // Empty if this option doesn't require a value, or the name of the expected value.
      char m_mnemonic;           // One character that can be used in the command line to use this option.
      bool m_enabled;            // True if the user specified this argument.

      friend class ProgramOptions;
    };

    typedef std::vector<Option*> OptionList;
    typedef std::vector<std::string> ValueList;

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

    // Reset all options values/flags.
    void reset();

    // Returns the list of available options. To know the list of
    // specified options you can iterate this list asking for
    // Option::enabled() flag to know if the option was specified by
    // the user in the command line.
    const OptionList& options() const { return m_options; }

    // Returns the list of values that are not associated to any
    // options. E.g. a list of files specified in the command line to
    // be opened.
    const ValueList& values() const { return m_values; }

  private:
    static void resetOption(Option* option);

    OptionList m_options;
    ValueList m_values;
  };

} // namespace base

// Prints the program options correctly formatted to be read by
// the user. E.g. This can be used in a --help option.
std::ostream& operator<<(std::ostream& os, const base::ProgramOptions& po);

#endif
