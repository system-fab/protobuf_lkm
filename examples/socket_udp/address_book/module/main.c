
// SPDX-License-Identifier: MIT

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>
#include <net/sock.h>

#include "../../../common/address_book/generated/address_book.h"

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("address_book_udp: some description");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#define MY_UDP_PORT         60001
#define LISTEN_BACKLOG      5       // queue length for port
#define BUFFER_SIZE         128

static struct socket *sock;    /* listening (server) socket */

static void process_message(char *buffer, int size)
{
    uint8_t workspace[1024];

    struct address_book_address_book_t *address_book_p;
    struct address_book_person_t *person_p;
    struct address_book_phone_number_t *phone_number_p;

    /* Decode the message. */
    address_book_p = address_book_address_book_new(&workspace[0], sizeof(workspace));
    if (address_book_p == NULL) {
        pr_err("address_book_p is null");
        return;
    }

    size = address_book_address_book_decode(address_book_p, buffer, size);
    if (address_book_p->people.length != 1) {
        pr_err("address_book_p->people.length = %d", address_book_p->people.length);
        return;
    }

    pr_info("people.length: %d", address_book_p->people.length);

    /* Check the decoded person. */
    person_p = &address_book_p->people.items_p[0];
    WARN_ON(strcmp(person_p->name_p, "Kalle Kula") != 0);
    WARN_ON(person_p->id != 56);
    pr_info("person_p->id: %d", person_p->id);

    WARN_ON(strcmp(person_p->email_p, "kalle.kula@foobar.com") != 0);
    WARN_ON(person_p->phones.length != 2);

    /* Check home phone number. */
    phone_number_p = &person_p->phones.items_p[0];
    WARN_ON(strcmp(phone_number_p->number_p, "+46701232345") != 0);
    WARN_ON(phone_number_p->type != address_book_home_e);
    pr_info("phone_number_p->number_p: %s", phone_number_p->number_p);

    /* Check work phone number. */
    phone_number_p = &person_p->phones.items_p[1];
    WARN_ON(strcmp(phone_number_p->number_p, "+46999999999") != 0);
    WARN_ON(phone_number_p->type != address_book_work_e);
}

/**
 * This function reads the first 4 bytes of the payload extracting the size of the message,
 * if the size is bigger than the buffer size, returns -1
 */
static int extract_message_size(char *buffer)
{
    // extract first 4 bytes (int) as protobuf size
    int size = (uint32_t) ntohl(*((uint32_t *) buffer));

    if (size < BUFFER_SIZE) {

        pr_info("Found size: %u\n", size);
        return size;
    } else {

        pr_err("Payload is too big for the specified buffer: [wanted: %u, got: %d]\n", size, BUFFER_SIZE);
        return -1;
    }
}

static int __init address_book_udp_init(void)
{
    int err;
    err = 0;

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    /* address to bind on */
    struct sockaddr_in addr = {
            .sin_family    = AF_INET,
            .sin_port    = htons(MY_UDP_PORT),
            .sin_addr    = { htonl(INADDR_LOOPBACK) }
    };

    sock = NULL;

    pr_info("Loaded module");

    // create listening socket
    err = sock_create_kern(&init_net, PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (err < 0) {

        pr_err("Could not create socket: %d", err);
        goto out;
    }

    // reset err
    err = 0;

    // bind socket to loopback on port
    err = sock->ops->bind (sock, (struct sockaddr *) &addr, sizeof(addr));
    if (err < 0) {

        /* handle error */
        pr_err("Could not bind socket: %d", err);
        goto out;
    }

    // reset err
    err = 0;

    // receive protobuf

    struct msghdr msg;
    struct kvec iov;

    memset(&msg, 0, sizeof(struct msghdr));
    memset(&iov, 0, sizeof(struct kvec));

    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    err = kernel_recvmsg(sock, &msg, &iov, 1, 1024, MSG_WAITALL);
    if (err < 0) {
        pr_err("Failed to receive UDP data (protobuf): %d\n", err);
        goto out_release;
    }

    // extract size of protobuf from buffer
    int size = extract_message_size(buffer);

    if (size > 0) {

        // Process received data as needed
        process_message(buffer + 4, size);
    }

    out_release:

        /* cleanup listening socket */
        sock_release(sock);

    out:
        return 0;
}

static void __exit address_book_udp_exit(void)
{
    pr_info("removed\n");
}

module_init(address_book_udp_init);
module_exit(address_book_udp_exit);
