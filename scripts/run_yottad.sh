#!/bin/bash

# Pull and build the latest version
cd /home/ubuntu/yottaflux || exit

sudo -u ubuntu git -C /home/ubuntu/yottaflux pull

systemctl daemon-reload

make
make install

# check for instance metadata

if [ -z "${ENVIRONMENT}" ]; then
  ENVIRONMENT=$(curl --silent --fail http://169.254.169.254/latest/meta-data/tags/instance/environment)
  CURL_RETURN=$?

  if [ $CURL_RETURN -ne 0 ]; then
    unset ENVIRONMENT
  fi;

fi;

MY_IP=$(curl --silent http://169.254.169.254/latest/meta-data/public-ipv4)
AZ=$(curl  --silent http://169.254.169.254/latest/meta-data/placement/availability-zone)
REGION=${AZ::-1}

echo Configuring yotta seed for IP $MY_IP , AZ $AZ , in region $REGION - $ENVIRONMENT mode
mkdir /yottaflux || true

if [ -z "${ENVIRONMENT}" ]; then
  cp scripts/yottaflux.conf /yottaflux/yottaflux.conf
  exec yottafluxd -printtoconsole -datadir=/yottaflux
elif [ "${ENVIRONMENT}" == "dev" ]; then
  cp scripts/yottaflux_test.conf /yottaflux/yottaflux.conf
  exec yottafluxd -testnet -printtoconsole -datadir=/yottaflux
elif [ "${ENVIRONMENT}" == "prod" ]; then
  cp scripts/yottaflux.conf /yottaflux/yottaflux.conf
  exec yottafluxd -printtoconsole -datadir=/yottaflux
else
  echo "ERROR: ENVIRONMENT supplied but invalid."
fi

