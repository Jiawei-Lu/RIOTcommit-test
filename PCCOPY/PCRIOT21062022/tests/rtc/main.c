/*
 * Copyright (C) 2015 Lari Lehtomäki
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test for low-level Real Time clock drivers
 *
 * This test will initialize the real-time timer and trigger an alarm printing
 * 'Hello' every 10 seconds
 *
 * @author      Lari Lehtomäki <lari@lehtomaki.fi>
 *
 * @}
 */

#include <stdio.h>
#include <time.h>

#include "mutex.h"
#include "periph_conf.h"
#include "periph/rtc.h"
#include "xtimer.h"

#include "periph/pm.h"
#ifdef MODULE_PM_LAYERED
#include "pm_layered.h"
#endif

#define PERIOD              (2U)
#define REPEAT              (4U)

#define TM_YEAR_OFFSET      (1900)


int wakeup_gap =60;
static unsigned cnt = 0;
struct tm alarm_time;
struct tm current_time;
static void print_time(const char *label, const struct tm *time)
{
    printf("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
            time->tm_year + TM_YEAR_OFFSET,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec);
}

static void inc_secs(struct tm *time, unsigned val)
{
    time->tm_sec += val;
}

static void cb_rtc(void *arg)
{
    puts(arg);
}
static void cb(void *arg)
{
    mutex_unlock(arg);
}

int main(void)
{
    struct tm time = {
        .tm_year = 2020 - TM_YEAR_OFFSET,   /* years are counted from 1900 */
        .tm_mon  =  1,                      /* 0 = January, 11 = December */
        .tm_mday = 28,
        .tm_hour = 23,
        .tm_min  = 59,
        .tm_sec  = 57
    };

    mutex_t rtc_mtx = MUTEX_INIT_LOCKED;

    puts("\nRIOT RTC low-level driver test");
    printf("This test will display 'Alarm!' every %u seconds for %u times\n",
            PERIOD, REPEAT);

    rtc_init();

    /* set RTC */
    print_time("  Setting clock to ", &time);
    rtc_set_time(&time);

    /* read RTC to confirm value */
    rtc_get_time(&time);
    print_time("Clock value is now ", &time);

    /* set initial alarm */
    inc_secs(&time, PERIOD);
    print_time("  Setting alarm to ", &time);
    rtc_set_alarm(&time, cb, &rtc_mtx);

    /* verify alarm */
    rtc_get_alarm(&time);
    print_time("   Alarm is set to ", &time);

    /* clear alarm */
    rtc_clear_alarm();
    rtc_get_time(&time);
    print_time("  Alarm cleared at ", &time);

    /* verify alarm has been cleared */
    xtimer_sleep(PERIOD);
    rtc_get_time(&time);
    if (mutex_trylock(&rtc_mtx)) {
        print_time("   Error: Alarm at ", &time);
    }
    else {
        print_time("       No alarm at ", &time);
    }

    /* set alarm */
    rtc_get_time(&time);
    inc_secs(&time, PERIOD);
    rtc_set_alarm(&time, cb, &rtc_mtx);
    print_time("  Setting alarm to ", &time);
    puts("");

    /* loop over a few alarm cycles */
    while (1) {
        // mutex_lock(&rtc_mtx);
        // puts("Alarm!");
        puts("wake up!");
        ++cnt;
        rtc_get_time(&current_time);
        print_time("currenttime:\n", &current_time);
        int current_timestamp= mktime(&current_time);
        printf("current time stamp: %d\n", current_timestamp);
        int alarm_timestamp = 0;
        if ((int)(current_timestamp % 120) < (wakeup_gap*1)){
            int chance = ( wakeup_gap ) - ( current_timestamp % 120 );
            alarm_timestamp = (current_timestamp / 120) *120+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp- 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            printf("---------%ds",chance);
            puts("xtimer sleep");
            
            xtimer_sleep(chance);
           
            rtc_set_alarm(&alarm_time, cb_rtc, "111");
            
            puts("radio is off");
            puts("radio is offfffffffffffffffffffffffff");
            puts("zzzzzzzzzzzzzzzzzzzz");
            pm_set(1);
            /*源代码*/
            // rtc_get_alarm(&time);
            // inc_secs(&time, PERIOD);
            // rtc_set_alarm(&time, cb, &rtc_mtx);
        }
        else{
            printf("fflush");
            fflush(stdout);
            alarm_timestamp =  current_timestamp+(120- (current_timestamp % 120));
            // alarm_timestamp = (current_timestamp / 360) *360+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp - 1577836800+10;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            
            int modetest =1;
            rtc_set_alarm(&alarm_time, cb_rtc, (void *)modetest);
            pm_set(1);
            puts("radio is off");
            puts("radio is offfffffffffffffffffffffffff");
            puts("zzzzzzzzzzzzzzzzzzzz");
            puts("3333333333333");
        }
    }

    return 0;
}
