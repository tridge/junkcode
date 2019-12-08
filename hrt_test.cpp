#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CH_CFG_ST_RESOLUTION 16
#define CH_CFG_ST_FREQUENCY 1000000U

#if CH_CFG_ST_RESOLUTION == 16
typedef uint16_t systime_t;
#else
typedef uint32_t systime_t;
#endif

#define TIME_MAX_SYSTIME ((systime_t)-1)

#define MAX_TIME_STEP_US 2000U

static systime_t global_us;

static systime_t chVTGetSystemTimeX()
{
    return global_us / (1000000UL / CH_CFG_ST_FREQUENCY);
}

typedef uint32_t syssts_t;

static syssts_t chSysGetStatusAndLockX()
{
    return 0;
}
static void chSysRestoreStatusX(syssts_t x)
{
}

/*
  we have 4 possible configurations of boards, made up of boards that
  have the following properties:

    CH_CFG_ST_RESOLUTION = 16 or 32
    CH_CFG_ST_FREQUENCY  = 1000 or 1000000

  To keep as much code in common as possible we create a function
  system_time_u32_us() for all boards which gives system time since
  boot in microseconds, and which wraps at 0xFFFFFFFF.

  On top of this base function we build get_systime_us32() which has
  the same property, but which also maintains two globals:

    - timer_base_us64: 64 bit offset in microseconds to support micros64()
    - timer_base_ms: 32 bit offset in milliseconds to support millis32()

  The aim of all of this code is to avoid 64 bit operations on the
  fast paths for 32 bit return functions
*/

#if CH_CFG_ST_FREQUENCY != 1000000U && CH_CFG_ST_FREQUENCY != 1000U
#error "unsupported tick frequency"
#endif

#if CH_CFG_ST_RESOLUTION == 16
static uint32_t system_time_u32_us(void)
{
    systime_t now = chVTGetSystemTimeX();
#if CH_CFG_ST_FREQUENCY == 1000U
    now *= 1000U;
#endif
    static systime_t last_systime;
    static uint32_t timer_base_us32;
    uint16_t dt = now - last_systime;
    last_systime = now;
    timer_base_us32 += dt;
    return timer_base_us32;
}
#elif CH_CFG_ST_RESOLUTION == 32
static uint32_t system_time_u32_us(void)
{
    systime_t now = chVTGetSystemTimeX();
#if CH_CFG_ST_FREQUENCY == 1000U
    now *= 1000U;
#endif
    return now;
}
#else
#error "unsupported timer resolution"
#endif

static uint64_t timer_base_us64;
static uint32_t timer_base_ms;

static uint32_t get_systime_us32(void)
{
    static uint32_t last_us32;
    uint32_t now = system_time_u32_us();
    if (now < last_us32) {
        const uint64_t dt_us = 0x100000000ULL;
        timer_base_us64 += dt_us;
        timer_base_ms += dt_us/1000ULL;
    }
    last_us32 = now;
    return now;
}

/*
  for the exposed functions we use chSysGetStatusAndLockX() to prevent
  an interrupt changing the globals while allowing this call from any
  context
*/

uint64_t hrt_micros64()
{
    syssts_t sts = chSysGetStatusAndLockX();
    uint32_t now = get_systime_us32();
    uint64_t ret = timer_base_us64 + now;
    chSysRestoreStatusX(sts);
    return ret;
}

uint32_t hrt_micros32()
{
    syssts_t sts = chSysGetStatusAndLockX();
    uint32_t ret = get_systime_us32();
    chSysRestoreStatusX(sts);
    return ret;
}

uint32_t hrt_millis32()
{
    syssts_t sts = chSysGetStatusAndLockX();
    uint32_t now = get_systime_us32();
    uint32_t ret = (now / 1000U) + timer_base_ms;
    chSysRestoreStatusX(sts);
    return ret;
}

/*
  get random time step
 */
static uint16_t get_random_step(void)
{
    static uint32_t m_z = 1234;
    static uint32_t m_w = 76542;
    m_z = 36969 * (m_z & 0xFFFFu) + (m_z >> 16);
    m_w = 18000 * (m_w & 0xFFFFu) + (m_w >> 16);
    return ((m_z << 16) + m_w) % MAX_TIME_STEP_US;
}

int main(void)
{
    uint32_t last_ms = hrt_millis32();
    uint32_t last_us32 = hrt_micros32();
    uint64_t last_us64 = hrt_micros64();
    uint32_t err_count=0;
    uint32_t wrap_count_ms32 = 0;
    // test for up to 10 wraps of ms32, which is 490 days
    while (wrap_count_ms32 < 10) {
        global_us += get_random_step() / (1000000UL / CH_CFG_ST_FREQUENCY);
        uint32_t now_ms = hrt_millis32();
        uint32_t now_us32 = hrt_micros32();
        uint64_t now_us64 = hrt_micros64();
        uint32_t dt_us32 = now_us32 - last_us32;
        uint64_t dt_us64 = now_us64 - last_us64;
        uint32_t dt_ms = now_ms - last_ms;
        if (dt_us32 > MAX_TIME_STEP_US) {
            printf("step us32 0x%08x 0x%08x %u\n", now_us32, last_us32, dt_us32);
            err_count++;
        }
        if (dt_us64 > MAX_TIME_STEP_US) {
            printf("step us64 0x%llx 0x%llx %llu\n", (unsigned long long)now_us64, (unsigned long long)last_us64, (unsigned long long)dt_us64);
            err_count++;
        }
        if (dt_ms > (MAX_TIME_STEP_US/1000U)) {
            printf("step ms 0x%08x 0x%08x %u now_us32=0x%08x\n", now_ms, last_ms, dt_ms, now_us32);
            err_count++;
        }
        if (now_ms < last_ms) {
            wrap_count_ms32++;
            printf("wrap ms32 %u\n", wrap_count_ms32);
        }
        last_ms = now_ms;
        last_us32 = now_us32;
        last_us64 = now_us64;
    }
    if (err_count != 0) {
        printf("FAILED\n");
        return -1;
    }
    printf("PASSED\n");
    return 0;
}
