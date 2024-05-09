build:
	go build main.go

run:
	go run main.go

requirements:
	apt install -y protobuf-compiler golang

generate:
	rm -rf gen
	mkdir gen
	scripts/gen_grpc_classes.sh
