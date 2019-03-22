FROM ubuntu:18.04 as builder

RUN apt-get update && apt-get install -y build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils libboost-all-dev software-properties-common wget
RUN add-apt-repository ppa:bitcoin/bitcoin
RUN apt-get update && apt-get install -y libdb4.8-dev libdb4.8++-dev libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev

WORKDIR /btcpow
COPY . .

RUN cd /btcpow && ./autogen.sh && ./configure --disable-wallet --without-gui && make

# Pull thor into a second stage deploy ubuntu container
FROM ubuntu:18.04

COPY --from=builder /btcpow/src/bitcoind /usr/local/bin/
COPY --from=builder /btcpow/src/bitcoin-cli /usr/local/bin/
COPY --from=builder /btcpow/src/bitcoin-tx /usr/local/bin/

COPY --from=builder /usr/lib/x86_64-linux-gnu/libboost*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libssl*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libevent*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libcrypto*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libminiupnpc*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libzmq*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libstdc++*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libsodium*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libpgm*.so* /usr/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libnorm*.so* /usr/lib/
COPY --from=builder /usr/lib/libdb*.so* /usr/lib/

RUN mkdir /data
COPY bitcoin.conf /data/bitcoin.conf

EXPOSE 8332 9209
ENTRYPOINT ["bitcoind", "-datadir=/data"]
