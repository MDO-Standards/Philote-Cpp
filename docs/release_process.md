# Release Process {#release_process}

This document describes the automated release process for Philote-Cpp, including how to create both stable releases and prereleases.

## Overview

Philote-Cpp uses an automated release workflow that is triggered when a pull request with specific labels is merged to the `main` branch. The workflow:

1. Validates PR labels
2. Calculates the new version number
3. Updates copyright years (stable releases only)
4. Updates version in `CMakeLists.txt`
5. Updates `CHANGELOG.md`
6. Creates a git commit and tag
7. Creates a GitHub Release

## Version Numbering

Philote-Cpp follows [Semantic Versioning](https://semver.org/) (SemVer):

- **MAJOR** version for incompatible API changes
- **MINOR** version for backward-compatible functionality additions
- **PATCH** version for backward-compatible bug fixes

Prerelease versions follow the format: `MAJOR.MINOR.PATCH-TYPE.NUMBER` where:
- **TYPE** is one of: `alpha`, `beta`, or `rc` (release candidate)
- **NUMBER** is an auto-incrementing counter starting at 1

Examples:
- Stable: `0.4.0`, `1.0.0`, `2.1.3`
- Prerelease: `0.4.0-alpha.1`, `1.0.0-beta.2`, `2.0.0-rc.1`

## Release Types

### Stable Release

A stable release is intended for production use and marks a version as complete and tested.

**Required labels:**
- `release` - Marks this as a stable release
- Exactly one of: `major`, `minor`, or `patch` - Specifies the version bump type

**Process:**
1. Increments the version according to the bump type
2. Updates copyright years in all source files
3. Replaces `[Unreleased]` section in CHANGELOG.md with the new version
4. Creates a new `[Unreleased]` section for future changes
5. Creates a git tag and GitHub release

**Example labels:** `release`, `minor` → Version `0.3.0` becomes `0.4.0`

### Prerelease

A prerelease is a preview version for testing purposes before a stable release.

**Required labels:**
- `prerelease` - Marks this as a prerelease
- Exactly one of: `major`, `minor`, or `patch` - Specifies the base version bump type
- Exactly one of: `alpha`, `beta`, or `rc` - Specifies the prerelease stage

**Process:**
1. Increments the base version according to the bump type
2. Finds existing prerelease tags for this version and increments the prerelease number
3. Keeps the `[Unreleased]` section in CHANGELOG.md
4. Adds a new prerelease section above `[Unreleased]`
5. Creates a git tag and GitHub release marked as prerelease

**Example labels:** `prerelease`, `minor`, `alpha` → Version `0.3.0` becomes `0.4.0-alpha.1`

## Step-by-Step Release Instructions

### 1. Prepare Your Changes

Before creating a release, ensure:

- All changes are committed and pushed to a feature branch
- Tests pass in CI
- Documentation is updated
- CHANGELOG.md `[Unreleased]` section lists all changes

### 2. Update CHANGELOG.md

Edit `CHANGELOG.md` to document changes under the `[Unreleased]` section:

```markdown
## [Unreleased]

### Added
- New feature X that does Y

### Changed
- Modified behavior of Z

### Fixed
- Fixed bug in component A
```

Follow the [Keep a Changelog](https://keepachangelog.com/) format with these categories:
- **Added** - New features
- **Changed** - Changes to existing functionality
- **Deprecated** - Soon-to-be removed features
- **Removed** - Removed features
- **Fixed** - Bug fixes
- **Security** - Security fixes

### 3. Create Pull Request

Create a pull request to merge your changes to `main`:

```bash
git checkout -b feature/my-feature
git add -A
git commit -m "Add feature X"
git push origin feature/my-feature
```

### 4. Add Release Labels

Add the appropriate labels to the pull request:

**For a patch release (0.3.0 → 0.3.1):**
- Add labels: `release`, `patch`

**For a minor release (0.3.0 → 0.4.0):**
- Add labels: `release`, `minor`

**For a major release (0.3.0 → 1.0.0):**
- Add labels: `release`, `major`

**For an alpha prerelease (0.3.0 → 0.4.0-alpha.1):**
- Add labels: `prerelease`, `minor`, `alpha`

**For a beta prerelease (0.4.0-alpha.2 → 0.4.0-beta.1):**
- Add labels: `prerelease`, `minor`, `beta`

**For a release candidate (0.4.0-beta.2 → 0.4.0-rc.1):**
- Add labels: `prerelease`, `minor`, `rc`

### 5. Merge Pull Request

Once the PR is reviewed and approved:

1. Merge the pull request to `main`
2. The release workflow will automatically trigger
3. Monitor the workflow in the **Actions** tab

### 6. Verify Release

After the workflow completes:

1. Check that the version was updated in `CMakeLists.txt`
2. Verify the git tag was created: `git tag -l`
3. View the GitHub Release at: `https://github.com/MDO-Standards/Philote-Cpp/releases`
4. Confirm the CHANGELOG.md was updated correctly

## Label Validation Rules

The release workflow enforces these rules:

1. **Release type**: Must have exactly one of `release` or `prerelease`
2. **Version bump**: Must have exactly one of `major`, `minor`, or `patch`
3. **Prerelease type**: If using `prerelease`, must have exactly one of `alpha`, `beta`, or `rc`
4. **Stable releases**: Cannot have `alpha`, `beta`, or `rc` labels
5. **Prereleases**: Must have a prerelease type label

If labels are invalid, the workflow will fail with a descriptive error message.

## Prerelease Progression Example

A typical prerelease progression for version 1.0.0 might look like:

```
0.3.0              (current stable)
  ↓
1.0.0-alpha.1      (early testing)
  ↓
1.0.0-alpha.2      (more changes)
  ↓
1.0.0-beta.1       (feature complete)
  ↓
1.0.0-beta.2       (bug fixes)
  ↓
1.0.0-rc.1         (release candidate)
  ↓
1.0.0-rc.2         (final fixes)
  ↓
1.0.0              (stable release)
```

To create each version:
- `1.0.0-alpha.1`: labels `prerelease`, `major`, `alpha`
- `1.0.0-alpha.2`: labels `prerelease`, `major`, `alpha` (auto-increments to .2)
- `1.0.0-beta.1`: labels `prerelease`, `major`, `beta`
- `1.0.0-rc.1`: labels `prerelease`, `major`, `rc`
- `1.0.0`: labels `release`, `major`

## Automated Actions

### Copyright Year Updates

For **stable releases only**, the workflow runs `scripts/update_copyright.py` to update copyright years in:

- `*.cpp` files
- `*.h` and `*.hpp` files
- `CMakeLists.txt` files

The script updates copyright statements from:
```
Copyright 2022-2024 Christopher A. Lupp
```
to:
```
Copyright 2022-2025 Christopher A. Lupp
```

### CHANGELOG.md Updates

The workflow automatically:

**For stable releases:**
1. Replaces `## [Unreleased]` with `## [VERSION] - DATE`
2. Creates a new `## [Unreleased]` section at the top
3. Updates version comparison links at the bottom

**For prereleases:**
1. Keeps the `## [Unreleased]` section
2. Adds a new `## [VERSION] - DATE` section below it
3. Adds prerelease version comparison links

### CMakeLists.txt Version Update

The workflow updates the VERSION in the root CMakeLists.txt:

```cmake
project(Philote-Cpp
    VERSION 0.4.0
    # ...
)
```

## Troubleshooting

### Workflow fails with "Must have one of 'major', 'minor', or 'patch' label"

**Solution:** Add exactly one version bump label to the PR.

### Workflow fails with "Cannot have both 'release' and 'prerelease' labels"

**Solution:** Remove one of the conflicting labels. Use only `release` OR `prerelease`.

### Workflow fails with "Prerelease must have one of 'alpha', 'beta', or 'rc' label"

**Solution:** Add a prerelease type label when using the `prerelease` label.

### Release was created with wrong version

If the release was created but has the wrong version:

1. Delete the incorrect tag locally and remotely:
   ```bash
   git tag -d vX.Y.Z
   git push origin :refs/tags/vX.Y.Z
   ```

2. Delete the GitHub Release from the releases page

3. Manually update `CMakeLists.txt` and `CHANGELOG.md` to correct versions

4. Create a new PR with correct labels

### Copyright years not updated

This is expected for prereleases. Copyright years are only updated for stable releases.

## Manual Release (Emergency)

In case the automated workflow fails, you can create a release manually:

```bash
# 1. Update version in CMakeLists.txt
vim CMakeLists.txt

# 2. Update copyright years (stable releases only)
python scripts/update_copyright.py 2025

# 3. Update CHANGELOG.md
vim CHANGELOG.md

# 4. Commit changes
git add -A
git commit -m "chore: release version X.Y.Z"
git push origin main

# 5. Create and push tag
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z

# 6. Create GitHub Release manually via web interface
```

## Best Practices

1. **Test prereleases**: Always create alpha/beta/rc versions before stable releases
2. **Document changes**: Keep CHANGELOG.md up to date as you develop
3. **Follow SemVer**: Use major bumps for breaking changes, minor for features, patch for fixes
4. **Review labels**: Double-check PR labels before merging
5. **Monitor workflow**: Watch the Actions tab to ensure the release completes successfully
6. **Update docs**: Ensure documentation reflects changes before releasing

## Resources

- [Semantic Versioning](https://semver.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [GitHub Releases Documentation](https://docs.github.com/en/repositories/releasing-projects-on-github)
- Release workflow: `.github/workflows/release.yml`
- Copyright script: `scripts/update_copyright.py`
