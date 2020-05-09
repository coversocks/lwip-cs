#include "cs.h"
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

/* Define those to better describe your network interface. */
#define IFNAME0 't'
#define IFNAME1 'p'

/*-----------------------------------------------------------------------------------*/
/*
 * cs_low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *cs_low_level_input(struct netif *netif)
{
  struct pbuf *p;
  u16_t len;
  ssize_t readlen;
  char *buf;
  struct cs_callback *back = netif->state;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  buf = back->input(back->state, netif, &readlen);
  if (buf == NULL)
  {
    perror("cs read returned -1");
    return NULL;
  }
  len = (u16_t)readlen;

  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL)
  {
    pbuf_take(p, buf, len);
    /* acknowledge that packet has been read(); */
  }
  else
  {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("cs_netif_input: could not allocate pbuf\n"));
  }
  back->input_free(back->state, netif, buf);
  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * cs_netif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
void cs_netif_input(struct netif *netif)
{
  struct pbuf *p = cs_low_level_input(netif);
  if (p == NULL)
  {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(1, ("cs_netif_input: cs_low_level_input returned NULL\n"));
    return;
  }
  if (netif->input(p, netif) != ERR_OK)
  {
    LWIP_DEBUGF(NETIF_DEBUG, ("cs_netif_input: netif input error\n"));
    pbuf_free(p);
  }
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t cs_low_level_output(struct netif *netif, struct pbuf *p)
{
  ssize_t written;
  struct cs_callback *back = netif->state;
  char buf[1518]; /* max packet size including VLAN excluding CRC */

  if (p->tot_len > sizeof(buf))
  {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("cs_netif: packet too large");
    return ERR_IF;
  }

  /* initiate transfer(); */
  pbuf_copy_partial(p, buf, p->tot_len, 0);

  /* signal that packet should be sent(); */
  written = back->output(back->state, netif, buf, p->tot_len);
  if (written < p->tot_len)
  {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("tapif: write");
    return ERR_IF;
  }
  else
  {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}

/*-----------------------------------------------------------------------------------*/
static void cs_low_level_init(struct netif *netif)
{
#if LWIP_IPV4
  int ret;
  char buf[1024];
#endif /* LWIP_IPV4 */
  /* Obtain MAC address from network interface. */
  /* (We just fake an address...) */
  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x12;
  netif->hwaddr[2] = 0x34;
  netif->hwaddr[3] = 0x56;
  netif->hwaddr[4] = 0x78;
  netif->hwaddr[5] = 0xab;
  netif->hwaddr_len = 6;
  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
  netif_set_link_up(netif);
}
/*-----------------------------------------------------------------------------------*/
/*
 * cs_netif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function cs_low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t cs_netif_init(struct netif *netif)
{
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = cs_low_level_output;
  netif->mtu = 1500;

  cs_low_level_init(netif);

  return ERR_OK;
}

void cs_lwip_app_platform_assert(const char *msg, int line, const char *file)
{
  printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
  fflush(NULL);
  abort();
}
