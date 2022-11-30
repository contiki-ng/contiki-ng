#!/bin/bash -e

# Script that gives the container user uid $LOCAL_UID and gid $LOCAL_GID.
# If $LOCAL_UID or $LOCAL_GID are not set, they default to 1000 (default
# for the first user created in Ubuntu).

USER_ID=${LOCAL_UID:-1000}
GROUP_ID=${LOCAL_GID:-1000}

[[ "$USER_ID" == "1000" ]] || usermod -u $USER_ID -o -m -d /home/user user
[[ "$GROUP_ID" == "1000" ]] || groupmod -g $GROUP_ID user
exec /usr/sbin/gosu user "$@"
