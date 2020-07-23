#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname $script))

# Script is chained with && so that if a command fails the scripts stops and
# returns the error code (for continuous integration)

cd ${SCRIPTPATH}/../.. && \
    cd ci && \
    ./setup_cukinia.sh && \
    [ $(whoami) == "jenkins" ] && ./cukinia -f junitxml -o so3_userspace_results.xml cukinia.conf || \
        ./cukinia cukinia.conf && \
            true # We want this to return a good error code even if tests fail

# If your username is "jenkins" well that's too bad ... we assumed you were the butler
