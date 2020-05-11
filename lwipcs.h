#include "src/cs.h"

err_t go_cs_tcp_accept_h(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state);
err_t go_cs_tcp_recv_h(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state);
void go_cs_tcp_send_done_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
void go_cs_tcp_close_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
err_t go_cs_udp_recv_h(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
struct pbuf *go_cs_input_h(void *arg, struct netif *netif, u16_t *readlen);
void go_cs_input_free_h(void *arg, struct netif *netif, char *buf);
ssize_t go_cs_output_h(void *arg, struct netif *netif, const char *buf, u16_t len);
