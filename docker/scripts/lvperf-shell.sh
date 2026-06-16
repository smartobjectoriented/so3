#!/bin/sh
# Open an interactive shell in the SO3 lv_perf container instead of running the
# benchmark. Use it to drive the in-container workflow by hand:
#   . ./env.sh ; build.sh -x usr-so3 ; build.sh -f ; deploy.sh -a bsp-so3 ; docker/scripts/run.sh
#
# Override the image with an argument (default: so3-lvperf64b), e.g.
#   docker/scripts/lvperf-shell.sh so3-lvperf32b
exec docker run -it --rm --privileged --network=host -v /dev:/dev "${1:-so3-lvperf64b}" /bin/bash
