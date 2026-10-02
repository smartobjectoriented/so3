#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA

# Release banner, once per invocation (see scripts/common/banner.sh).
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/banner.sh"

bitbake $1 -c updiff

