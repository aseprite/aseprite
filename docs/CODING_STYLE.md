# Code Style Guidelines

Some general rules to write code: Try to follow the same style/format
of the file that you are editing (naming, indentation, etc.) or the
style of the module. Some [submodules](https://github.com/aseprite/aseprite/blob/main/.gitmodules),
created by us, or by third-parties, have their own style.

## clang-format

There is a [.clang-format](https://github.com/aseprite/aseprite/blob/main/.clang-format)
file available for Aseprite and laf, and we are using it with
Clang 19. You have to configure a [pre-commit](../CONTRIBUTING.md#pre-commit-hooks)
hook which will help you to do the formatting automatically before committing.

There is a [.clang-tidy](https://github.com/aseprite/aseprite/blob/main/.clang-tidy)
file used [in the GitHub actions](https://github.com/aseprite/aseprite/blob/main/.github/workflows/clang_tidy.yml)
executed on each PR. These rules are adopted progressively on patches
because are only executed in the diff, and if some rule is violated a
comment by [aseprite-bot](https://github.com/aseprite-bot) is
made. (Sometimes the bot will be wrong, so be careful.)

## Column limit

We use a [column limit](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#columnlimit)
of 100. clang-format will break lines to avoid excessing more than 100
lines, but in some extreme cases it might not break this limit, as
our [PenaltyExcessCharacter](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#penaltyexcesscharacter)
is not the highest value.

## Basics

Basic statements:

```c++
void function_with_long_args(const int argument1,
                             const int argument2,
                             const std::string& argument3,
                             const double argument4,
                             ...)
{
}

void function_with_short_args(int arg1, const int arg2, const int arg3, ...)
{
  const int constValue = 0;
  int value;

  // If a condition will return, we prefer the "return"
  // statement in its own line to avoid missing the "return"
  // keyword when we read code.
  if (condition)
    return;

  // You can use braces {} if the condition has multiple lines
  // or the if-body has multiple lines.
  if (condition1 || condition2) {
    ...
    return;
  }

  if (condition) {
    ...
  }

  for (int i = 0; i < 10; ++i) {
    // Same case as in if-return.
    if (condition)
      break;

    ...
  }

  while (condition) {
    ...
  }

  do {
    ...
  } while (condition);

  switch (condition) {
    case 1: ... break;
    case 2: {
      int varInsideCase;
      // ...
      break;
    }
    default: break;
  }
}
```

## Namespaces

Define namespaces with lower case:

```c++
namespace app {

...

} // namespace app
```

## Classes

Define classes with `CapitalCase` and member functions with `camelCase`:

```c++
class ClassName {
public:
  ClassName() : m_memberVarA(1), m_memberVarB(2), m_memberVarC(3) {}

  ClassName(int a, int b, int c, int d)
    : m_memberVarA(a)
    , m_memberVarB(b)
    , m_memberVarC(c)
    , m_memberVarD(d)
  {
    // ...
  }

  virtual ~ClassName();

  // We can return in the same line for getter-like functions
  int memberVar() const { return m_memberVar; }
  void setMemberVar();

protected:
  virtual void onEvent1() {} // Do nothing functions can be defined as "{}"
  virtual void onEvent2() = 0;

private:
  int m_memberVarA;
  int m_memberVarB;
  int m_memberVarC;
  int m_memberVarD = 4; // We can initialize variables here too
};

class Special : public ClassName {
public:
  Special();

protected:
  void onEvent2() override
  {
    // No need to repeat virtual in overridden methods
    ...
  }
};
```

## Const

We use the const-west notation:

* [NL.26: Use conventional const notation](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl26-use-conventional-const-notation)

There is a problem with `clang-tidy` that will make comments using
East const notation:
[#4361](https://github.com/aseprite/aseprite/issues/4361), but
clang-format should fix the `const` position anyway.

## C++17

We are using C++17 standard. Some things might not be used as we're
targetting macOS 10.14, please add some note about its limitations if
you find one:

* Use `nullptr` instead of `NULL` macro
* Use `auto`/`auto*` for complex types/pointers, iterators, or when
  the variable type is obvious (e.g. `auto* s = new Sprite;`)
* Use range-based for loops (`for (const auto& item : values) { ... }`)
* Use template alias (`template<typename T> alias = orig<T>;`)
* Use generic lambda functions
* Use `std::shared_ptr`, `std::unique_ptr`, or `base::Ref`, but
  generally we'd prefer value semantics instead of smart pointers
* Use `std::min`/`std::max`/`std::clamp`
* Use `static constexpr T v = ...;`
* You can use `<atomic>`, `<thread>`, `<mutex>`, and `<condition_variable>`
* Prefer `using T = ...;` instead of `typedef ... T`
* Use `[[fallthrough]]` if needed
* Use `= {}` only to specify a default argument value of an
  user-defined type in a function declaration, e.g.
  `void func(const std::string& s = {}) { ... }`.
  In other cases (e.g. a member variable of an user-defined type)
  it's not required or we prefer to use the explicit value
  for built-in types (`int m_var = 0;`).
* We use gcc 9.2 or clang 9.0 on Linux, so check the features available in
  https://en.cppreference.com/w/cpp/compiler_support
