# Philote-Cpp Documentation

This directory contains the Doxygen documentation source files and configuration for Philote-Cpp.

## Documentation Structure

The documentation is organized into topic-specific pages:

- **mainpage.md** - Main landing page with overview and quick reference
- **getting_started.md** - Getting started guide with basic concepts
- **installation.md** - Installation and integration guide for external projects
- **variables.md** - Variable class and data structures guide
- **explicit_disciplines.md** - Creating explicit disciplines
- **implicit_disciplines.md** - Creating implicit disciplines
- **client_usage.md** - Client connection and usage patterns
- **best_practices.md** - Best practices, limitations, and troubleshooting

## Building Documentation Locally

### Prerequisites

Install Doxygen:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz
```

### Generate Documentation

```bash
cd docs
doxygen Doxyfile
```

The generated HTML documentation will be available at:
```
docs/generated/html/index.html
```

Open in browser:
```bash
# macOS
open generated/html/index.html

# Linux
xdg-open generated/html/index.html
```

## Automatic Deployment

Documentation is automatically built and deployed to GitHub Pages on every push to the `main` branch.

### GitHub Actions Workflow

The `.github/workflows/documentation.yml` workflow:

1. **Triggers** on push to `main` branch
2. **Installs** Doxygen and Graphviz
3. **Builds** documentation using `doxygen Doxyfile`
4. **Creates** `.nojekyll` file to disable Jekyll processing
5. **Deploys** to GitHub Pages

### Viewing Published Documentation

Once deployed, the documentation is available at:
```
https://mdo-standards.github.io/Philote-Cpp/
```

(Adjust URL based on your GitHub organization/username and repository name)

### GitHub Pages Setup

To enable GitHub Pages for this repository:

1. Go to **Settings** → **Pages**
2. Under **Source**, select **GitHub Actions**
3. The workflow will automatically deploy on the next push to `main`

## Doxygen Configuration

Key configuration settings in `Doxyfile`:

- **PROJECT_NAME**: "Philote-Cpp"
- **PROJECT_BRIEF**: "C++ bindings for the Philote MDO standard"
- **INPUT**: All markdown files and header directories
- **OUTPUT_DIRECTORY**: `generated/`
- **RECURSIVE**: Yes (searches subdirectories)
- **EXTRACT_ALL**: Yes (documents all entities)
- **JAVADOC_AUTOBRIEF**: Yes (first line becomes brief description)
- **USE_MDFILE_AS_MAINPAGE**: `mainpage.md`

## Adding New Documentation

### Adding a New Page

1. Create a new markdown file in `docs/`:
   ```bash
   touch docs/my_new_page.md
   ```

2. Add a page anchor at the top:
   ```markdown
   # My New Page {#my_new_page}
   ```

3. Add the file to `Doxyfile` INPUT:
   ```
   INPUT = ... my_new_page.md
   ```

4. Link to it from other pages:
   ```markdown
   See @ref my_new_page for details.
   ```

5. Rebuild documentation:
   ```bash
   cd docs
   doxygen Doxyfile
   ```

### Adding Examples to Headers

Use Doxygen comment blocks in header files:

```cpp
/**
 * @brief Brief description of the class
 *
 * Detailed description here.
 *
 * @par Example
 * @code
 * MyClass obj;
 * obj.doSomething();
 * @endcode
 *
 * @see OtherClass
 */
class MyClass {
    // ...
};
```

### Cross-References

Link to other documentation:

- Pages: `@ref page_id`
- Classes: `philote::ClassName`
- Functions: `philote::ClassName::functionName()`
- Sections: `@ref section_id`

Example:
```markdown
See @ref installation for setup instructions.
Use philote::ExplicitDiscipline as base class.
Call philote::Variable::Size() to get array size.
```

## Documentation Style Guide

### Markdown Sections

Use headers to organize content:

```markdown
# Top Level Section

## Subsection

### Sub-subsection
```

### Code Blocks

Specify language for syntax highlighting:

```markdown
```cpp
// C++ code here
```

```bash
# Bash commands here
```
```

### Tables

Use markdown tables:

```markdown
| Column 1 | Column 2 |
|----------|----------|
| Value 1  | Value 2  |
```

### Lists

```markdown
- Bullet point 1
- Bullet point 2

1. Numbered item 1
2. Numbered item 2
```

### Notes and Warnings

```markdown
@note This is an important note.

@warning This is a warning.

**Important**: This is also important.
```

## Troubleshooting

### Documentation not updating

- Clear generated files: `rm -rf docs/generated/`
- Rebuild: `cd docs && doxygen Doxyfile`

### Missing pages

- Check file is listed in `Doxyfile` INPUT
- Verify page anchor: `{#page_id}`
- Check for Doxygen warnings during build

### Broken links

- Ensure referenced pages exist
- Check anchor format: `@ref page_id` or `philote::ClassName`
- Verify cross-references are spelled correctly

### GitHub Pages not updating

- Check workflow run in **Actions** tab
- Verify GitHub Pages is enabled in **Settings**
- Check for workflow errors or warnings
- May take a few minutes to propagate

## Maintenance

### Regular Updates

When adding new features:

1. Update relevant documentation pages
2. Add examples to header files
3. Update mainpage.md if adding major features
4. Test documentation build locally
5. Commit documentation with feature code

### Version Updates

When releasing a new version:

1. Update version in root `CMakeLists.txt`
2. Add notes to `CHANGELOG.md`
3. Verify all documentation is current
4. Tag release after documentation is merged

## Resources

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Markdown Syntax](https://www.markdownguide.org/basic-syntax/)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)
