
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

#ifndef LWIP_CS_H
#define LWIP_CS_H

struct cs_tcp_raw_state;

typedef err_t (*cs_tcp_accept_fn)(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state);
typedef err_t (*cs_tcp_recv_fn)(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state);
typedef void (*cs_tcp_close_fn)(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
typedef err_t (*cs_udp_recv_fn)(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
typedef err_t (*cs_udp_recv_fn)(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
typedef ssize_t (*cs_output_fn)(void *arg, struct netif *netif, const char *buf, u16_t len);
typedef ssize_t (*cs_input_fn)(void *arg, struct netif *netif, char *buf, u16_t len);

struct cs_callback
{
  void *state;
  cs_tcp_accept_fn tcp_accept;
  cs_tcp_recv_fn tcp_recv;
  cs_tcp_close_fn tcp_close;
  cs_udp_recv_fn udp_recv;
  cs_output_fn output;
  cs_input_fn input;
};

void cs_netif_input(struct netif *netif);
err_t cs_netif_init(struct netif *netif);
void cs_lwip_app_platform_assert(const char *msg, int line, const char *file);

#endif /* LWIP_CS_H */
