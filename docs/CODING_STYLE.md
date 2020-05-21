# Code Style Guidelines

## Basics

Basic statements:

```c++
void global_function(int arg1, int arg2,
                     int arg3, ...)
{
  int value;
  const int constValue = 0;

  if (condition) {
    ...
  }

  for (int i=0; i<10; ++i) {
    ...
  }

  while (condition) {
    ...
  }

  do {
    ...
  } while  (condition);

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
  ClassName();
  virtual ~ClassName();

  int memberVar() const { return m_memberVar; }
  void setMemberVar();

protected:
  void protectedMember();

private:
  int m_memberVar;
};
```

## C++11

We are using some C++11 features, mainly:

* Use `nullptr` instead of `NULL` macro
* Use `auto` for complex types, iterators, or when it's variable type
  obvious (e.g. `auto s = new Sprite;`)
* Use range-based for loops (`for (const auto& item : values) { ... }`)
* Use template alias (`template<typename T> alias = orig<T>;`)
* Use non-generic lambda functions
* Use `std::shared_ptr` and `std::unique_ptr`
* Use `base::clamp` (no `std::clamp` yet)
* Use `static constexpr T v = ...;`
* You can use `<atomic>`, `<thread>`, `<mutex>`, and `<condition_variable>`
* We can use `using T = ...;` instead of `typedef ... T`
* We use gcc 9.2 or clang 9.0 on Linux, so check the features available in
  https://developer.mozilla.org/en-US/docs/Mozilla/Using_CXX_in_Mozilla_code
