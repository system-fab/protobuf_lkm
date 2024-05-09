package main

import (
	"encoding/binary"
	"fmt"
	"net"

	"hello_world/gen"

	"google.golang.org/protobuf/proto"
)

const (
	port int = 60001
)

func main() {

	for {

		fmt.Println("input number:")
    	var input int32
    	_, err := fmt.Scanf("%d", &input)
		if err != nil {
			panic(err)
		}

    	fmt.Printf("read int: %d\n", input)

		// create protobuf
		foo := gen.Foo{
			Bar: input,
		}

		// Marshal the Foo into bytes
		data, err := proto.Marshal(&foo)
		if err != nil {
			panic(err)
		}


		// Get the message length in bytes
        messageLength := len(data)
        fmt.Println("length:", messageLength)

        // Prepend the message length to the payload
        buffer := make([]byte, 4+messageLength)
        binary.BigEndian.PutUint32(buffer[:4], uint32(messageLength))
        copy(buffer[4:], data)

		// Connect to UDP server
		address := fmt.Sprintf("127.0.0.1:%d", port)
		conn, err := net.Dial("udp", address)
		if err != nil {
			panic(err)
		}
    	defer conn.Close()

		// Send the encoded data
		if _, err = conn.Write(buffer); err != nil {
			panic(err)
		}
	}
}