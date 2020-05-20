#define LWIP_DEBUG 1

#include "lwipcs.h"
#include "tcp.h"
#include "udp.h"
#include <netif/ethernet.h>

go_cs_callback *go_cs_ = NULL;

err_t go_cs_tcp_accept_h(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state)
{
    return go_tcp_accept_h(arg, newpcb, state);
    // return ERR_OK;
}

err_t go_cs_tcp_recv_h(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state)
{
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
    // return ERR_OK;
    err_t ret = go_tcp_recv_h(arg, tpcb, state, p);
    go_cs_->pbuf_free(p);
    return ret;
}

void go_cs_tcp_send_done_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    go_tcp_send_done_h(arg, tpcb, state);
}

void go_cs_tcp_close_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    go_tcp_close_h(arg, tpcb, state);
}

err_t go_cs_udp_recv_h(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    err_t ret = go_udp_recv_h(arg, upcb, addr, port, p);
    go_cs_->pbuf_free(p);
    return ret;
}

ssize_t go_cs_output_h(void *arg, struct netif *netif, const char *buf, u16_t len)
{
    return go_cs_->output(go_cs_->state, buf, len);
}

struct pbuf *go_cs_input_h(void *arg, struct netif *netif, u16_t *readlen)
{
    // char buf[1518];
    // struct pbuf *p;
    // ssize_t len = go_cs_->input(go_cs_->state, buf, 1518);
    // if (len < 1)
    // {
    //     *readlen = 0;
    //     return NULL;
    // }
    // p = go_cs_->pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    // if (p != NULL)
    // {
    //     go_cs_->pbuf_take(p, buf, len);
    // }
    return go_input_h(arg, netif, readlen);
}

void go_cs_init_handle(struct cs_callback *back)
{
    back->tcp_accept = go_cs_tcp_accept_h;
    back->tcp_recv = go_cs_tcp_recv_h;
    back->tcp_send_done = go_cs_tcp_send_done_h;
    back->tcp_close = go_cs_tcp_close_h;
    back->udp_recv = go_cs_udp_recv_h;
    back->output = go_cs_output_h;
    back->input = go_cs_input_h;
}

void go_cs_tcp_recved(void *tpcb_, u16_t len)
{
    struct tcp_pcb *tpcb = tpcb_;
    return go_cs_->tcp_recved(tpcb, len);
}

int go_cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len)
{
    struct tcp_pcb *tpcb = tpcb_;
    struct cs_tcp_raw_state *state = state_;
    return go_cs_->tcp_send(tpcb, state, buf, buf_len);
}

int go_cs_tcp_close(void *tpcb_, void *state_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return go_cs_->tcp_close(tpcb);
}

int go_cs_udp_send(void *upcb_, ip_addr_t *laddr, u16_t lport, ip_addr_t *raddr, uint16_t rport, char *buf, int buf_len)
{
    return go_cs_->udp_send(upcb_, laddr, lport, raddr, rport, buf, buf_len);
}

ip_addr_t go_cs_tcp_local_ip(void *tpcb_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return tpcb->local_ip;
}

int go_cs_tcp_local_port(void *tpcb_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return tpcb->local_port;
}

ip_addr_t go_cs_tcp_remote_ip(void *tpcb_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return tpcb->remote_ip;
}

int go_cs_tcp_remote_port(void *tpcb_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return tpcb->remote_port;
}

ip_addr_t go_cs_udp_local_ip(void *upcb_)
{
    struct udp_pcb *upcb = upcb_;
    return upcb->local_ip;
}

int go_cs_udp_local_port(void *upcb_)
{
    struct udp_pcb *upcb = upcb_;
    return upcb->local_port;
}

int go_cs_ip_len(ip_addr_t *addr_)
{
    if (IP_GET_TYPE(addr_) == IPADDR_TYPE_V6)
    {
        return 16;
    }
    else
    {
        return 4;
    }
}

void go_cs_ip_get(ip_addr_t *addr_, char *buf)
{
    go_cs_->ip_get(addr_, buf);
}

int go_cs_pbuf_len(void *p_)
{
    struct pbuf *p = p_;
    return p->tot_len;
}

void *go_cs_pbuf_alloc(u16_t len)
{
    return go_cs_->pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
}

void go_cs_pbuf_take(void *p_, char *buf)
{
    struct pbuf *p = p_;
    go_cs_->pbuf_take(p, buf, p->tot_len);
}

void go_cs_pbuf_copy(void *p_, char *buf)
{
    struct pbuf *p = p_;
    go_cs_->pbuf_copy(p, buf, p->tot_len, 0);
}

void go_cs_pbuf_free(void *p_)
{
    struct pbuf *p = p_;
    go_cs_->pbuf_free(p_);
}

void *go_cs_netif_get()
{
    return go_cs_->netif;
}

void go_cs_netif_proc(void *netif_)
{
    struct netif *netif = netif_;
    return go_cs_->netif_proc(netif);
}

ssize_t go_cs_input(char *buf, u16_t buflen)
{
    return go_cs_->input(go_cs_->state, buf, buflen);
}

void go_cs_init(go_cs_callback *back)
{
    go_cs_ = back;
}
