package main

import (
	"encoding/binary"
	"fmt"
	"net"

	"address_book/gen"

	"google.golang.org/protobuf/proto"
)

const (
	port int = 60001
)

func main() {

	// create protobuf
	person := gen.Person{
		Name: "Kalle Kula",
		Id: 56,
		Email: "kalle.kula@foobar.com",
		Phones: []*gen.PhoneNumber{
			{
				Number: "+46701232345",
				Type: gen.PhoneType_HOME,
			},
			{
				Number: "+46999999999",
				Type: gen.PhoneType_WORK,
			},
		},
	}

	book := gen.AddressBook{
		People: []*gen.Person{
			&person,
		},
	}

	// Marshal the AddressBook into bytes
    data, err := proto.Marshal(&book)
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