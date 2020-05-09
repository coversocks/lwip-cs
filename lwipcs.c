#include "lwipcs.h"
#include "tcp.h"
#include "udp.h"

extern int tcpAccept(void *arg, void *newpcb, void *state, int addr_len, char *local, int local_port, char *remote, int remote_port);
extern int tcpRecv(void *arg, void *tpcb, void *state, int addr_len, char *local, int local_port, char *remote, int remote_port, char *buf, int buflen);
extern int tcpClose(void *arg, void *tpcb, void *state, int addr_len, char *local, int local_port, char *remote, int remote_port);
extern int udpRecv(void *arg, void *upcb, int addr_len, char *local, int local_port, char *remote, int remote_port, char *buf, int buflen);

err_t cs_tcp_accept(void *arg, struct tcp_pcb *newpcb, struct cs_tcp_raw_state *state)
{
    int addr_len;
    char *local = cs_ip_val(newpcb->local_ip, &addr_len);
    char *remote = cs_ip_val(newpcb->remote_ip, &addr_len);
    err_t err = tcpAccept(arg, newpcb, state, addr_len, local, newpcb->local_port, remote, newpcb->remote_port);
    cs_ip_val_free(local);
    cs_ip_val_free(remote);
    return err;
}

err_t cs_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, struct cs_tcp_raw_state *state)
{
    char buf[1518];
    if (p->tot_len > sizeof(buf))
    {
        return ERR_IF;
    }
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    int addr_len;
    char *local = cs_ip_val(tpcb->local_ip, &addr_len);
    char *remote = cs_ip_val(tpcb->remote_ip, &addr_len);
    err_t err = tcpRecv(arg, tpcb, state, addr_len, local, tpcb->local_port, remote, tpcb->remote_port, buf, p->tot_len);
    cs_ip_val_free(local);
    cs_ip_val_free(remote);
    pbuf_free(p);
    return ERR_OK;
}

void cs_tcp_close(void *arg, struct tcp_pcb *tpcb, struct cs_tcp_raw_state *state)
{
    int addr_len;
    char *local = cs_ip_val(tpcb->local_ip, &addr_len);
    char *remote = cs_ip_val(tpcb->remote_ip, &addr_len);
    err_t err = tcpClose(arg, tpcb, state, addr_len, local, tpcb->local_port, remote, tpcb->remote_port);
    cs_ip_val_free(local);
    cs_ip_val_free(remote);
}

int cs_tcp_send(void *tpcb_, void *state_, char *buf, int buf_len)
{
    struct tcp_pcb *tpcb = tpcb_;
    struct cs_tcp_raw_state *state = state_;
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

err_t cs_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    char buf[1518];
    if (p->tot_len > sizeof(buf))
    {
        return ERR_IF;
    }
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    int addr_len;
    char *local = cs_ip_val(upcb->local_ip, &addr_len);
    char *remote = cs_ip_val(*addr, &addr_len);
    err_t err = udpRecv(arg, upcb, addr_len, local, upcb->local_port, remote, port, buf, p->tot_len);
    cs_ip_val_free(local);
    cs_ip_val_free(remote);
    pbuf_free(p);
    return ERR_OK;
}

int cs_udp_send(void *upcb_, void *addr_, u16_t port, char *buf, int buf_len)
{
    struct udp_pcb *upcb = upcb_;
    ip_addr_t *addr = addr_;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, buf_len, PBUF_POOL);
    if (p == NULL)
    {
        return 1;
    }
    pbuf_take(p, buf, buf_len);
    cs_udp_sendto(upcb, p, addr, port);
    pbuf_free(p);
    return 0;
}
