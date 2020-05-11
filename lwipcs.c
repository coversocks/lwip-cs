#define LWIP_DEBUG 1

#include "lwipcs.h"
#include "tcp.h"
#include "udp.h"
#include <netif/ethernet.h>

extern int go_tcp_accept_h(void *arg, void *newpcb, void *state);
extern int go_tcp_recv_h(void *arg, void *tpcb, void *state, void *p);
extern int go_tcp_send_done_h(void *arg, void *tpcb, void *state);
extern int go_tcp_close_h(void *arg, void *tpcb, void *state);
extern int go_udp_recv_h(void *arg, void *upcb, const ip_addr_t *addr, int port, void *p);

err_t go_cs_tcp_accept_h(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state)
{
    return go_tcp_accept_h(arg, newpcb, state);
    // return ERR_OK;
}

err_t go_cs_tcp_recv_h(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state)
{
    // err_t ret = go_tcp_recv_h(arg, tpcb, state, p);
    // pbuf_free(p);
    // tcp_recved(tpcb, p->tot_len);
    // return ERR_OK;
    if (state->send == NULL)
    {
        state->send = p;
        cs_tcp_raw_send(tpcb, state);
    }
    else
    {
        struct pbuf *ptr;
        ptr = state->send;
        pbuf_cat(ptr, p);
    }
    return ERR_OK;
}

void go_cs_tcp_send_done_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    // go_tcp_send_done_h(arg, tpcb, state);
}

void go_cs_tcp_close_h(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    go_tcp_close_h(arg, tpcb, state);
}

int go_cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len)
{
    struct tcp_pcb *tpcb = tpcb_;
    struct cs_tcp_raw_state *state = state_;
    if (state->send != NULL)
    {
        return 100;
    }
    struct pbuf *p = pbuf_alloc(PBUF_RAW, buf_len, PBUF_POOL);
    if (p == NULL)
    {
        return 1;
    }
    pbuf_take(p, buf, buf_len);
    if (state->send == NULL)
    {
        state->send = p;
        cs_tcp_raw_send(tpcb, state);
    }
    else
    {
        struct pbuf *ptr;
        ptr = state->send;
        pbuf_cat(ptr, p);
    }
    return 0;
}

int go_cs_tcp_close(void *tpcb_, void *state_)
{
    struct tcp_pcb *tpcb = tpcb_;
    return tcp_close(tpcb);
}

err_t go_cs_udp_recv_h(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    // cs_udp_sendto(upcb, p, addr, port);
    // pbuf_free(p);
    if (port != 5353)
    {
        // ip_addr_debug_print_val(LWIP_DBG_ON,*addr);
        // printf(":%ld\n",port);
        go_udp_recv_h(arg, upcb, addr, port, p);
    }
    pbuf_free(p);
    // printf("rr-%p,%p,%u\n",upcb,addr,port);
    return ERR_OK;
}

int go_cs_udp_send(void *upcb_, ip_addr_t *laddr, u16_t lport, ip_addr_t *raddr, uint16_t rport, char *buf, int buf_len)
{
    struct udp_pcb *upcb = upcb_;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, buf_len, PBUF_POOL);
    if (p == NULL)
    {
        return 1;
    }
    pbuf_take(p, buf, buf_len);
    ip_addr_t old_addr = upcb->local_ip;
    u16_t old_port = upcb->local_port;
    upcb->local_ip = *laddr;
    upcb->local_port = lport;
    cs_udp_sendto(upcb, p, raddr, rport);
    upcb->local_ip = old_addr;
    upcb->local_port = old_port;
    pbuf_free(p);
    return 0;
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
    ip_addr_t *addr = addr_;
    return cs_ip_len(addr);
}

void go_cs_ip_get(ip_addr_t *addr_, char *buf)
{
    ip_addr_t *addr = addr_;
    cs_ip_get(addr, buf);
}

int go_cs_pbuf_len(void *p_)
{
    struct pbuf *p = p_;
    return p->tot_len;
}

void *go_cs_pbuf_alloc(u16_t len)
{
    return pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
}

void go_cs_pbuf_take(void *p_, char *buf)
{
    struct pbuf *p = p_;
    pbuf_take(p, buf, p->tot_len);
}

void go_cs_pbuf_copy(void *p_, char *buf)
{
    struct pbuf *p = p_;
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    // pbuf_copy()
}

#define LWIP_PORT_INIT_IPADDR(addr) IP4_ADDR((addr), 192, 168, 100, 200)
#define LWIP_PORT_INIT_GW(addr) IP4_ADDR((addr), 192, 168, 100, 1)
#define LWIP_PORT_INIT_NETMASK(addr) IP4_ADDR((addr), 255, 255, 255, 0)

void *go_cs_netif_init(void *back_)
{
    struct cs_callback *back = back_;
    // init lwip
    lwip_init();
    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    LWIP_PORT_INIT_GW(&gw);
    LWIP_PORT_INIT_IPADDR(&addr);
    LWIP_PORT_INIT_NETMASK(&netmask);
    struct netif *netif = malloc(sizeof(struct netif));
    back->tcp_accept = go_cs_tcp_accept_h;
    back->tcp_recv = go_cs_tcp_recv_h;
    back->tcp_send_done = go_cs_tcp_send_done_h;
    back->tcp_close = go_cs_tcp_close_h;
    back->udp_recv = go_cs_udp_recv_h;
    // init netif
    if (!netif_add(netif, &addr, &netmask, &gw, back, cs_netif_init, ethernet_input))
    {
        free(netif);
        free(back);
        return NULL;
    }
    // set netif up
    netif_set_up(netif);
    // set netif link up, otherwise ip route will refuse to route
    netif_set_link_up(netif);
    // set netif default
    netif_set_default(netif);
    cs_tcp_raw_init(back);
    cs_udp_raw_init(back);
    return netif;
}

void go_cs_netif_deinit(void *netif_)
{
    struct netif *netif = netif_;
    struct cs_callback *back = netif->state;
    free(netif);
    free(back);
}

void go_cs_netif_proc(void *netif_)
{
    struct netif *netif = netif_;
    cs_netif_input(netif);
    netif_poll_all();
}
