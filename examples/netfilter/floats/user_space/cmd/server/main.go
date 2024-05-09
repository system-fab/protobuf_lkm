package main

import (
	"encoding/binary"
	"floats/gen"
	"fmt"
	"net"

	"google.golang.org/protobuf/proto"
)

const (
	port int = 60001
)

func main() {

	address := fmt.Sprintf(":%d", port)

	// Listen on TCP port 60001
	ln, err := net.Listen("tcp4", address)
	if err != nil {
		
		fmt.Println("Error listening:", err)
		return
	}
	defer ln.Close()

	fmt.Printf("listening on [%s]...\n", address)

	for {
		// Accept a connection
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err)
			continue
		}
		defer conn.Close()

		// Create a new message object
		message := &gen.Foo{}

		// Read the message length
		var messageLength uint32
		if err := binary.Read(conn, binary.BigEndian, &messageLength); err != nil {
			
			fmt.Println("Error reading message length:", err)
			return
		}

		fmt.Println("Received length:", messageLength)

		// Create a buffer to hold the entire data (length + payload)
		buffer := make([]byte, messageLength)

		// Read the complete data (including length)
		if _, err = conn.Read(buffer); err != nil {
			
			fmt.Println("Error reading message data:", err)
			return
		}

		// Now you can use proto.Unmarshal on the extracted payload
		err = proto.Unmarshal(buffer, message)
		if err != nil {
			fmt.Println("Error unmarshalling message:", err)
			return
		}

		// Process the message (replace with your logic)
		fmt.Printf("Received message: %v\n", message)
	}
}