#!/bin/bash

# exit if fails
set -e

# where am i ?
FULL_PATH="$(readlink -f $0)"
LOCAL_PATH="$(dirname $FULL_PATH)"

# PATHS
cd "$LOCAL_PATH"/..
ROOT_PATH="$(pwd)"
PROTO_PATH="$ROOT_PATH"/../../../common/address_book/proto/
OUT_PATH="$ROOT_PATH"/gen

# install go plugins
go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

PATH="$PATH:$(go env GOPATH)/bin"
export PATH

protoc --proto_path="$PROTO_PATH" --go_out="$OUT_PATH" --go_opt=paths=source_relative --go-grpc_out="$OUT_PATH" --go-grpc_opt=paths=source_relative "$PROTO_PATH"/address_book.proto
