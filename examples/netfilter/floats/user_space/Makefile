PROTO_PATH = proto
OUT_PATH = gen

build:
	go build -o bin/server cmd/server/main.go
	go build -o bin/client cmd/client/main.go

client:
	go run cmd/client/main.go

server:
	go run cmd/server/main.go

requirements:
	apt install -y protobuf-compiler golang

generate:
	rm -rf gen
	mkdir gen
	scripts/gen_grpc_classes.sh