# Coding Style Guide {#coding_style}

This document describes the coding standards and style guidelines for Philote-Cpp.

## Overview

Philote-Cpp follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with one key exception:

**Indentation: Use 4 spaces instead of 2 spaces**

All other aspects of the Google C++ Style Guide apply unless explicitly stated otherwise in this document.

## Indentation and Formatting

### Indentation

Use **4 spaces** for indentation (not tabs, not 2 spaces):

```cpp
// Good
class MyClass {
public:
    void MyMethod() {
        if (condition) {
            DoSomething();
        }
    }
};

// Bad - uses 2 spaces
class MyClass {
  public:
    void MyMethod() {
      if (condition) {
        DoSomething();
      }
    }
};
```

### Line Length

Maximum line length is **80 characters** (as per Google style).

```cpp
// Good - fits within 80 characters
void ShortFunction(int arg1, int arg2);

// Good - wrapped appropriately
void LongFunctionName(int argument1, int argument2,
                     int argument3, int argument4);

// Bad - exceeds 80 characters
void LongFunctionName(int argument1, int argument2, int argument3, int argument4, int argument5);
```

### Braces

Use K&R style bracing (opening brace on same line):

```cpp
// Good
if (condition) {
    DoSomething();
}

class MyClass {
    // ...
};

void Function() {
    // ...
}

// Bad - opening brace on new line
if (condition)
{
    DoSomething();
}
```

## Naming Conventions

### Class Names

Use PascalCase for class names:

```cpp
class ExplicitDiscipline { /* ... */ };
class DisciplineServer { /* ... */ };
class MyCustomDiscipline { /* ... */ };
```

### Function Names

Use PascalCase for function names:

```cpp
void ComputeFunction();
void GetInfo();
void SetVariableMeta();
```

### Variable Names

Use snake_case for local variables and function parameters:

```cpp
int variable_count = 0;
std::string discipline_name;
double computation_result;

void ProcessData(const Variables& input_variables,
                Variables& output_variables) {
    // ...
}
```

### Member Variables

Use snake_case with a trailing underscore for private member variables:

```cpp
class MyClass {
public:
    void SetName(const std::string& name) { name_ = name; }

private:
    std::string name_;
    int counter_;
    std::vector<double> data_;
};
```

### Constants

Use kConstantName (k prefix with PascalCase):

```cpp
const int kMaxIterations = 1000;
const double kTolerance = 1e-6;
const char* kDefaultServerAddress = "localhost:50051";
```

### Enumerations

Use PascalCase for enum names and kEnumValue for enum values:

```cpp
enum VariableType {
    kInput,
    kOutput,
    kResidual
};
```

### Namespace

Use lowercase for namespace names:

```cpp
namespace philote {
    // ...
}

namespace philote::internal {
    // ...
}
```

## File Organization

### Header Files

Header files should use `.h` extension:

```
discipline.h
explicit.h
variable.h
```

### Implementation Files

Implementation files should use `.cpp` extension:

```
discipline.cpp
explicit.cpp
variable.cpp
```

### File Structure

Every file should start with a copyright header:

```cpp
/*
 * Copyright 2022-2025 Christopher A. Lupp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This work has been cleared for public release, distribution unlimited,
 * case number: AFRL-2023-5716.
 */
```

### Header Guards

Use `#pragma once` for header guards (preferred) or traditional include guards:

```cpp
#pragma once

// Header content
```

Or:

```cpp
#ifndef PHILOTE_DISCIPLINE_H
#define PHILOTE_DISCIPLINE_H

// Header content

#endif  // PHILOTE_DISCIPLINE_H
```

### Include Order

Order includes as follows:

1. Related header (for .cpp files)
2. C system headers
3. C++ standard library headers
4. Other libraries' headers
5. Project headers

Separate each group with a blank line:

```cpp
#include "discipline.h"  // Related header

#include <string.h>      // C system headers

#include <memory>        // C++ standard library
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>  // Other libraries

#include "variable.h"       // Project headers
```

## Comments and Documentation

### Class Documentation

Use Doxygen-style comments for classes:

```cpp
/**
 * @brief Base class for explicit disciplines
 *
 * Explicit disciplines compute direct function evaluations of the form
 * y = f(x) where inputs map directly to outputs.
 *
 * Derived classes must implement Setup(), Compute(), and optionally
 * ComputePartials() to provide gradient information.
 */
class ExplicitDiscipline : public Discipline {
    // ...
};
```

### Function Documentation

Document public functions with Doxygen comments:

```cpp
/**
 * @brief Compute the function outputs given inputs
 *
 * @param inputs Input variables for the computation
 * @param outputs Output variables to be populated
 */
void Compute(const Variables& inputs, Variables& outputs);
```

### Inline Comments

Use `//` for single-line comments:

```cpp
// Calculate the gradient using finite differences
double gradient = (f1 - f0) / epsilon;

// This is a workaround for gRPC issue #12345
SomeComplexWorkaround();
```

Use `/* */` for multi-line explanatory comments:

```cpp
/*
 * This algorithm implements a specialized solver for the implicit
 * discipline residuals. It uses Newton's method with backtracking
 * line search to ensure convergence.
 */
```

### TODO Comments

Use TODO comments with your name or GitHub issue:

```cpp
// TODO(username): Add error checking for invalid inputs
// TODO(#123): Implement caching for computed gradients
```

## Language Features

### C++ Version

Philote-Cpp uses **C++20**. Modern C++ features are encouraged:

```cpp
// Good - use auto for complex types
auto it = variables.find("x");

// Good - use range-based for loops
for (const auto& [name, variable] : variables) {
    ProcessVariable(name, variable);
}

// Good - use structured bindings
auto [value, success] = TryGetValue();
```

### Smart Pointers

Use smart pointers instead of raw pointers:

```cpp
// Good
std::unique_ptr<Server> server = builder.BuildAndStart();
std::shared_ptr<Channel> channel = CreateChannel(address);

// Bad
Server* server = builder.BuildAndStart();  // Manual memory management
```

### Const Correctness

Use `const` wherever possible:

```cpp
// Good
void ProcessData(const Variables& inputs) {
    const double value = inputs.at("x")(0);
    // ...
}

// Bad
void ProcessData(Variables& inputs) {  // Should be const
    double value = inputs.at("x")(0);  // Should be const
    // ...
}
```

### References vs Pointers

Prefer references over pointers when possible:

```cpp
// Good - use reference for required parameters
void Process(const Variable& input);

// Good - use pointer to indicate optional
void Process(const Variable* optional_param = nullptr);

// Bad - pointer for required parameter
void Process(const Variable* input);  // Unclear if can be null
```

## Class Design

### Inheritance

Use `override` keyword for virtual function overrides:

```cpp
class MyDiscipline : public ExplicitDiscipline {
public:
    void Setup() override;
    void Compute(const Variables& inputs,
                Variables& outputs) override;
};
```

### Access Specifiers

Order class members as: public, protected, private:

```cpp
class MyClass {
public:
    MyClass();
    void PublicMethod();

protected:
    void ProtectedMethod();

private:
    void PrivateMethod();
    int private_member_;
};
```

### Constructor Initialization

Use member initialization lists:

```cpp
// Good
MyClass::MyClass(int value, const std::string& name)
    : value_(value),
      name_(name),
      counter_(0) {
    // Constructor body
}

// Bad
MyClass::MyClass(int value, const std::string& name) {
    value_ = value;
    name_ = name;
    counter_ = 0;
}
```

## Error Handling

### Exceptions

Use exceptions for error handling:

```cpp
if (!IsValid(input)) {
    throw std::invalid_argument("Input must be positive");
}
```

### Assertions

Use assertions for internal consistency checks:

```cpp
#include <cassert>

void Process(const Variable& var) {
    assert(var.Size() > 0);  // Internal invariant
    // ...
}
```

### Error Messages

Provide clear, actionable error messages:

```cpp
// Good
throw std::runtime_error(
    "Failed to connect to server at " + address +
    ": connection refused. Check that the server is running.");

// Bad
throw std::runtime_error("Error");  // Not helpful
```

## Best Practices

### RAII

Follow RAII (Resource Acquisition Is Initialization) principle:

```cpp
// Good - resource automatically cleaned up
{
    std::unique_ptr<Resource> resource = AcquireResource();
    UseResource(resource.get());
}  // Resource automatically released

// Bad - manual cleanup required
Resource* resource = AcquireResource();
UseResource(resource);
delete resource;  // Easy to forget or skip on error paths
```

### Avoid Global Variables

Minimize use of global variables:

```cpp
// Bad
int global_counter = 0;  // Global state

// Good - encapsulate in class
class Counter {
public:
    void Increment() { count_++; }
private:
    int count_ = 0;
};
```

### Use Standard Library

Prefer standard library over custom implementations:

```cpp
// Good
std::vector<double> data;
std::map<std::string, Variable> variables;

// Bad - reinventing the wheel
MyCustomVector<double> data;
MyCustomMap<std::string, Variable> variables;
```

### Minimize Header Dependencies

Use forward declarations where possible:

```cpp
// Good - forward declaration in header
class Variable;

class MyClass {
    void Process(const Variable& var);
};

// In .cpp file
#include "variable.h"
```

## Code Examples

### Good Example - Explicit Discipline

```cpp
/**
 * @brief Paraboloid discipline computing f(x,y) = (x-3)^2 + xy + (y+4)^2 - 3
 */
class Paraboloid : public philote::ExplicitDiscipline {
public:
    /**
     * @brief Setup input and output variables
     */
    void Setup() override {
        AddInput("x", {1}, "m");
        AddInput("y", {1}, "m");
        AddOutput("f_xy", {1}, "m^2");
    }

    /**
     * @brief Compute the paraboloid function
     *
     * @param inputs Input variables containing x and y
     * @param outputs Output variables to store f_xy
     */
    void Compute(const philote::Variables& inputs,
                philote::Variables& outputs) override {
        const double x = inputs.at("x")(0);
        const double y = inputs.at("y")(0);

        outputs.at("f_xy")(0) = (x - 3.0) * (x - 3.0) +
                                x * y +
                                (y + 4.0) * (y + 4.0) - 3.0;
    }
};
```

## Tools

### ClangFormat (Optional)

A `.clang-format` configuration file is provided to automatically format code:

```bash
# Format a single file
clang-format -i src/discipline.cpp

# Format all source files
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

Note: The `.clang-format` file is configured for 4-space indentation.

### Editor Configuration

Configure your editor to use 4 spaces for indentation:

**VS Code (.vscode/settings.json):**
```json
{
    "editor.tabSize": 4,
    "editor.insertSpaces": true,
    "editor.detectIndentation": false
}
```

**Vim (.vimrc):**
```vim
set tabstop=4
set shiftwidth=4
set expandtab
```

**CLion/IntelliJ:**
Go to Settings → Editor → Code Style → C/C++ → Tabs and Indents
- Tab size: 4
- Indent: 4
- Use spaces: ✓

## Code Review Checklist

When reviewing code, check for:

- [ ] 4-space indentation used consistently
- [ ] Line length does not exceed 80 characters
- [ ] Naming conventions followed (PascalCase, snake_case, etc.)
- [ ] Copyright header present in new files
- [ ] Public functions documented with Doxygen comments
- [ ] `const` used where appropriate
- [ ] `override` keyword used for virtual functions
- [ ] Smart pointers used instead of raw pointers
- [ ] Exceptions used for error handling with clear messages
- [ ] No global variables introduced
- [ ] RAII principles followed
- [ ] Standard library used where appropriate

## References

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) - Base style guide
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - Modern C++ best practices
- [Doxygen Documentation](https://www.doxygen.nl/manual/docblocks.html) - Documentation comment syntax

## Questions

If you have questions about coding style or encounter situations not covered in this guide:

1. Check the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
2. Look at existing code in the project for examples
3. When in doubt, prioritize clarity and consistency with existing code
4. Open an issue or discussion on GitHub for guidance
