#!/bin/bash

# ubuntu@ip-10-0-1-11:~$ GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=false" git clone git@github.com:yottaflux/yottaflux.git



sudo sed -i "s/#\$nrconf{restart} = 'i';/\$nrconf{restart} = 'a';/" /etc/needrestart/needrestart.conf
sudo apt-get update
sudo apt-get -y --no-install-recommends install \
         build-essential \
         libssl-dev \
         libboost-all-dev \
         bison \
         libexpat1-dev \
         libdbus-1-dev \
         libice-dev \
         libsm-dev \
         libevent-dev \
         libminiupnpc-dev \
         libprotobuf-dev \
         libqrencode-dev \
         xcb-proto \
         zlib1g-dev \
         libczmq-dev \
         autoconf \
         automake \
         libtool \
         protobuf-compiler \
         wget \
         bsdmainutils \
         libdb++-dev \
         qtbase5-dev \
         qttools5-dev \
         libfontconfig-dev \
         libfreetype-dev \
         libx11-dev \
         libxau-dev \
         libxext-dev \
         libxcb1-dev \
         libxkbcommon-dev \
         x11proto-xext-dev \
         x11proto-dev \
         xtrans-dev

cd "${HOME}"/yottaflux || exit
./contrib/install_db4.sh "${HOME}"/
./autogen.sh
./configure BDB_LIBS="-L${HOME}/db4/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${HOME}/db4/include" \
    --disable-test --disable-bench --without-gui
make -j2
sudo make install

sudo ln -s /home/ubuntu/yottaflux/scripts/yotta.service /etc/systemd/system/yotta.service



