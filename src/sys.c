
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#ifdef LWIP_UNIX_MACH
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"

u32_t lwip_port_rand(void)
{
    return (u32_t)rand();
}

static void
get_monotonic_time(struct timespec *ts)
{
#ifdef LWIP_UNIX_MACH
    /* darwin impl (no CLOCK_MONOTONIC) */
    u64_t t = mach_absolute_time();
    mach_timebase_info_data_t timebase_info = {0, 0};
    mach_timebase_info(&timebase_info);
    u64_t nano = (t * timebase_info.numer) / (timebase_info.denom);
    u64_t sec = nano / 1000000000L;
    nano -= sec * 1000000000L;
    ts->tv_sec = sec;
    ts->tv_nsec = nano;
#else
    clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
    return NULL;
}

void sys_lock_tcpip_core(void)
{
}

void sys_unlock_tcpip_core(void)
{
}

void sys_mark_tcpip_thread(void)
{
}

void sys_check_core_locking(void)
{
}

/*-----------------------------------------------------------------------------------*/
/* Mailbox */
err_t sys_mbox_new(struct sys_mbox **mb, int size)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
    return ERR_OK;
}

void sys_mbox_free(struct sys_mbox **mb)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
}

err_t sys_mbox_trypost(struct sys_mbox **mb, void *msg)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
    return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
    return sys_mbox_trypost(q, msg);
}

void sys_mbox_post(struct sys_mbox **mb, void *msg)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
}

u32_t sys_arch_mbox_tryfetch(struct sys_mbox **mb, void **msg)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
    return 0;
}

u32_t sys_arch_mbox_fetch(struct sys_mbox **mb, void **msg, u32_t timeout)
{
    LWIP_PLATFORM_ASSERT("panic:no mbox");
    return 0;
}

/*-----------------------------------------------------------------------------------*/
/* Mutex */
err_t sys_mutex_new(struct sys_mutex **mutex)
{
    *mutex = (struct sys_mutex *)0x01;
    return ERR_OK;
}

void sys_mutex_lock(struct sys_mutex **mutex)
{
}

void sys_mutex_unlock(struct sys_mutex **mutex)
{
}

void sys_mutex_free(struct sys_mutex **mutex)
{
    *mutex = 0x00;
}

/*-----------------------------------------------------------------------------------*/
/* Time */
u32_t sys_now(void)
{
    struct timespec ts;
    u32_t now;

    get_monotonic_time(&ts);
    now = (u32_t)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
#ifdef LWIP_FUZZ_SYS_NOW
    now += sys_now_offset;
#endif
    return now;
}

u32_t sys_jiffies(void)
{
    struct timespec ts;
    get_monotonic_time(&ts);
    return (u32_t)(ts.tv_sec * 1000000000L + ts.tv_nsec);
}

/*-----------------------------------------------------------------------------------*/
/* Init */

void sys_init(void)
{
}

/*-----------------------------------------------------------------------------------*/
/* Critical section */
sys_prot_t sys_arch_protect(void)
{
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
}
