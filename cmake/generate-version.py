#!/usr/bin/env python3
#
# generate-version.py
#
# ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-generate-version.py`.
#

"""
Usage: generate-version.py -o OUTPUT_FILE NAME VERSION
"""

import argparse
import semver
import sys

from pathlib import Path

# Functions -------------------------------------------------------------------------------------------------

def write_header(f, name, version_string):
  version = semver.parse_version_info(version_string)
  value = version.major * 1_000_000 + version.minor * 1_000 + version.patch

  f.write(
f"""/**
 * @file version.h
 *
 * GENERATED FILE. DO NOT EDIT.
 */

#pragma once

#define {name}_VERSION_NAME "{version_string}" ///< SemVer string.

#define {name}_VERSION_MAJOR {version.major} ///< Major.
#define {name}_VERSION_MINOR {version.minor} ///< Minor.
#define {name}_VERSION_PATCH {version.patch} ///< Patch.

/// Comparable version value (major * 1,000,000 + minor * 1,000 + patch)
#define {name}_VERSION {value}

// EOF
""")

# `main`-----------------------------------------------------------------------------------------------------

def main():
  # Parse arguments
  parser = argparse.ArgumentParser()
  parser.add_argument(
    "-o", "--output",
    dest="output_file",
    type=str,
    help="Output file path",
    required=True,
  )
  parser.add_argument(
    "name",
    type=str,
    help="Name",
  )
  parser.add_argument(
    "version",
    type=str,
    help="Version string",
  )
  args = parser.parse_args()

  # Make directories as needed
  path = Path(args.output_file)
  path.parent.mkdir(parents=True, exist_ok=True)

  # Write output file
  with open(args.output_file, 'w') as f:
    write_header(f, args.name, args.version)

  return 0

if __name__ == "__main__":
  sys.exit(main())

# EOF
