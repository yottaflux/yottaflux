#!/bin/sh
set -e

NETWORK_FLAGS=""
if [ "$NETWORK" = "testnet" ]; then
    NETWORK_FLAGS="-testnet"
fi

exec yottafluxd \
    -datadir=/var/lib/yottaflux \
    -printtoconsole \
    -onlynet=ipv4 \
    $NETWORK_FLAGS \
    "$@"
