# Contributing to Philote-Cpp

Thank you for your interest in contributing to Philote-Cpp! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Documentation](#documentation)
- [Submitting Changes](#submitting-changes)
- [Release Process](#release-process)

## Code of Conduct

Please be respectful and constructive in all interactions. We aim to foster an open and welcoming environment for all contributors.

## Getting Started

### Prerequisites

Before contributing, ensure you have:

- **C++20** compatible compiler (gcc-12+, clang-16+)
- **CMake** 3.20 or higher
- **gRPC** and **Protobuf** libraries
- **Git** for version control

See the [Installation Guide](docs/installation.md) for detailed setup instructions.

### Building the Project

```bash
# Clone the repository
git clone https://github.com/MDO-Standards/Philote-Cpp.git
cd Philote-Cpp

# Create build directory
mkdir build && cd build

# Configure with tests enabled
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### Running Tests

```bash
# Run all tests
ctest

# Run specific test
./test/ExplicitDisciplineServerTests

# Run with verbose output
ctest --verbose

# Run with coverage (if enabled)
cmake .. -DENABLE_COVERAGE=ON
cmake --build .
ctest
# Generate coverage report
```

## Development Workflow

### 1. Create a Feature Branch

```bash
git checkout -b feature/my-new-feature
```

Branch naming conventions:
- `feature/` - New features
- `bugfix/` - Bug fixes
- `docs/` - Documentation changes
- `refactor/` - Code refactoring
- `test/` - Test additions/improvements

### 2. Make Your Changes

- Write clean, well-documented code
- Follow the [Coding Style Guide](docs/coding_style.md)
- Add tests for new functionality
- Update documentation as needed

### 3. Test Your Changes

```bash
# Build and run tests
cmake --build . && ctest --output-on-failure

# Run specific tests related to your changes
./test/MyNewTests
```

### 4. Update CHANGELOG

Add your changes to the `[Unreleased]` section of `CHANGELOG.md`:

```markdown
## [Unreleased]

### Added
- New feature X that does Y (#123)

### Fixed
- Fixed bug in component Z (#124)
```

Follow the [Keep a Changelog](https://keepachangelog.com/) format.

### 5. Commit Your Changes

Write clear, descriptive commit messages:

```bash
git add -A
git commit -m "Add feature X to improve Y

- Implement new functionality for Z
- Add tests for feature X
- Update documentation

Fixes #123"
```

**Commit message format:**
- First line: Brief summary (50 chars or less)
- Blank line
- Detailed description (wrap at 72 chars)
- Reference related issues/PRs

### 6. Push and Create Pull Request

```bash
git push origin feature/my-new-feature
```

Create a pull request on GitHub with:
- Clear title describing the change
- Description of what was changed and why
- Reference to related issues
- Screenshots/examples if applicable

## Coding Standards

Philote-Cpp follows the **Google C++ Style Guide** with one key exception:

### Indentation: 4 Spaces

Use **4 spaces** for indentation (not 2 spaces, not tabs).

```cpp
class MyClass {
public:
    void MyMethod() {
        if (condition) {
            DoSomething();
        }
    }
};
```

See the complete [Coding Style Guide](docs/coding_style.md) for details on:
- Naming conventions (PascalCase, snake_case, etc.)
- File organization
- Comments and documentation
- Language features and best practices

### Automatic Formatting

Use the provided `.clang-format` configuration:

```bash
# Format a single file
clang-format -i src/myfile.cpp

# Format all source files
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### Editor Configuration

The `.editorconfig` file ensures consistent settings across editors. Most modern editors support EditorConfig automatically.

## Testing

### Writing Tests

All new functionality should include tests using Google Test:

```cpp
#include <gtest/gtest.h>

TEST(MyFeatureTest, BasicFunctionality) {
    MyClass obj;
    EXPECT_EQ(obj.Compute(5), 10);
}

TEST(MyFeatureTest, ErrorHandling) {
    MyClass obj;
    EXPECT_THROW(obj.Compute(-1), std::invalid_argument);
}
```

### Test Organization

- Unit tests: Test individual classes/functions in isolation
- Integration tests: Test interactions between components
- Use mocks for external dependencies (gRPC, etc.)

### Running Specific Tests

```bash
# Run all tests
ctest

# Run tests matching pattern
ctest -R ExplicitDiscipline

# Run tests with verbose output
ctest -V

# Run specific test executable
./test/ExplicitDisciplineServerTests --gtest_filter=MyTest.*
```

## Documentation

### Code Documentation

Use Doxygen-style comments for all public APIs:

```cpp
/**
 * @brief Compute the function outputs given inputs
 *
 * This method performs the main computation of the discipline.
 * Derived classes must override this method.
 *
 * @param inputs Input variables for the computation
 * @param outputs Output variables to be populated
 * @throws std::invalid_argument if inputs are invalid
 */
virtual void Compute(const Variables& inputs,
                    Variables& outputs) = 0;
```

### User Documentation

Update documentation in `docs/` when:
- Adding new features
- Changing APIs
- Adding examples
- Fixing significant bugs

Build documentation locally:

```bash
cd docs
doxygen Doxyfile
open generated/html/index.html
```

See [Documentation Guide](docs/README.md) for more details.

## Submitting Changes

### Pull Request Checklist

Before submitting a pull request, ensure:

- [ ] Code follows the [coding style guide](docs/coding_style.md)
- [ ] All tests pass (`ctest`)
- [ ] New functionality includes tests
- [ ] Documentation is updated
- [ ] CHANGELOG.md is updated
- [ ] Commit messages are clear and descriptive
- [ ] No compiler warnings introduced
- [ ] Code compiles on supported platforms

### Code Review Process

1. Submit pull request to `main` branch
2. Automated CI tests will run
3. Maintainer will review your code
4. Address any feedback
5. Once approved, PR will be merged

### CI/CD

GitHub Actions automatically:
- Builds on multiple platforms/compilers
- Runs all tests
- Checks code formatting (future)
- Builds documentation

Ensure all CI checks pass before requesting review.

## Release Process

Releases are automated using GitHub Actions. See the [Release Process Guide](docs/release_process.md) for details.

### For Maintainers

To create a release:

1. Create PR with changes and updated CHANGELOG
2. Add appropriate labels:
   - `release` or `prerelease`
   - `major`, `minor`, or `patch`
   - For prereleases: `alpha`, `beta`, or `rc`
3. Merge PR to `main`
4. Automated workflow creates release

Example labels:
- **Patch release**: `release`, `patch`
- **Minor release**: `release`, `minor`
- **Alpha prerelease**: `prerelease`, `minor`, `alpha`

## Questions or Issues?

- **Bug reports**: Open an issue on GitHub
- **Feature requests**: Open an issue with `[Feature Request]` prefix
- **Questions**: Use GitHub Discussions or open an issue
- **Security issues**: Email maintainers directly

## License

By contributing to Philote-Cpp, you agree that your contributions will be licensed under the Apache License 2.0.

## Recognition

Contributors will be acknowledged in:
- Git commit history
- GitHub contributors page
- Release notes (for significant contributions)

Thank you for contributing to Philote-Cpp!
