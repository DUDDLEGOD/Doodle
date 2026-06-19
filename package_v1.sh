#!/bin/bash
# A simple script to archive the current repository state, excluding temporary build directories, native libraries, and virtual environments.

OUTPUT_FILE="doodle-v1.tar.gz"

echo "Creating source archive ${OUTPUT_FILE}..."

# Create tarball excluding typical build, dev, and virtual environment files
tar -czf "${OUTPUT_FILE}" \
  --exclude="./.git" \
  --exclude="./.github" \
  --exclude="./.venv" \
  --exclude="./build" \
  --exclude="./dist" \
  --exclude="./doodle.egg-info" \
  --exclude="./src/doodle.egg-info" \
  --exclude="./src/build" \
  --exclude="*.pyc" \
  --exclude="*.pyd" \
  --exclude="*.so" \
  --exclude="__pycache__" \
  .

echo "Archive created successfully!"
