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
#include <netif/ethernet.h>
#include "cs.h"
#include "tcp.h"
#include "udp.h"

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
  u16_t readlen = 0;
  cs_callback *back = netif->state;
  p = back->input(back->state, netif, &readlen);
  MIB2_STATS_NETIF_ADD(netif, ifinoctets, readlen);
  if (p == NULL)
  {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("cs_netif_input: could not allocate pbuf\n"));
  }
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
  cs_callback *back = netif->state;
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
    perror("cs_low_level_output: write");
    return ERR_IF;
  }
  else
  {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}

err_t cs_tap_init(struct netif *netif);

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

int cs_ip_len(ip_addr_t *ipaddr)
{
  if (IP_GET_TYPE(ipaddr) == IPADDR_TYPE_V6)
  {
    return 16;
  }
  else
  {
    return 4;
  }
}

void cs_ip_get(const ip_addr_t *ipaddr, char *buf)
{
  if (IP_GET_TYPE(ipaddr) == IPADDR_TYPE_V6)
  {
    buf[0] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[0]) >> 24;
    buf[1] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[0]) >> 16;
    buf[2] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[0]) >> 8;
    buf[3] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[0]);
    buf[4] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[1]) >> 24;
    buf[5] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[1]) >> 16;
    buf[6] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[1]) >> 8;
    buf[7] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[1]);
    buf[8] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[2]) >> 24;
    buf[9] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[2]) >> 16;
    buf[10] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[2]) >> 8;
    buf[11] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[2]);
    buf[12] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[3]) >> 24;
    buf[13] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[3]) >> 16;
    buf[14] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[3]) >> 8;
    buf[15] = (char)lwip_htonl(ip_2_ip6(ipaddr)->addr[3]);
  }
  else
  {
    buf[0] = ip4_addr1_val(*ip_2_ip4(ipaddr));
    buf[1] = ip4_addr2_val(*ip_2_ip4(ipaddr));
    buf[2] = ip4_addr3_val(*ip_2_ip4(ipaddr));
    buf[3] = ip4_addr4_val(*ip_2_ip4(ipaddr));
  }
}

#define LWIP_PORT_INIT_IPADDR(addr) IP4_ADDR((addr), 192, 168, 100, 200)
#define LWIP_PORT_INIT_GW(addr) IP4_ADDR((addr), 192, 168, 100, 1)
#define LWIP_PORT_INIT_NETMASK(addr) IP4_ADDR((addr), 255, 255, 255, 0)

int cs_init(cs_callback *back)
{
    // init lwip
    lwip_init();
    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    LWIP_PORT_INIT_GW(&gw);
    LWIP_PORT_INIT_IPADDR(&addr);
    LWIP_PORT_INIT_NETMASK(&netmask);
    struct netif *netif = back->netif;
    // init netif
    if (!netif_add(netif, &addr, &netmask, &gw, back, cs_netif_init, netif_input))
    {
        return -1;
    }
    back->init(back->state, netif);
    // set netif up
    netif_set_up(netif);
    // set netif link up, otherwise ip route will refuse to route
    netif_set_link_up(netif);
    // set netif default
    netif_set_default(netif);
    cs_tcp_raw_init(back);
    cs_udp_raw_init(back);
    return 0;
}

void cs_netif_proc(struct netif *netif)
{
    cs_netif_input(netif);
    netif_poll_all();
}
