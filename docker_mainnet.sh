#!/bin/bash
DOCKER_REPO=meterio/pow
VERSION=1.2.0

VERSION_TAG=${DOCKER_REPO}:$VERSION
STATIC_TAG=${DOCKER_REPO}:mainnet

docker build -t $VERSION_TAG .
docker tag $VERSION_TAG $STATIC_TAG
echo "Built docker image ${VERSION_TAG} & ${STATIC_TAG}"

docker system prune -f
echo "Start to push docker image to DockerHub"
docker push $DOCKER_REPO