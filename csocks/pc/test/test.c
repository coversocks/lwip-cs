#include "libcsocks.h"
#include <stdio.h>
#include <pthread.h>
#include "cs.h"
#include "tcp.h"
#include "udp.h"
#include "lwipcs.h"

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

int tap_fd = -1;

/*-----------------------------------------------------------------------------------*/
err_t test_tap_init(struct netif *netif)
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
ssize_t test_output(void *arg, const char *buf, u16_t len)
{
  return write(tap_fd, buf, len);
}
ssize_t test_input(void *arg, char *buf, u16_t len)
{
  // printf("----->input\n");
  ssize_t l=read(tap_fd, buf, len);
  // printf("----->input done %ld \n",l);
  return l;
}

ssize_t test_raw_output_h(void *arg, struct netif *netif, const char *buf, u16_t len)
{
  return test_output(arg, buf, len);
}

struct pbuf *test_raw_input_h(void *arg, struct netif *netif, u16_t *readlen)
{
  char buf[1518];
  struct pbuf *p;
  ssize_t len = test_input(arg, buf, 1518);
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

err_t test_tcp_accept(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state)
{
  printf("---->cs_tcp_accept\n");
  return ERR_OK;
}
err_t test_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state)
{
  printf("---->cs_tcp_recv\n");
  // if (state->send == NULL)
  // {
  //   state->send = p;
  //   cs_tcp_raw_send(tpcb, state);
  // }
  // else
  // {
  //   struct pbuf *ptr;
  //   ptr = state->send;
  //   pbuf_cat(ptr, p);
  // }
  tcp_recved(tpcb, p->tot_len);
  pbuf_free(p);
  return ERR_OK;
}
void test_tcp_send_done(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
}
void test_tcp_close(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
  printf("---->cs_tcp_close\n");
}

err_t test_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  // printf("---->cs_udp_recv\n");
  cs_udp_sendto(upcb, p, addr, port);
  pbuf_free(p);
  return ERR_OK;
}

int main()
{
  printf("starting test\n");
  go_cs_callback gback;
  gback.tcp_recved = tcp_recved;
  gback.tcp_send = cs_tcp_char_send;
  gback.tcp_close = tcp_close;
  gback.udp_send = cs_udp_char_sendto;
  gback.ip_get = cs_ip_get;
  gback.pbuf_alloc = pbuf_alloc;
  gback.pbuf_take = pbuf_take;
  gback.pbuf_copy = pbuf_copy_partial;
  gback.pbuf_free = pbuf_free;
  gback.netif_proc = cs_netif_proc;
  gback.input = test_input;
  gback.output = test_output;
  cs_callback back;
  struct netif netif;
  back.netif = &netif;
  gback.netif = &netif;
  gback.state = &back;
  //
  go_cs_init(&gback);
  go_cs_init_handle(&back);
  cs_init(&back);
  // back.tcp_accept = test_tcp_accept;
  // back.tcp_close = test_tcp_close;
  // back.tcp_send_done = test_tcp_send_done;
  // back.tcp_recv = test_tcp_recv;
  back.output=test_raw_output_h;
  //
  test_tap_init(&netif);
  //
  go_cs_proc(&netif);
}

#define LWIP_PORT_INIT_IPADDR(addr) IP4_ADDR((addr), 192, 168, 100, 200)
#define LWIP_PORT_INIT_GW(addr) IP4_ADDR((addr), 192, 168, 100, 1)
#define LWIP_PORT_INIT_NETMASK(addr) IP4_ADDR((addr), 255, 255, 255, 0)

// int main()
// {
//   // init lwip
//   lwip_init();
//   // tcpip_init(NULL,NULL);

//   ip4_addr_t addr;
//   ip4_addr_t netmask;
//   ip4_addr_t gw;
//   LWIP_PORT_INIT_GW(&gw);
//   LWIP_PORT_INIT_IPADDR(&addr);
//   LWIP_PORT_INIT_NETMASK(&netmask);
//   struct netif netif;
//   cs_callback back;
//   // back.tcp_accept = go_cs_tcp_accept_h;
//   // back.tcp_recv = go_cs_tcp_recv_h;
//   // back.tcp_send_done = go_cs_tcp_send_done_h;
//   // back.tcp_close = go_cs_tcp_close_h;
//   // back.udp_recv = go_cs_udp_recv_h;
//   back.tcp_accept = test_tcp_accept;
//   back.tcp_recv = test_tcp_recv;
//   back.tcp_send_done = test_tcp_send_done;
//   back.tcp_close = test_tcp_close;
//   back.udp_recv = test_udp_recv;

//   // back.input = go_cs_input_h;
//   // back.input_free = go_cs_input_free_h;
//   // back.output = go_cs_output_h;
//   back.input = test_raw_input_h;
//   back.output = test_raw_output_h;
//   back.netif = &netif;
//   // init netif
//   if (!netif_add(&netif, &addr, &netmask, &gw, &back, cs_netif_init, ethernet_input))
//   {
//     goto fail;
//   }

//   // set netif up
//   netif_set_up(&netif);

//   // set netif link up, otherwise ip route will refuse to route
//   netif_set_link_up(&netif);

//   // set netif default
//   netif_set_default(&netif);
//   cs_tcp_raw_init(&back);
//   cs_udp_raw_init(&back);
//   test_tap_init(&netif);
//   // pthread_t send_s;
//   // pthread_create(&send_s, NULL, test_send, &netif);
//   // pthread_t recv_s;
//   // pthread_create(&recv_s, NULL, test_recv, &netif);
//   // udpecho_raw_init();
//   while (1)
//   {
//     cs_netif_input(&netif);
//     // default_netif_poll();
//     // tapif_poll(&netif);
//     netif_poll_all();
//     // go_cs_proc(&netif);
//   }
// fail:
//   printf("all done");
//   return 1;
// }
