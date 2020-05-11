/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Christiaan Simons rewrote this file to get a more stable echo example.
 */

/**
 * @file
 * TCP echo server example using raw API.
 *
 * Echos all bytes sent by connecting client,
 * and passively closes when client is done.
 *
 */

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "tcp.h"
#include "cs.h"

#if LWIP_TCP && LWIP_CALLBACK_API

static void cs_tcp_raw_free(struct cs_tcp_raw_state *es)
{
  if (es != NULL)
  {
    if (es->recv)
    {
      /* free the buffer chain if present */
      pbuf_free(es->recv);
    }
    if (es->send)
    {
      /* free the buffer chain if present */
      pbuf_free(es->send);
    }

    mem_free(es);
  }
}

static void cs_tcp_raw_close(struct tcp_pcb *tpcb, struct cs_tcp_raw_state *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  es->callback->tcp_close(es->callback->state, tpcb, es);
  cs_tcp_raw_free(es);
  tcp_close(tpcb);
}

void cs_tcp_raw_send(struct tcp_pcb *tpcb, struct cs_tcp_raw_state *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  while ((wr_err == ERR_OK) &&
         (es->send != NULL) &&
         (es->send->len <= tcp_sndbuf(tpcb)))
  {
    ptr = es->send;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
    if (wr_err == ERR_OK)
    {
      u16_t plen;

      plen = ptr->len;
      /* continue with next pbuf in chain (if any) */
      es->send = ptr->next;
      if (es->send != NULL)
      {
        /* new reference! */
        pbuf_ref(es->send);
      }
      /* chop first pbuf from chain */
      pbuf_free(ptr);
      // /* we can read more data now */
      tcp_recved(tpcb, plen);
    }
    else if (wr_err == ERR_MEM)
    {
      /* we are low on memory, try later / harder, defer to poll */
      es->send = ptr;
    }
    else
    {
      // /* close */
      // tcp_close(tpcb);
      // pbuf_free(ptr);
      // es->send = NULL;
    }
  }
  if (es->send == NULL)
  {
    es->callback->tcp_send_done(es->callback->state, tpcb, es);
  }
}

static void cs_tcp_raw_error(void *arg, err_t err)
{
  struct cs_tcp_raw_state *es;
  LWIP_ASSERT("arg != NULL", arg != NULL);

  LWIP_UNUSED_ARG(err);

  es = (struct cs_tcp_raw_state *)arg;

  cs_tcp_raw_free(es);
}

static err_t cs_tcp_raw_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct cs_tcp_raw_state *es;
  LWIP_ASSERT("arg != NULL", arg != NULL);

  es = (struct cs_tcp_raw_state *)arg;
  if (es != NULL)
  {
    if (es->send != NULL)
    {
      /* there is a remaining pbuf (chain)  */
      cs_tcp_raw_send(tpcb, es);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if (es->state == ES_CLOSING)
      {
        cs_tcp_raw_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

static err_t cs_tcp_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct cs_tcp_raw_state *es;
  LWIP_ASSERT("arg != NULL", arg != NULL);
  LWIP_UNUSED_ARG(len);

  es = (struct cs_tcp_raw_state *)arg;
  es->retries = 0;

  if (es->send != NULL)
  {
    /* still got pbufs to send */
    tcp_sent(tpcb, cs_tcp_raw_sent);
    cs_tcp_raw_send(tpcb, es);
  }
  else
  {
    /* no more pbufs to send */
    if (es->state == ES_CLOSING)
    {
      cs_tcp_raw_close(tpcb, es);
    }
  }
  return ERR_OK;
}

static err_t cs_tcp_raw_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct cs_tcp_raw_state *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL", arg != NULL);
  es = (struct cs_tcp_raw_state *)arg;
  if (p == NULL)
  {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if (es->send == NULL)
    {
      /* we're done sending, close it */
      cs_tcp_raw_close(tpcb, es);
    }
    else
    {
      /* we're not done yet */
      cs_tcp_raw_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }
  else if (err != ERR_OK)
  {
    /* cleanup, for unknown reason */
    LWIP_ASSERT("no pbuf expected here", p == NULL);
    ret_err = err;
  }
  else if (es->state == ES_ACCEPTED)
  {
    /* first data chunk in p->payload */
    es->state = ES_RECEIVED;
    /* store reference to incoming pbuf (chain) */
    /*
    es->send = p;
    cs_tcp_raw_send(tpcb, es);
    */
    ret_err = es->callback->tcp_recv(es->callback->state, tpcb, p, es);
  }
  else if (es->state == ES_RECEIVED)
  {
    /* read some more data */
    // if (es->send == NULL)
    // {
    /*
      es->send = p;
      cs_tcp_raw_send(tpcb, es);
      */
    ret_err = es->callback->tcp_recv(es->callback->state, tpcb, p, es);
    // }
    // else
    // {
    //   struct pbuf *ptr;
    //   /* chain pbufs to the end of what we recv'ed previously  */
    //   ptr = es->send;
    //   pbuf_cat(ptr, p);
    // }
    // ret_err = ERR_OK;
  }
  else
  {
    /* unknown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static err_t cs_tcp_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct cs_tcp_raw_state *es;
  struct cs_callback *back = arg;
  if ((err != ERR_OK) || (newpcb == NULL) || (back == NULL))
  {
    return ERR_VAL;
  }
  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  es = (struct cs_tcp_raw_state *)mem_malloc(sizeof(struct cs_tcp_raw_state));
  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->recv = NULL;
    es->send = NULL;
    es->callback = back;
    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, cs_tcp_raw_recv);
    tcp_err(newpcb, cs_tcp_raw_error);
    tcp_poll(newpcb, cs_tcp_raw_poll, 0);
    tcp_sent(newpcb, cs_tcp_raw_sent);
    ret_err = back->tcp_accept(back->state, newpcb, es);
    if (ret_err != ERR_OK)
    {
      cs_tcp_raw_free(es);
    }
  }
  else
  {
    ret_err = ERR_MEM;
  }
  return ret_err;
}

struct tcp_pcb *cs_tcp_raw_init(struct cs_callback *callback)
{
  err_t err;
  struct tcp_pcb *npcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (npcb != NULL)
  {
    tcp_arg(npcb, callback);
    err = tcp_bind(npcb, IP_ANY_TYPE, 7);
    if (err == ERR_OK)
    {
      npcb = tcp_listen(npcb);
      tcp_accept(npcb, cs_tcp_raw_accept);
    }
    else
    {
      tcp_free(npcb);
      npcb = NULL;
    }
  }
  return npcb;
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
