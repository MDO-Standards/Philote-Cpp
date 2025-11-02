#!/usr/bin/env python3
"""
Update copyright year ranges in source files.

This script updates copyright statements to include the current year.
It handles both C-style (/* */) and CMake (#) comment formats.

Usage:
    python update_copyright.py [year]

If year is not provided, uses the current year.
"""

import os
import re
import sys
from datetime import datetime
from pathlib import Path


def find_source_files(root_dir):
    """
    Find all source files that should have copyright headers updated.

    Searches for .cpp, .h, .hpp, and CMakeLists.txt files, excluding
    the proto/ submodule directory.

    Args:
        root_dir: Root directory to search from

    Returns:
        List of Path objects for files to update
    """
    root_path = Path(root_dir)
    files = []

    # Patterns to match
    extensions = ['*.cpp', '*.h', '*.hpp']

    # Find all matching files
    for ext in extensions:
        for file_path in root_path.rglob(ext):
            # Skip proto/ submodule
            if 'proto' in file_path.parts:
                continue
            # Skip build directories
            if 'build' in file_path.parts:
                continue
            files.append(file_path)

    # Also find CMakeLists.txt files
    for file_path in root_path.rglob('CMakeLists.txt'):
        if 'proto' in file_path.parts:
            continue
        if 'build' in file_path.parts:
            continue
        files.append(file_path)

    return sorted(files)


def update_copyright_year(content, target_year):
    """
    Update copyright year range in content.

    Patterns matched:
    - Copyright 2022-2024 -> Copyright 2022-2025
    - Copyright 2022 -> Copyright 2022-2025

    Args:
        content: File content as string
        target_year: Year to update to (as string)

    Returns:
        Updated content, or None if no copyright found
    """
    # Pattern to match: Copyright YYYY-YYYY or Copyright YYYY
    # Captures the starting year and optionally the ending year
    pattern = r'(Copyright\s+)(\d{4})(?:-(\d{4}))?(\s+Christopher A\. Lupp)'

    def replace_year(match):
        prefix = match.group(1)  # "Copyright "
        start_year = match.group(2)  # Starting year (e.g., "2022")
        end_year = match.group(3)  # Ending year if present (e.g., "2024" or None)
        suffix = match.group(4)  # " Christopher A. Lupp"

        # If end year exists and matches target, no change needed
        if end_year == target_year:
            return match.group(0)

        # If start year equals target year, no range needed
        if start_year == target_year:
            return f"{prefix}{start_year}{suffix}"

        # Otherwise, create or update the range
        return f"{prefix}{start_year}-{target_year}{suffix}"

    updated_content = re.sub(pattern, replace_year, content)

    # Check if any replacement was made
    if updated_content != content:
        return updated_content

    return None


def process_file(file_path, target_year, dry_run=False):
    """
    Process a single file to update copyright year.

    Args:
        file_path: Path to file
        target_year: Year to update to
        dry_run: If True, don't write changes

    Returns:
        True if file was updated, False otherwise
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        updated_content = update_copyright_year(content, str(target_year))

        if updated_content is not None:
            if not dry_run:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(updated_content)
            print(f"Updated: {file_path}")
            return True
        else:
            # Check if file even has a copyright
            if 'Copyright' in content and 'Christopher A. Lupp' in content:
                print(f"Already up-to-date: {file_path}")
            else:
                print(f"No copyright found: {file_path}")
            return False

    except Exception as e:
        print(f"Error processing {file_path}: {e}", file=sys.stderr)
        return False


def main():
    """Main entry point."""
    # Get target year from command line or use current year
    if len(sys.argv) > 1:
        target_year = int(sys.argv[1])
    else:
        target_year = datetime.now().year

    print(f"Updating copyright years to include {target_year}")
    print()

    # Find repository root (directory containing this script's parent)
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    print(f"Repository root: {repo_root}")
    print()

    # Find all source files
    files = find_source_files(repo_root)
    print(f"Found {len(files)} files to process")
    print()

    # Process each file
    updated_count = 0
    for file_path in files:
        if process_file(file_path, target_year, dry_run=False):
            updated_count += 1

    print()
    print(f"Summary: Updated {updated_count} of {len(files)} files")

    # Return 0 (success) - either files were updated or already up-to-date
    # The script should only fail on actual errors, not when files are current
    return 0


if __name__ == '__main__':
    sys.exit(main())
