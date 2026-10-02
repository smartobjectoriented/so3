#!/bin/sh

# Copyright (c) 2026 EDGEMTech SA

# Print the Infrabase release this tree's build system is aligned on,
# e.g. "1.0.0".
#
# Unlike in infrabase itself, the version cannot come from git here: this
# tree carries its own release tags (or none), and its history is not
# infrabase's. IB_VERSION is therefore a constant, bumped whenever the build
# system is realigned on a newer infrabase release.
#
# Used by the banner every script prints (scripts/common/banner.sh).
#
# Usage: ibversion.sh

IB_VERSION="1.0.0"

printf '%s\n' "$IB_VERSION"
