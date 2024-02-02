#!/bin/bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
REPO_DIR=$(readlink -f $SCRIPT_DIR/..)



DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "$DIR/common.sh"

docker run \
  --rm \
  --network=host \
  --mount type=bind,source=$HOME/.gitconfig,target=/home/build-user/.gitconfig \
  --mount type=bind,source=$HOME/.local,target=/home/$BUILD_USER/.local \
  --mount type=bind,source=$REPO_DIR,target=/home/$BUILD_USER/farbot \
  -ti $DOCKER_IMAGE_TAG:latest /bin/zsh

## logging in to a running container:
## get id with: docker ps
## docker exec -it <id> /bin/bash
