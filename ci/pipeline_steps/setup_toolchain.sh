#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname "$script"))

# Script is chained with && so that if a command fails the scripts stops and
# returns the error code (for continuous integration)

cd "${SCRIPTPATH}/../.." && \
source toolchain/setup_env
