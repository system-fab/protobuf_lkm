
// SPDX-License-Identifier: MIT

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>
#include <net/sock.h>

#include "../../../common/hello_world/generated/hello_world.h"

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("hello_world_udp: some description");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#define MY_UDP_PORT         60001
#define LISTEN_BACKLOG      5       // queue length for port
#define BUFFER_SIZE         128

static struct socket *sock;    /* listening (server) socket */

static int process_message(char *buffer, int size)
{
    uint8_t workspace[1024];

    struct hello_world_foo_t *hello_world_str;

    /* Decode the message. */
    hello_world_str = hello_world_foo_new(&workspace[0], sizeof(workspace));
    WARN_ON(hello_world_str == NULL);

    hello_world_foo_decode(hello_world_str, buffer, size);

    return (int) hello_world_str->bar;
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

static int __init hello_world_udp_init(void)
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
    int bar = 0;

    do {

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
            bar = process_message(buffer + 4, size);
            pr_info("received: %d", bar);
        } else {

            pr_err("received empty package -> Bye");
            goto out_release;
        }

    } while (bar <= 5);

    pr_info("%d > 5 => exit the loop", bar);

    out_release:

        /* cleanup listening socket */
        sock_release(sock);

    out:
        return 0;
}

static void __exit hello_world_udp_exit(void)
{
    pr_info("removed\n");
}

module_init(hello_world_udp_init);
module_exit(hello_world_udp_exit);
