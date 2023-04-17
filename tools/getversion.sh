#!/bin/bash

if [[ -n "$1" ]]; then
    cd $1
fi
PREFIX=git
TAG=$(git tag --points-at HEAD)
COMMIT_SHA=$(git rev-parse --short HEAD)

if [[ -n "${TAG}" ]]; then
    echo "v${TAG}" 
    exit 0
elif [[ -n "${COMMIT_SHA}" ]]; then
    echo "${PREFIX}-${COMMIT_SHA}" 
    exit 0;
fi

exit 1
