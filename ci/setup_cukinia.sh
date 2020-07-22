#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname $script))

if [ ! -f "${SCRIPTPATH}/cukinia" ]; then
    echo "Cukinia is missing, installing ..."
    tmp_dir=$(mktemp -d)
    git clone https://github.com/savoirfairelinux/cukinia ${tmp_dir}
    cp ${tmp_dir}/cukinia .
    rm -rf ${tmp_dir}
    echo "Cukinia is locally installed !"
fi
