
#include <lwip/init.h>
#include <lwip/ip_addr.h>
#include <lwip/priv/tcp_priv.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/ip4_frag.h>
#include <lwip/nd6.h>
#include <lwip/ip6_frag.h>
#include <lwip/tcpip.h>
#include <time.h>

#include "cs.h"
#include "tcp.h"
#include "udp.h"

/* -----------tap-------------- */
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#define IFCONFIG_BIN "/sbin/ifconfig "

#if __APPLE__
#include <fcntl.h>
#endif

#if defined(LWIP_UNIX_LINUX)
#include <sys/ioctl.h>
#include <sys/socket.h> // <-- This one
#include <linux/if.h>
#include <linux/if_tun.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
/*
 * Creating a tap interface requires special privileges. If the interfaces
 * is created in advance with `tunctl -u <user>` it can be opened as a regular
 * user. The network must already be configured. If DEVTAP_IF is defined it
 * will be opened instead of creating a new tap device.
 *
 * You can also use PRECONFIGURED_TAPIF environment variable to do so.
 */
#ifndef DEVTAP_DEFAULT_IF
#define DEVTAP_DEFAULT_IF "tap0"
#endif
#ifndef DEVTAP
#define DEVTAP "/dev/net/tun"
#endif
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#elif defined(LWIP_UNIX_OPENBSD)
#define DEVTAP "/dev/tun0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tun0 inet %d.%d.%d.%d " NETMASK_ARGS " link0"
#else /* others */
#define DEVTAP "/dev/tap0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 't'
#define IFNAME1 'p'

#ifndef TAPIF_DEBUG
#define TAPIF_DEBUG LWIP_DBG_OFF
#endif

/* ------------------------- */

#define LWIP_PORT_INIT_IPADDR(addr) IP4_ADDR((addr), 192, 168, 100, 200)
#define LWIP_PORT_INIT_GW(addr) IP4_ADDR((addr), 192, 168, 100, 1)
#define LWIP_PORT_INIT_NETMASK(addr) IP4_ADDR((addr), 255, 255, 255, 0)

int tap_fd = -1;

/*-----------------------------------------------------------------------------------*/
err_t cs_tap_init(struct netif *netif)
{
#if LWIP_IPV4
    int ret;
    char buf[1024];
#endif /* LWIP_IPV4 */

    tap_fd = open(DEVTAP, O_RDWR);
    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: fd %d\n", tapif->fd));
    if (tap_fd == -1)
    {
#ifdef LWIP_UNIX_LINUX
        perror("tapif_init: try running \"modprobe tun\" or rebuilding your kernel with CONFIG_TUN; cannot open " DEVTAP);
#else  /* LWIP_UNIX_LINUX */
        perror("tapif_init: cannot open " DEVTAP);
#endif /* LWIP_UNIX_LINUX */
        exit(1);
    }

#ifdef LWIP_UNIX_LINUX
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, DEVTAP_DEFAULT_IF, sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = 0; /* ensure \0 termination */

        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
        if (ioctl(tap_fd, TUNSETIFF, (void *)&ifr) < 0)
        {
            perror("tapif_init: " DEVTAP " ioctl TUNSETIFF");
            exit(1);
        }
    }
#endif /* LWIP_UNIX_LINUX */

#if LWIP_IPV4
    snprintf(buf, 1024, IFCONFIG_BIN IFCONFIG_ARGS,
             ip4_addr1(netif_ip4_gw(netif)),
             ip4_addr2(netif_ip4_gw(netif)),
             ip4_addr3(netif_ip4_gw(netif)),
             ip4_addr4(netif_ip4_gw(netif))
#ifdef NETMASK_ARGS
                 ,
             ip4_addr1(netif_ip4_netmask(netif)),
             ip4_addr2(netif_ip4_netmask(netif)),
             ip4_addr3(netif_ip4_netmask(netif)),
             ip4_addr4(netif_ip4_netmask(netif))
#endif /* NETMASK_ARGS */
    );

    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: system(\"%s\");\n", buf));
    ret = system(buf);
    if (ret < 0)
    {
        perror("ifconfig failed");
        exit(1);
    }
    if (ret != 0)
    {
        printf("ifconfig returned %d\n", ret);
    }
#else  /* LWIP_IPV4 */
    perror("todo: support IPv6 support for non-preconfigured tapif");
    exit(1);
#endif /* LWIP_IPV4 */
    return ERR_OK;
}

err_t cs_tcp_accept(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state)
{
    printf("---->cs_tcp_accept\n");
    return ERR_OK;
}
err_t cs_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state)
{
    // printf("---->cs_tcp_recv\n");
    // if (state->p == NULL)
    // {
    //     state->p = p;
    //     cs_tcp_raw_send(tpcb, state);
    // }
    // else
    // {
    //     struct pbuf *ptr;
    //     ptr = state->p;
    //     pbuf_cat(ptr, p);
    // }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}
void cs_tcp_send_done(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
}
void cs_tcp_close(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    printf("---->cs_tcp_close\n");
}
err_t cs_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    // printf("---->cs_udp_recv\n");
    cs_udp_sendto(upcb, p, addr, port);
    pbuf_free(p);
    return ERR_OK;
}
ssize_t cs_output(void *arg, struct netif *netif, const char *buf, u16_t len)
{
    // printf("---->cs_output\n");
    return write(tap_fd, buf, len);
}
struct pbuf *cs_input(void *arg, struct netif *netif, u16_t *readlen)
{
    char buf[1518];
    struct pbuf *p;
    ssize_t len;
    len = read(tap_fd, buf, 1518);
    if (len < 1)
    {
        *readlen = 0;
        return NULL;
    }
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL)
    {
        pbuf_take(p, buf, len);
    }
    return p;
}
void cs_input_free(void *arg, struct netif *netif, char *buf)
{
    free(buf);
}

void tcpecho_raw_init(void);

int main()
{
    // init lwip
    lwip_init();
    // tcpip_init(NULL,NULL);

    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    LWIP_PORT_INIT_GW(&gw);
    LWIP_PORT_INIT_IPADDR(&addr);
    LWIP_PORT_INIT_NETMASK(&netmask);
    struct netif netif;
    struct cs_callback back;
    back.tcp_accept = cs_tcp_accept;
    back.tcp_recv = cs_tcp_recv;
    back.tcp_send_done = cs_tcp_send_done;
    back.tcp_close = cs_tcp_close;
    back.udp_recv = cs_udp_recv;
    back.output = cs_output;
    back.input = cs_input;
    back.netif = &netif;
    // init netif
    if (!netif_add(&netif, &addr, &netmask, &gw, &back, cs_netif_init, ethernet_input))
    {
        goto fail;
    }
    // set netif default
    netif_set_default(&netif);

    // set netif up
    netif_set_up(&netif);

    // set netif link up, otherwise ip route will refuse to route
    // netif_set_link_up(&netif);
    // tcpecho_raw_init();
    cs_tcp_raw_init(&back);
    // cs_udp_raw_init(&back);
    cs_tap_init(&netif);
    // udpecho_raw_init();
    while (1)
    {
        cs_netif_input(&netif);
        // default_netif_poll();
        // tapif_poll(&netif);
        netif_poll_all();
    }
fail:
    printf("all done");
    return 1;
}
