#include "src/cs.h"

err_t cs_tcp_accept(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state);
err_t cs_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state);
void cs_tcp_close(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
err_t cs_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
