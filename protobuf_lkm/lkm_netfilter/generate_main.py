import os

MAIN_NF_FMT = '''
// SPDX-License-Identifier: Apache-2.0

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

#include "{import_path}"

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("{module_name}: some description");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#define MY_PORT          60001
#define BUFFER_SIZE 1024

//struct holding set of hook function options
static struct nf_hook_ops nfho;

// Function to print payload
void print_hex(const unsigned char *payload, unsigned int payload_len)
{{
    unsigned int i;
    for (i = 0; i < payload_len; ++i) {{
        pr_info("%02x -> %c\\n", payload[i], payload[i]);
    }}
    pr_info("\\n");
}}

unsigned int process_message(char *buffer, int size)
{{
    
    // example code
    pr_info("Encoded data:\\n");
    print_hex((unsigned char *)buffer, size);

    /*
     * TODO: Place your code here
     */

    return NF_ACCEPT;
}}

/**
 * This function reads the first 4 bytes of the payload extracting the size of the message,
 * if the size is bigger than the buffer size, returns -1
 */
static int extract_message_size(char *buffer)
{{
    // extract first 4 bytes (int) as protobuf size
    int size = (uint32_t) ntohl(*((uint32_t *) buffer));

    if (size < BUFFER_SIZE) {{

        pr_info("Found size: %u\\n", size);
        return size;
    }} else {{

        pr_err("Payload is too big for the specified buffer: [wanted: %u, got: %d]\\n", size, BUFFER_SIZE);
        return -1;
    }}
}}

unsigned int handle_tcp_payload(struct sk_buff *skb)
{{
    struct iphdr *ip;
    struct tcphdr *tcp;

    unsigned char *buffer;

    ip = ip_hdr(skb); // Update IP header pointer
    tcp = tcp_hdr(skb); // Update TCP header pointer

    uint32_t payload_len = ntohs(ip->tot_len) - (ip->ihl * 4) - (tcp->doff * 4);
    pr_info("Payload length: %d\\n", payload_len);

    if (payload_len > 0) {{

        buffer = (unsigned char *)(tcp) + (tcp->doff * 4);

        // extract size of protobuf from buffer
        int size = extract_message_size(buffer);

        if (size > 0) {{

            // Process received data as needed
            return process_message(buffer + 4, size);
        }}
    }}

    return NF_ACCEPT;
}}

unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{{
    struct iphdr *ip;
    struct tcphdr *tcp;

    int non_linear = 0;
    struct sk_buff *skb_linear;

    if (!skb) return NF_ACCEPT;

    ip = ip_hdr(skb);

    // consider only tcp
    if (ip->protocol == IPPROTO_TCP && ip->daddr == htonl(INADDR_LOOPBACK)) {{

        // get header tcp
        tcp = tcp_hdr(skb);

        // Access fields of TCP header
        uint16_t src_port = ntohs(tcp->source);
        uint16_t dest_port = ntohs(tcp->dest);

        if (dest_port == MY_PORT && tcp->psh) {{

            pr_info("Received TCP packet. Source Port:%d, Destination Port:%d PUSH: %d\\n", src_port, dest_port, tcp->psh);

            // check if linear
            non_linear = skb_is_nonlinear(skb);
            if (non_linear) {{

                //pr_info("is_nonlinear: %d", non_linear);

                skb_linear = skb_copy(skb, GFP_ATOMIC); // Make a copy to preserve the original skb
                if (!skb_linear) {{
                    printk(KERN_ERR "Failed to allocate linearized skb\\n");
                    return NF_DROP; // Drop packet if copy fails
                }}

                // linearize
                int ret = skb_linearize(skb_linear);
                if (ret != 0) {{

                    printk(KERN_ERR "Failed to linearize skb\\n");
                    kfree_skb(skb_linear); // Free the linearized skb
                    return NF_DROP; // Drop packet if linearization fails
                }}

                // Process the linearized skb
                return handle_tcp_payload(skb_linear);

            }} else {{

                return handle_tcp_payload(skb);
            }}
        }}
    }}

    return NF_ACCEPT;
}}

static int __init {module_name}_init(void)
{{
    pr_info("Loaded module\\n");

    nfho.hook       = (nf_hookfn*)hook_func;    /* hook function */
    nfho.hooknum    = NF_INET_LOCAL_IN;         /* packets to this machine */
    nfho.pf         = PF_INET;                  /* IPv4 */
    nfho.priority   = NF_IP_PRI_FIRST;          /* min hook priority */

    nf_register_net_hook(&init_net, &nfho);
    pr_info("Registered nethook function\\n");

    return 0;
}}

static void __exit {module_name}_exit(void)
{{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("Removed module\\n");
}}

module_init({module_name}_init);
module_exit({module_name}_exit);
'''


def generate_main_nf(module_name, import_path, output_directory):

    filename = os.path.join(output_directory, 'main.c')

    with open(filename, 'w') as f:
        f.write(MAIN_NF_FMT.format(module_name=module_name, import_path=import_path))

