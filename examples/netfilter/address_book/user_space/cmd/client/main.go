package main

import (
	"address_book/gen"
	"encoding/binary"
	"fmt"
	"math/rand"
	"net"
	"time"

	"google.golang.org/protobuf/proto"
)

const (
	port int = 60001
)

func GetInput() *gen.AddressBook {

	ab := gen.AddressBook{}

	for i := 0; i < 3; i++ {
		
		p := gen.Person{
			Name: fmt.Sprintf("Person%d", i),
			Id: int32(i),
			Email: fmt.Sprintf("person%d@mail.example", i),
		}

		// phones
		for k := 0; k < 5; k++ {
			
			pn := gen.PhoneNumber{}

			// get type pseudo-casually
			t := rand.Int() %3
			pn.Type = gen.PhoneType(int32(t))

			// gen phone number
			for w := 0; w < 10; w++ {
				
				// get 1 digit number pseudo-casually
				d := rand.Int() % 10

				// convert digit into string
				c := fmt.Sprintf("%d", d)

				// append to number
				pn.Number = pn.Number + c
			}

			p.Phones = append(p.Phones, &pn)
		}


		// add to slice
		ab.People = append(ab.People, &p)
	}

	return &ab
}

func main() {

	for {

		ab := GetInput()

		// Connect to TCP server
		address := fmt.Sprintf(":%d", port)
		conn, err := net.Dial("tcp4", address)
		if err != nil {
			panic(err)
		}
    	defer conn.Close()

		// Marshal the Foo into bytes
		data, err := proto.Marshal(ab)
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

		fmt.Printf("Sent message to %s [%v]\n", address, ab)

		// Wait for 5 seconds before next send/receive
		time.Sleep(5 * time.Second)
	}

}