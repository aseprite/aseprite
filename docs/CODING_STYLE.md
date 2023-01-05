# Code Style Guidelines

## Basics

Basic statements:

```c++
void global_function(int arg1,
                     const int arg2, // You can use "const" preferably
                     const int arg3, ...)
{
  int value;
  const int constValue = 0;

  // We prefer to use "var = (condition ? ...: ...)" instead of
  // "var = condition ? ...: ...;" to make clear about the
  // ternary operator limits.
  int conditionalValue1 = (condition ? 1: 2);
  int conditionalValue2 = (condition ? longVarName:
                                       otherLongVarName);

  // If a condition will return, we prefer the "return"
  // statement in its own line to avoid missing the "return"
  // keyword when we read code.
  if (condition)
    return;

  // You can use braces {} if the condition has multiple lines
  // or the if-body has multiple lines.
  if (condition1 ||
      condition2) {
    return;
  }

  if (condition) {
    ...
    ...
    ...
  }

  // We prefer to avoid whitespaces between "var=initial_value"
  // or "var<limit" to see better the "; " separation. Anyway it
  // can depend on the specific condition/case, etc.
  for (int i=0; i<10; ++i) {
    ...
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
    case 1:
      ...
      break;
    case 2: {
      int varInsideCase;
      ...
      break;
    }
    default:
      break;
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
  ClassName()
    : m_memberVarA(1),
      m_memberVarB(2),
      m_memberVarC(3) {
    ...
  }

  virtual ~ClassName();

  // We can return in the same line for getter-like functions
  int memberVar() const { return m_memberVar; }
  void setMemberVar();

protected:
  virtual void onEvent1() { } // Do nothing functions can be defined as "{ }"
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
  void onEvent2() override {
    ...
  }
};
```

## Const

* [NL.26: Use conventional const notation](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl26-use-conventional-const-notation)

## C++17

We are using C++17 standard. Some things cannot be used because we're
targetting macOS 10.9, some notes are added about this:

* Use `nullptr` instead of `NULL` macro
* Use `auto` for complex types, iterators, or when the variable type
  is obvious (e.g. `auto s = new Sprite;`)
* Use range-based for loops (`for (const auto& item : values) { ... }`)
* Use template alias (`template<typename T> alias = orig<T>;`)
* Use generic lambda functions
* Use `std::shared_ptr`, `std::unique_ptr`, or `base::Ref`
* Use `std::clamp`
* Use `std::optional` but taking care of some limitations from macOS 10.9:
  * Use `std::optional::has_value()` instead of `std::optional::operator bool()` ([example](https://github.com/aseprite/laf/commit/81622fcbb9e4a0edc14a02250c387bd6fa878708))
  * Use `std::optional::operator*()` instead of `std::optional::value()` ([example](https://github.com/aseprite/aseprite/commit/4471dab289cdd45762155ce0b16472e95a7f8642))
* Use `std::variant` but taking care of some limitations from macOS 10.9:
  * Use `T* p = std::get_if<T>(&value)` instead of `T v = std::get<T>(value)` or
    create an auxiliary `get_value()` using `std::get_if` function ([example](https://github.com/aseprite/aseprite/commit/dc0e57728ae2b10cd8365ff0a50263daa8fcc9ac#diff-a59e14240d83bffc2ea917d7ddd7b2762576b0e9ab49bf823ba1a89c653ff978R98))
  * Don't use `std::visit()`, use some alternative with switch-case and the `std::variant::index()` ([example](https://github.com/aseprite/aseprite/commit/574f58375332bb80ce5572fdedb1028617786e45))
* Use `std::any` but taking care of some limitations from macOS 10.9:
  * Use `T* p = std::any_cast<T>(&value)` instead of `T v = std::any_cast<T>(value)` ([example](https://github.com/aseprite/aseprite/commit/c8d4c60f07df27590381ef28001a40f8f785f50e))
* Use `static constexpr T v = ...;`
* You can use `<atomic>`, `<thread>`, `<mutex>`, and `<condition_variable>`
* Prefer `using T = ...;` instead of `typedef ... T`
* Use `[[fallthrough]]` if needed
* We use gcc 9.2 or clang 9.0 on Linux, so check the features available in
  https://en.cppreference.com/w/cpp/compiler_support
