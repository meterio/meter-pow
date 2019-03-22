FROM dfinlab/pow-build as builder

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
