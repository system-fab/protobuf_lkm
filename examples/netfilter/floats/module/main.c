
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
#include <asm/uaccess.h>
#include <asm/fpu/api.h>

#include "../../../common/floats/generated/floats.h"

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("floats_nf: some description");
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
    uint8_t workspace[BUFFER_SIZE];

    struct hello_world_foo_t *hello_world_str;
    unsigned long flags;

    /* 
     * Decode the message.
     */
    hello_world_str = hello_world_foo_new(&workspace[0], sizeof(workspace));
    WARN_ON(hello_world_str == NULL);

    hello_world_foo_decode(hello_world_str, buffer, size);

    /*
     * checks:
     * Drop pkg if float bigger than 10.1
     */

    //pr_info("%u", hello_world_str->bar);
    
    // Save the current FPU state
    local_irq_save(flags);
    kernel_fpu_begin();

    union FloatConverter {
        uint32_t raw_value;
        float float_value;
    };

    union FloatConverter converter;
    converter.raw_value = hello_world_str->bar;

    float barf = converter.float_value;
    pr_info("bar_i: %d", (int) barf);
    //int bari = (int)barf;
    //pr_info("bari: %d", bari);
    
    //int treshold = 10;
    float treshold_f = 10.1;

    //int discard_i = (bari > treshold);
    int discard_f = (barf > treshold_f);

    //pr_info("is_discarded_int: %d", discard_i);
    pr_info("to_discard: %d", discard_f);
    
    // Restore the saved FPU state
    kernel_fpu_end();
    local_irq_restore(flags);
    
    if (discard_f) {

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

static int __init floats_nf_init(void)
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

static void __exit floats_nf_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("Removed module\n");
}

module_init(floats_nf_init);
module_exit(floats_nf_exit);
