#!/bin/bash -e

set -o pipefail

# Only hash the directory the script resides in.
DOCKERDIR=$(dirname "$(readlink -f "$0")")

# Echo CI variables if running in CI.
[ -z "$CI" ] || echo -n DOCKER_IMG=$DOCKER_BASE_IMG:

# Print hash.
tar -C $DOCKERDIR -cf - --group=0 --owner=0 --numeric-owner \
        --sort=name --mtime='UTC 2022-07-31' . | sha256sum | cut -d' ' -f1
