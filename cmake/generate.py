#!/usr/bin/env python3
#
# generate.py
#

import argparse
import sys

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
  args = parser.parse_args()

  # Write output file
  with open(args.output_file, 'w') as f:
    f.write("const char* generated() { return \"Hello from `generated.cc`!\"; }\n")

  return 0

if __name__ == "__main__":
  sys.exit(main())

# EOF
