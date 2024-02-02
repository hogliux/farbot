#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "$DIR/common.sh"

docker build    --tag $DOCKER_IMAGE_TAG \
                --build-arg UID=$(id -u) \
                --build-arg GID=$(id -g) \
                --build-arg BUILD_USER=$BUILD_USER \
                $DIR
ret=$?

if [[ -n "$(docker ps --all -q -f status=exited)" ]] ; then
    docker rm $(docker ps --all -q -f status=exited)
fi

if [[ -n "$(docker images --filter "dangling=true" -q --no-trunc)" ]] ; then
    docker rmi $(docker images --filter "dangling=true" -q --no-trunc)
fi

exit $ret
