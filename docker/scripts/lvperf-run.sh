#!/bin/sh
# Run the SO3 lv_perf container: it (re)builds the user space, creates+deploys
# the SD-card and runs the LVGL benchmark under the patched QEMU, then exits
# (semihosting) with the perf output on stdout.
#
# Override the image with an argument (default: so3-lvperf64b), e.g.
#   docker/scripts/lvperf-run.sh so3-lvperf32b
#
# --network=host: the run-time usr rebuild may fetch LVGL (see docker/README.md).
exec docker run -it --rm --privileged --network=host -v /dev:/dev "${1:-so3-lvperf64b}"
