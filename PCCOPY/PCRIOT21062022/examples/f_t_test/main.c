/*
 * Copyright (C) 2018 OTA keys S.A.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       File system usage example application
 *
 * @author      Vincent Dupont <vincent@otakeys.com>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "msg.h"


#include "shell.h"
#include "board.h" /* MTD_0 is defined in board.h */

/* Configure MTD device for SD card if none is provided */
#if !defined(MTD_0) && MODULE_MTD_SDCARD
#include "mtd_sdcard.h"
#include "sdcard_spi.h"
#include "sdcard_spi_params.h"

/*RTC*/
#include "periph_conf.h"
#include "periph/rtc.h"

/*NETIF AND PM*/
#include "xtimer.h"

#define TM_YEAR_OFFSET      (1900)

gnrc_netif_t* netif = NULL;
// int wakeup_gap =60;
static unsigned cnt = 0;
struct tm alarm_time;
struct tm alarm_time1;
struct tm current_time;

#define MAIN_QUEUE_SIZE (512)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define SDCARD_SPI_NUM ARRAY_SIZE(sdcard_spi_params)

/* SD card devices are provided by drivers/sdcard_spi/sdcard_spi.c */
extern sdcard_spi_t sdcard_spi_devs[SDCARD_SPI_NUM];

struct tm coaptime;

/* Configure MTD device for the first SD card */
static mtd_sdcard_t mtd_sdcard_dev = {
    .base = {
        .driver = &mtd_sdcard_driver
    },
    .sd_card = &sdcard_spi_devs[0],
    .params = &sdcard_spi_params[0],
};
static mtd_dev_t *mtd0 = (mtd_dev_t*)&mtd_sdcard_dev;
#define MTD_0 mtd0
#endif

/* Flash mount point */
#define FLASH_MOUNT_POINT   "/dev"

/* In this example, MTD_0 is used as mtd interface for littlefs or spiffs */
/* littlefs and spiffs basic usage are shown */
#ifdef MTD_0
/* File system descriptor initialization */
#if defined(MODULE_LITTLEFS)
/* include file system header for driver */
#include "fs/littlefs_fs.h"

/* file system specific descriptor
 * for littlefs, some fields can be tweaked to define the size
 * of the partition, see header documentation.
 * In this example, default behavior will be used, i.e. the entire
 * memory will be used (parameters come from mtd) */
static littlefs_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* littlefs file system driver will be used */
#define FS_DRIVER littlefs_file_system

#elif defined(MODULE_LITTLEFS2)
/* include file system header for driver */
#include "fs/littlefs2_fs.h"

/* file system specific descriptor
 * for littlefs2, some fields can be tweaked to define the size
 * of the partition, see header documentation.
 * In this example, default behavior will be used, i.e. the entire
 * memory will be used (parameters come from mtd) */
static littlefs2_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* littlefs file system driver will be used */
#define FS_DRIVER littlefs2_file_system

#elif defined(MODULE_SPIFFS)
/* include file system header */
#include "fs/spiffs_fs.h"

/* file system specific descriptor
 * as for littlefs, some fields can be changed if needed,
 * this example focus on basic usage, i.e. entire memory used */
static spiffs_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* spiffs driver will be used */
#define FS_DRIVER spiffs_file_system

#elif defined(MODULE_FATFS_VFS)
/* include file system header */
#include "fs/fatfs.h"

/* file system specific descriptor
 * as for littlefs, some fields can be changed if needed,
 * this example focus on basic usage, i.e. entire memory used */
static fatfs_desc_t fs_desc;

/* provide mtd devices for use within diskio layer of fatfs */
mtd_dev_t *fatfs_mtd_devs[FF_VOLUMES];

/* fatfs driver will be used */
#define FS_DRIVER fatfs_file_system
#endif

/* this structure defines the vfs mount point:
 *  - fs field is set to the file system driver
 *  - mount_point field is the mount point name
 *  - private_data depends on the underlying file system. For both spiffs and
 *  littlefs, it needs to be a pointer to the file system descriptor */
static vfs_mount_t flash_mount = {
    .fs = &FS_DRIVER,
    .mount_point = FLASH_MOUNT_POINT,
    .private_data = &fs_desc,
};
#endif /* MTD_0 */

/* Add simple macro to check if an MTD device together with a filesystem is
 * compiled in */
#if  defined(MTD_0) && \
     (defined(MODULE_SPIFFS) || \
      defined(MODULE_LITTLEFS) || \
      defined(MODULE_LITTLEFS2) || \
      defined(MODULE_FATFS_VFS))
#define FLASH_AND_FILESYSTEM_PRESENT    1
#else
#define FLASH_AND_FILESYSTEM_PRESENT    0
#endif
/*******************************************************************************************************************/
/* constfs example */
#include "fs/constfs.h"

#define HELLO_WORLD_CONTENT "Hello World!\n"
#define HELLO_RIOT_CONTENT  "Hello RIOT!\n"
#define TEMP_CONTENT  "TEMPXXX TIMESTAMPXXX\n"
/* this defines two const files in the constfs */
static constfs_file_t constfs_files[] = {
    {
        .path = "/hello-world",
        .size = sizeof(HELLO_WORLD_CONTENT),
        .data = (const uint8_t *)HELLO_WORLD_CONTENT,
    },
    {
        .path = "/hello-riot",
        .size = sizeof(HELLO_RIOT_CONTENT),
        .data = (const uint8_t *)HELLO_RIOT_CONTENT,
    },
    {
        .path = "/temp",
        .size = sizeof(TEMP_CONTENT),
        .data = (const uint8_t *)TEMP_CONTENT,
    }
};

/* this is the constfs specific descriptor */
static constfs_t constfs_desc = {
    .nfiles = ARRAY_SIZE(constfs_files),
    .files = constfs_files,
};

/* constfs mount point, as for previous example, it needs a file system driver,
 * a mount point and private_data as a pointer to the constfs descriptor */
static vfs_mount_t const_mount = {
    .fs = &constfs_file_system,
    .mount_point = "/const",
    .private_data = &constfs_desc,
};

/* Command handlers */
static int _mount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_mount(&flash_mount);
    if (res < 0) {
        printf("Error while mounting %s...try format\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully mounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _format(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_format(&flash_mount);
    if (res < 0) {
        printf("Error while formatting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully formatted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _umount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_umount(&flash_mount);
    if (res < 0) {
        printf("Error while unmounting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully unmounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _cat(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    /* With newlib or picolibc, low-level syscalls are plugged to RIOT vfs
     * on native, open/read/write/close/... are plugged to RIOT vfs */
#if defined(MODULE_NEWLIB) || defined(MODULE_PICOLIBC)
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (fread(&c, 1, 1, f) != 0) {
        putchar(c);
    }
    fclose(f);
#else
    int fd = open(argv[1], O_RDWR | O_CREAT);//O_RDONLY);
    if (fd < 0) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (read(fd, &c, 1) != 0) {
        putchar(c);
    }
    close(fd);
#endif
    fflush(stdout);
    return 0;
}

static int _tee(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <file> <str>\n", argv[0]);
        return 1;
    }

#if defined(MODULE_NEWLIB) || defined(MODULE_PICOLIBC)
    FILE *f = fopen(argv[1], "w+");
    if (f == NULL) {
        printf("error while trying to create1 %s\n", argv[1]);
        return 1;
    }
    if (fwrite(argv[2], 1, strlen(argv[2]), f) != strlen(argv[2])) {
        puts("Error while writing");
    }
    fclose(f);
#else
    int fd = open(argv[1], O_RDWR | O_CREAT);
    if (fd < 0) {
        printf("error while trying to create2 %s\n", argv[1]);
        return 1;
    }
    if (write(fd, argv[2], strlen(argv[2])) != (ssize_t)strlen(argv[2])) {
        puts("Error while writing");
    }
    close(fd);
#endif
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "mount", "mount flash filesystem", _mount },
    { "format", "format flash file system", _format },
    { "umount", "unmount flash filesystem", _umount },
    { "cat", "print the content of a file", _cat },
    { "tee", "write a string in a file", _tee },
    { NULL, NULL, NULL }
};

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
static void cb_rtc(void *arg)
{
    puts(arg);
}
int main(void)
{

msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
        struct tm time = {
        .tm_year = 2020 - TM_YEAR_OFFSET,   /* years are counted from 1900 */
        .tm_mon  =  4,                      /* 0 = January, 11 = December */
        .tm_mday = 28,
        .tm_hour = 23,
        .tm_min  = 58,
        .tm_sec  = 57
    };   
rtc_init();
    puts("1111111111111111111111111111111111");
    print_time("  Setting clock to ", &time);
    rtc_set_time(&time);
    rtc_get_time(&time);	
    print_time("Clock value is now ", &time);
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS) || defined(MODULE_LITTLEFS2))
    /* spiffs and littlefs need a mtd pointer
     * by default the whole memory is used */
    fs_desc.dev = MTD_0;
#elif defined(MTD_0) && defined(MODULE_FATFS_VFS)
    fatfs_mtd_devs[fs_desc.vol_idx] = MTD_0;
#endif
    int res = vfs_mount(&const_mount);
    if (res < 0) {
        puts("Error while mounting constfs");
    }
    else {
        puts("constfs mounted successfully");
    }
while (1) {
    puts("wake up!");
        ++cnt;
        rtc_get_time(&current_time);
        print_time("currenttime:\n", &current_time);
        int current_timestamp= mktime(&current_time);
        printf("current time stamp: %d\n", current_timestamp);
        int alarm_timestamp = 0;
        if ((int)(current_timestamp % 20) < (10*1)){
            // radio_on(netif);
            int chance = ( 10 ) - ( current_timestamp % 20 );
            alarm_timestamp = (current_timestamp / 20) *20 + (10 * 1);
            alarm_timestamp = alarm_timestamp- 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            printf("---------%ds",chance);
            puts("xtimer sleep");
            
            xtimer_sleep(chance);
           
            rtc_set_alarm(&alarm_time, cb_rtc, "111");
            // radio_off(netif);

            // pm_set(1);
            /*源代码*/
            // rtc_get_alarm(&time);
            // inc_secs(&time, PERIOD);
            // rtc_set_alarm(&time, cb, &rtc_mtx);
        }
        else{
            printf("fflush");
            fflush(stdout);
            // radio_off(netif);
            alarm_timestamp =  current_timestamp+(20- (current_timestamp % 20));
            // alarm_timestamp = (current_timestamp / 360) *360+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp - 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time1);
            print_time("alarm time:\n", &alarm_time1);
            
            // int modetest =1;
            // rtc_set_alarm(&alarm_time1, cb_rtc, (void *)modetest);
            rtc_set_alarm(&alarm_time1, cb_rtc, "111");
            // radio_on(netif);
            // pm_set(1);
            puts("                                                                 ");

            // puts("radio is off");
            // puts("radio is offfffffffffffffffffffffffff");
            // puts("zzzzzzzzzzzzzzzzzzzz");
            // puts("3333333333333");
        }
    }


    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
