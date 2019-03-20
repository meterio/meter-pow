#!/bin/bash

VERSION=1.0

DOCKER_TAG=dfinlab/pow:$VERSION
GIT_TAG=v$VERSION
TEMP_CONTAINER_NAME=powtemp
RELEASE_TARBALL=pow-$VERSION-linux-amd64.tar.gz
RELEASE_DIR=release/pow-$VERSION-linux-amd64
DEPENDENCY_TARBALL=pow-$VERSION-linux-amd64-dependency.tar.gz
DEPENDENCY_DIR=release/pow-$VERSION-linux-amd64-dependency

docker build -t $DOCKER_TAG .
docker run -d --name $TEMP_CONTAINER_NAME $DOCKER_TAG
echo "Brought up a temporary docker container"
mkdir -p $RELEASE_DIR
mkdir -p $DEPENDENCY_DIR
docker cp $TEMP_CONTAINER_NAME:/usr/local/bin/bitcoind $RELEASE_DIR/
docker cp $TEMP_CONTAINER_NAME:/usr/local/bin/bitcoin-cli $RELEASE_DIR/
docker cp $TEMP_CONTAINER_NAME:/usr/local/bin/bitcoin-tx $RELEASE_DIR/
docker cp $TEMP_CONTAINER_NAME:/usr/lib $DEPENDENCY_DIR/
docker rm --force $TEMP_CONTAINER_NAME
echo "Removed the temporary docker container"


tar -zcf release/$RELEASE_TARBALL $RELEASE_DIR
tar -zcf release/$DEPENDENCY_TARBALL $DEPENDENCY_DIR/lib
rm -rf $RELEASE_DIR

github-release release \
    --user dfinlab \
    --repo btcpow \
    --tag ${GIT_TAG} \
    --name "${GIT_TAG}" \
    --pre-release
echo "Created release ${GIT_TAG}"

echo "Start upload release/${RELEASE_TARBALL}"
github-release upload \
    --user dfinlab \
    --repo btcpow \
    --tag ${GIT_TAG} \
    --name "${RELEASE_TARBALL}" \
    --file release/$RELEASE_TARBALL

echo "Start upload release/${DEPENDENCY_TARBALL}"
github-release upload \
    --user dfinlab \
    --repo btcpow \
    --tag ${GIT_TAG} \
    --name "${DEPENDENCY_TARBALL}" \
    --file release/$DEPENDENCY_TARBALL
echo "Release uploaded, please check https://github.com/dfinlab/btcpow/releases"
