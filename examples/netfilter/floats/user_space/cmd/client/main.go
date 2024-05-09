package main

import (
	"encoding/binary"
	"fmt"
	"net"
	"time"

	"floats/gen"

	"google.golang.org/protobuf/proto"
)

const (
	port int = 60001
)

func main() {

	for {

		fmt.Println("input number:")
		
		var input float32
		
		if _, err := fmt.Scanf("%f", &input); err != nil {
			panic(err)
		}

		fmt.Printf("read float: %f\n", input)

		// create protobuf
		foo := gen.Foo{
			Bar: input,
		}

		// Connect to TCP server
		address := fmt.Sprintf(":%d", port)
		conn, err := net.Dial("tcp4", address)
		if err != nil {
			panic(err)
		}
		defer conn.Close()

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

		// Send the data over the connection
		_, err = conn.Write(buffer)
		if err != nil {
			fmt.Println("Error sending data:", err)
			return
		}

		fmt.Printf("Sent message to %s\n", address)

		// Wait for 5 seconds before next send/receive
		time.Sleep(5 * time.Second)
	}
	
}