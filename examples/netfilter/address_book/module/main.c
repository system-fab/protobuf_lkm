// SPDX-License-Identifier: MIT

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/skbuff.h>

#include "../../../common/address_book/generated/address_book.h"

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("address_book_nf: some description");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#define MY_PORT          60001
#define BUFFER_SIZE 1024

//struct holding set of hook function options
static struct nf_hook_ops nfho;

// Function to print payload
void print_hex(const unsigned char *payload, unsigned int payload_len)
{
    unsigned int i;
    for (i = 0; i < payload_len; ++i) {
        pr_info("%02x -> %c\n", payload[i], payload[i]);
    }
    pr_info("\n");
}

unsigned int process_message(char *buffer, int size)
{

    uint8_t workspace[1024];

    struct address_book_address_book_t *address_book_p;
    struct address_book_person_t *person_p;
    struct address_book_phone_number_t *phone_number_p;

    /*
     * Decode the message.
     */
    address_book_p = address_book_address_book_new(&workspace[0], sizeof(workspace));

    address_book_address_book_decode(address_book_p, buffer, size);
    pr_info("people.length: %d", address_book_p->people.length);

    /*
     * checks:
     * Drop pkg if first person does not have 2 WORK phone numbers
     */

    // counter x matches
    int match = 0;

    // get first person
    person_p = &address_book_p->people.items_p[0];
    pr_info("person_p->id: %d", person_p->id);

    // get number of phones
    int phones_len = person_p->phones.length;
    pr_info("phones_len: %d", phones_len);
    WARN_ON(phones_len != 5);

    // cycle across phone numbers
    for(int i = 0; i < phones_len; i++ ) {

        phone_number_p = &person_p->phones.items_p[i];

        //pr_info("[type: %d, phone: %s]", phone_number_p->type, phone_number_p->number_p);

        if (phone_number_p->type == address_book_work_e ) {

            pr_info("Found match: [type: %d, phone: %s]", phone_number_p->type, phone_number_p->number_p);
            match++;
        }
    }

    // if match < 2 -> DROP
    if (match < 2) {

        pr_warn("dropped pkg");
        return NF_DROP;
    }

    pr_warn("accepted pkg");
    return NF_ACCEPT;
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

unsigned int handle_tcp_payload(struct sk_buff *skb)
{
    struct iphdr *ip;
    struct tcphdr *tcp;

    unsigned char *buffer;

    ip = ip_hdr(skb); // Update IP header pointer
    tcp = tcp_hdr(skb); // Update TCP header pointer

    uint32_t payload_len = ntohs(ip->tot_len) - (ip->ihl * 4) - (tcp->doff * 4);
    pr_info("Payload length: %d\n", payload_len);

    if (payload_len > 0) {

        buffer = (unsigned char *)(tcp) + (tcp->doff * 4);

        // extract size of protobuf from buffer
        int size = extract_message_size(buffer);

        if (size > 0) {

            // Process received data as needed
            return process_message(buffer + 4, size);
        }
    }

    return NF_ACCEPT;
}

unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct iphdr *ip;
    struct tcphdr *tcp;

    int non_linear = 0;
    struct sk_buff *skb_linear;

    if (!skb) return NF_ACCEPT;

    ip = ip_hdr(skb);

    // consider only tcp
    if (ip->protocol == IPPROTO_TCP && ip->daddr == htonl(INADDR_LOOPBACK)) {

        // get header tcp
        tcp = tcp_hdr(skb);

        // Access fields of TCP header
        uint16_t src_port = ntohs(tcp->source);
        uint16_t dest_port = ntohs(tcp->dest);

        if (dest_port == MY_PORT && tcp->psh) {

            pr_info("Received TCP packet. Source Port:%d, Destination Port:%d PUSH: %d\n", src_port, dest_port, tcp->psh);

            // check if linear
            non_linear = skb_is_nonlinear(skb);
            if (non_linear) {

                //pr_info("is_nonlinear: %d", non_linear);

                skb_linear = skb_copy(skb, GFP_ATOMIC); // Make a copy to preserve the original skb
                if (!skb_linear) {
                    printk(KERN_ERR "Failed to allocate linearized skb\n");
                    return NF_DROP; // Drop packet if copy fails
                }

                // linearize
                int ret = skb_linearize(skb_linear);
                if (ret != 0) {

                    printk(KERN_ERR "Failed to linearize skb\n");
                    kfree_skb(skb_linear); // Free the linearized skb
                    return NF_DROP; // Drop packet if linearization fails
                }

                // Process the linearized skb
                return handle_tcp_payload(skb_linear);

            } else {

                return handle_tcp_payload(skb);
            }
        }
    }

    return NF_ACCEPT;
}

static int __init address_book_nf_init(void)
{
    pr_info("Loaded module\n");

    nfho.hook       = (nf_hookfn*)hook_func;    /* hook function */
    nfho.hooknum    = NF_INET_LOCAL_IN;         /* packets to this machine */
    nfho.pf         = PF_INET;                  /* IPv4 */
    nfho.priority   = NF_IP_PRI_FIRST;          /* min hook priority */

    nf_register_net_hook(&init_net, &nfho);
    pr_info("Registered nethook function\n");

    return 0;
}

static void __exit address_book_nf_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("Removed module\n");
}

module_init(address_book_nf_init);
module_exit(address_book_nf_exit);
