/*
 * Copyright (C) 2016-2018 Bas Stottelaar <basstottelaar@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Power management peripheral test.
 *
 * @author      Bas Stottelaar <basstottelaar@gmail.com>
 * @author      Vincent Dupont <vincent@otakeys.com>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "periph/pm.h"
#ifdef MODULE_PERIPH_GPIO
#include "board.h"
#include "periph/gpio.h"
#endif
#ifdef MODULE_PM_LAYERED
#ifdef MODULE_PERIPH_RTC
#include "periph_conf.h"
#include "periph/rtc.h"
#endif
#include "pm_layered.h"
#endif
#include "xtimer.h"

extern int _pm_handler(int argc, char **argv);
int wakeup_gap =60;
struct tm alarm_time;
struct tm current_time;
int alarm_timestamp = 0;

#include "shell.h"

#ifndef BTN0_INT_FLANK
#define BTN0_INT_FLANK  GPIO_RISING
#endif
#define TM_YEAR_OFFSET      (1900)
void print_time(const char *label, const struct tm *time)
{
    printf("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
            time->tm_year + TM_YEAR_OFFSET,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec);
}

#ifdef MODULE_PM_LAYERED

#ifdef MODULE_PERIPH_RTC
static int check_mode_duration(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <power mode> <duration (s)>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int parse_mode(char *argv)
{
    uint8_t mode = atoi(argv);

    if (mode >= PM_NUM_MODES) {
        printf("Error: power mode not in range 0 - %d.\n", PM_NUM_MODES - 1);
        return -1;
    }

    return mode;
}

static int parse_duration(char *argv)
{
    int duration = atoi(argv);

    if (duration < 0) {
        puts("Error: duration must be a positive number.");
        return -1;
    }

    return duration;
}

static void cb_rtc(void *arg)
{
    int level = (int)arg;

    pm_block(level);
}

static void cb_rtc_puts(void *arg)
{
    
    // print_time("currenttime:\n", &current_time);
    puts(arg);
    
}
static void cb_rtc_puts1(void *arg)
{
    
    // print_time("currenttime:\n", &current_time);
    puts(arg);
    
}
static int cmd_unblock_rtc(int argc, char **argv)
{
    if (check_mode_duration(argc, argv) != 0) {
        return 1;
    }

    int mode = parse_mode(argv[1]);
    int duration = parse_duration(argv[2]);

    if (mode < 0 || duration < 0) {
        return 1;
    }

    pm_blocker_t pm_blocker = pm_get_blocker();
    if (pm_blocker.val_u8[mode] == 0) {
        printf("Mode %d is already unblocked.\n", mode);
        return 1;
    }

    printf("Unblocking power mode %d for %d seconds.\n", mode, duration);
    fflush(stdout);

    struct tm time;

    rtc_get_time(&time);
    time.tm_sec += duration;
    rtc_set_alarm(&time, cb_rtc, (void *)mode);

    pm_unblock(mode);

    return 0;
}

static int cmd_set_rtc(int argc, char **argv)
{
    if (check_mode_duration(argc, argv) != 0) {
        return 1;
    }

    int mode = parse_mode(argv[1]);
    int duration = parse_duration(argv[2]);

    if (mode < 0 || duration < 0) {
        return 1;
    }

    printf("Setting power mode %d for %d seconds.\n", mode, duration);
    fflush(stdout);

    struct tm time;

    rtc_get_time(&time);
    time.tm_sec += duration;
    rtc_set_alarm(&time, cb_rtc_puts, "The alarm rang");

    pm_set(mode);

    return 0;
}
#endif /* MODULE_PERIPH_RTC */
#endif /* MODULE_PM_LAYERED */

// #if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
// static void btn_cb(void *ctx)
// {
//     (void) ctx;
//     puts("BTN0 pressed.");
// }
// #endif /* MODULE_PERIPH_GPIO_IRQ */

/**
 * @brief   List of shell commands for this example.
 */
static const shell_command_t shell_commands[] = {
#if defined MODULE_PM_LAYERED && defined MODULE_PERIPH_RTC
    { "set_rtc", "temporary set power mode", cmd_set_rtc },
    { "unblock_rtc", "temporarily unblock power mode", cmd_unblock_rtc },
#endif
    { NULL, NULL, NULL }
};

/**
 * @brief   Application entry point.
 */
int main(void)
{
   
//     char line_buf[SHELL_DEFAULT_BUFSIZE];
   
//     /* print test application information */
// #ifdef MODULE_PM_LAYERED
//     printf("This application allows you to test the CPU power management.\n"
//            "The available power modes are 0 - %d. Lower-numbered power modes\n"
//            "save more power, but may require an event/interrupt to wake up\n"
//            "the CPU. Reset the CPU if needed.\n",
//            PM_NUM_MODES - 1);

//     /* In case the system boots into an unresponsive shell, at least display
//      * the state of PM blockers so that the user will know which power mode has
//      * been entered and is presumably responsible for the unresponsive shell.
//      */
//     _pm_handler(2, (char *[]){"pm", "show"});

// #else
//     puts("This application allows you to test the CPU power management.\n"
//          "Layered support is not unavailable for this CPU. Reset the CPU if\n"
//          "needed.");
// #endif

// #if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
//     puts("using BTN0 as wake-up source");
//     gpio_init_int(BTN0_PIN, BTN0_MODE, BTN0_INT_FLANK, btn_cb, NULL);
// #endif
 struct tm time = {
        .tm_year = 2020 - TM_YEAR_OFFSET,   /* years are counted from 1900 */
        .tm_mon  =  1,                      /* 0 = January, 11 = December */
        .tm_mday = 28,
        .tm_hour = 23,
        .tm_min  = 50,
        .tm_sec  = 00
    };
    rtc_init();
    print_time("  Setting clock to ", &time);
    rtc_set_time(&time);
    rtc_get_time(&time);
    xtimer_sleep(5);
    rtc_get_time(&time);
    print_time("Clock value is now ", &time);

    while (1) {
        puts("wake up!");
        
        rtc_get_time(&current_time);
        print_time("currenttime:\n", &current_time);
        print_time("currenttime11111:\n", &time);
        int current_timestamp= mktime(&time);
        printf("current time stamp11111: %d\n", current_timestamp);
        
        int unit = 70;
        if ((int)(current_timestamp % unit) < (wakeup_gap*1)){
            int chance = ( wakeup_gap ) - ( current_timestamp % unit );
            alarm_timestamp = (current_timestamp / unit) *unit+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp- 1577836800;
            rtc_get_time(&current_time);
            print_time("currenttime:\n", &current_time);
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time after wakeup:\n", &alarm_time);
            printf("xtimer sleep: for  %d seconds", chance);
            xtimer_sleep(chance);
            rtc_get_time(&time);
            print_time("currenttime:\n", &time);
            rtc_set_alarm(&alarm_time, cb_rtc_puts, "111");
            puts("radio is off and pm set 1");
            pm_set(1);
        }
        else{
            printf("fflush");
            fflush(stdout);
            alarm_timestamp =  current_timestamp+(unit- (current_timestamp % unit));
            alarm_timestamp = alarm_timestamp - 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            rtc_set_alarm(&alarm_time, cb_rtc_puts1, "222");
            rtc_get_time(&time);
            print_time("currenttime:\n", &time);
            puts("radio is off and pm set 1");
            pm_set(1);
        }
    }
    /* run the shell and wait for the user to enter a mode */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
