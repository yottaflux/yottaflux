FROM amd64/ubuntu:20.04 AS base

LABEL maintainer="Yottaflux Developers"

# Mainnet P2P + RPC
EXPOSE 8559/tcp
EXPOSE 8558/tcp
# Testnet P2P + RPC
EXPOSE 18559/tcp
EXPOSE 18558/tcp

ENV DEBIAN_FRONTEND=noninteractive

#Install runtime dependencies
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	bash wget net-tools libminiupnpc17 \
	libevent-2.1 libevent-pthreads-2.1 \
	libboost-system1.71 libboost-filesystem1.71 libboost-chrono1.71 \
	libboost-program-options1.71 libboost-thread1.71 \
	libzmq5 && \
	apt-get clean

FROM base AS build

#Install build dependencies
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	bash net-tools build-essential libtool autotools-dev automake \
	pkg-config libssl-dev libevent-dev bsdmainutils python3 \
	libboost-system1.71-dev libboost-filesystem1.71-dev libboost-chrono1.71-dev \
	libboost-program-options1.71-dev libboost-test1.71-dev libboost-thread1.71-dev \
	libzmq3-dev libminiupnpc-dev && \
	apt-get clean

#Copy source dir
COPY . /home/yottaflux/build/Yottaflux/
WORKDIR /home/yottaflux/build/Yottaflux

#build db4 from source
WORKDIR /home/yottaflux/build/Yottaflux/contrib
RUN ./install_db4.sh ../../

# build Yottaflux
WORKDIR /home/yottaflux/build/Yottaflux
RUN ./autogen.sh && ./configure --disable-tests BDB_LIBS="-L/home/yottaflux/build/db4/lib -ldb_cxx-4.8" BDB_CFLAGS="-I/home/yottaflux/build/db4/include" --with-gui=no  && make -j4

FROM base AS final

#Add our service account user
RUN useradd -ms /bin/bash yottaflux && \
	mkdir /var/lib/yottaflux && \
	chown yottaflux:yottaflux /var/lib/yottaflux && \
	ln -s /var/lib/yottaflux /home/yottaflux/.yottaflux && \
	chown -h yottaflux:yottaflux /home/yottaflux/.yottaflux

VOLUME /var/lib/yottaflux

#Copy the compiled binaries from the build
COPY --from=build /home/yottaflux/build/Yottaflux/src/yottafluxd /usr/local/bin/yottafluxd
COPY --from=build /home/yottaflux/build/Yottaflux/src/yottaflux-cli /usr/local/bin/yottaflux-cli

#Copy entrypoint script
COPY contrib/docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Runtime network selection: "mainnet" (default) or "testnet"
ENV NETWORK=mainnet

WORKDIR /home/yottaflux
USER yottaflux

HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
	CMD yottaflux-cli -datadir=/var/lib/yottaflux \
	$([ "$NETWORK" = "testnet" ] && echo "-testnet") \
	getblockchaininfo > /dev/null 2>&1 || exit 1

ENTRYPOINT ["docker-entrypoint.sh"]
