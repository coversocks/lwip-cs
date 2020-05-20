

#include "src/cs.h"

#ifndef LWIP_GO_CS_H
#define LWIP_GO_CS_H

err_t go_cs_tcp_accept_h(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state);
err_t go_cs_tcp_recv_h(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state);
void go_cs_tcp_send_done_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
void go_cs_tcp_close_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state);
err_t go_cs_udp_recv_h(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
struct pbuf *go_cs_input_h(void *arg, struct netif *netif, u16_t *readlen);
ssize_t go_cs_output_h(void *arg, struct netif *netif, const char *buf, u16_t len);
void go_cs_init_handle(struct cs_callback *back);

typedef void (*go_cs_tcp_recved_fn)(struct tcp_pcb *tpcb, u16_t len);
typedef int (*go_cs_tcp_send_fn)(struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state, char *buf, int buf_len);
typedef err_t (*go_cs_tcp_close_fn)(struct tcp_pcb *tpcb);
typedef int (*go_cs_udp_send_fn)(struct udp_pcb *updb, ip_addr_t *laddr, u16_t lport, ip_addr_t *raddr, uint16_t rport, char *buf, int buf_len);
typedef void (*go_cs_ip_get_fn)(const ip_addr_t *addr_, char *buf);
typedef struct pbuf *(*go_cs_pbuf_alloc_fn)(pbuf_layer layer, u16_t length, pbuf_type type);
typedef err_t (*go_cs_pbuf_take_fn)(struct pbuf *buf, const void *dataptr, u16_t len);
typedef u16_t (*go_cs_pbuf_copy_fn)(const struct pbuf *p, void *dataptr, u16_t len, u16_t offset);
typedef u8_t (*go_cs_pbuf_free_fn)(struct pbuf *p);
typedef void (*go_cs_netif_proc_fn)(struct netif *netif);
typedef ssize_t (*go_cs_input_fn)(void *arg, char *buf, u16_t buflen);
typedef ssize_t (*go_cs_output_fn)(void *arg, const char *buf, u16_t len);

typedef struct
{
  void *state;
  struct netif *netif;
  go_cs_tcp_recved_fn tcp_recved;
  go_cs_tcp_send_fn tcp_send;
  go_cs_tcp_close_fn tcp_close;
  go_cs_udp_send_fn udp_send;
  go_cs_ip_get_fn ip_get;
  go_cs_pbuf_alloc_fn pbuf_alloc;
  go_cs_pbuf_take_fn pbuf_take;
  go_cs_pbuf_copy_fn pbuf_copy;
  go_cs_pbuf_free_fn pbuf_free;
  go_cs_netif_proc_fn netif_proc;
  go_cs_input_fn input;
  go_cs_output_fn output;
} go_cs_callback;

void go_cs_tcp_recved(void *tpcb_, u16_t len);
int go_cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len);
int go_cs_tcp_close(void *tpcb_, void *state_);
int go_cs_udp_send(void *upcb_, ip_addr_t *laddr, u16_t lport, ip_addr_t *raddr, uint16_t rport, char *buf, int buf_len);
ip_addr_t go_cs_tcp_local_ip(void *tpcb_);
int go_cs_tcp_local_port(void *tpcb_);
ip_addr_t go_cs_tcp_remote_ip(void *tpcb_);
int go_cs_tcp_remote_port(void *tpcb_);
ip_addr_t go_cs_udp_local_ip(void *upcb_);
int go_cs_udp_local_port(void *upcb_);
int go_cs_ip_len(ip_addr_t *addr_);
void go_cs_ip_get(ip_addr_t *addr_, char *buf);
int go_cs_pbuf_len(void *p_);
void *go_cs_pbuf_alloc(u16_t len);
void go_cs_pbuf_take(void *p_, char *buf);
void go_cs_pbuf_copy(void *p_, char *buf);
void go_cs_pbuf_free(void *p_);
void *go_cs_netif_get();
void go_cs_netif_proc(void *netif_);
ssize_t go_cs_input(char *buf, u16_t buflen);
void go_cs_init(go_cs_callback *back);

#ifndef _CGO_BUILD_
extern int go_tcp_accept_h(void *arg, void *newpcb, void *state);
extern int go_tcp_recv_h(void *arg, void *tpcb, void *state, void *p);
extern int go_tcp_send_done_h(void *arg, void *tpcb, void *state);
extern int go_tcp_close_h(void *arg, void *tpcb, void *state);
extern int go_udp_recv_h(void *arg, void *upcb, const ip_addr_t *addr, int port, void *p);
extern void *go_input_h(void *arg, void *netif, u16_t *readlen);
extern void go_netif_proc(void *netif_);
#endif

#endif /* LWIP_GO_CS_H */
