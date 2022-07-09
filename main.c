/*
 * Copyright (c) 2022 Alexis Murzeau
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <linux/rtc.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#ifndef VERSION
#define VERSION "unknown-version"
#endif

#define RTC_DEV_PATH "/dev/rtc"
#define RTC_CHECK_PERIOD_SEC 60

int read_rtc(struct rtc_time* rtc_time) {
    int rtc_fd;
    
    rtc_fd = open(RTC_DEV_PATH, O_RDONLY);

    if(rtc_fd < 0) {
        int error = errno;
        fprintf(stderr, "Can't open " RTC_DEV_PATH ": %s(%d)\n", strerror(error), error);
        return error;
    }

    int result = ioctl(rtc_fd, RTC_RD_TIME, rtc_time);
    if(result < 0) {
        fprintf(stderr, "Can't read time from " RTC_DEV_PATH ", ioctl(RTC_RD_TIME) failed: %s(%d)\n", strerror(errno), errno);
        exit(1);
    }

    close(rtc_fd);

    return 0;
}

struct tm rtc_to_tm(const struct rtc_time* rtc_time) {
    struct tm tm_time = {0};

    tm_time.tm_sec = rtc_time->tm_sec;
    tm_time.tm_min = rtc_time->tm_min;
    tm_time.tm_hour = rtc_time->tm_hour;
    tm_time.tm_mday = rtc_time->tm_mday;
    tm_time.tm_mon = rtc_time->tm_mon;
    tm_time.tm_year = rtc_time->tm_year;
    tm_time.tm_wday = rtc_time->tm_wday;
    tm_time.tm_yday = rtc_time->tm_yday;
    tm_time.tm_isdst = rtc_time->tm_isdst;

    return tm_time;
}

struct rtc_time tm_to_rtc(const struct tm* tm_time) {
    struct rtc_time rtc_time = {0};

    rtc_time.tm_sec =   tm_time->tm_sec;
    rtc_time.tm_min =   tm_time->tm_min;
    rtc_time.tm_hour =  tm_time->tm_hour;
    rtc_time.tm_mday =  tm_time->tm_mday;
    rtc_time.tm_mon =   tm_time->tm_mon;
    rtc_time.tm_year =  tm_time->tm_year;
    rtc_time.tm_wday =  tm_time->tm_wday;
    rtc_time.tm_yday =  tm_time->tm_yday;
    rtc_time.tm_isdst = tm_time->tm_isdst;

    return rtc_time;
}

int check_rtc_jumped(const struct rtc_time* previous_rtc_time,
    const struct rtc_time* current_rtc_time,
    int64_t expected_seconds_elapsed) {
    
    struct tm previous_tm = rtc_to_tm(previous_rtc_time);
    struct tm current_tm = rtc_to_tm(current_rtc_time);

    time_t previous_time_t = timegm(&previous_tm);
    time_t current_time_t = timegm(&current_tm);
    int64_t diff = current_time_t - previous_time_t;

    if(diff < (expected_seconds_elapsed * 0.9 - 1) || diff > (expected_seconds_elapsed * 1.1 + 1)) {
        printf("RTC time jumped of %lld seconds instead of %lld, syncing system time with rtc\n",
            (long long int) diff,
            (long long int) expected_seconds_elapsed);
        
        fflush(stdout);
        return 1;
    }

    return 0;
}

void update_system_time() {
    struct rtc_time rtc_time;
    int rtc_fd;
	struct timespec ts;
    uint8_t dummy[1];
    
    rtc_fd = open(RTC_DEV_PATH, O_RDONLY);

    if(rtc_fd < 0) {
        int error = errno;
        fprintf(stderr, "Can't open " RTC_DEV_PATH ": %s(%d)\n", strerror(error), error);
        return;
    }

    // Sync to next second
    read(rtc_fd, dummy, 1);

    int result = ioctl(rtc_fd, RTC_RD_TIME, &rtc_time);
    if(result < 0) {
        fprintf(stderr, "Can't read time from " RTC_DEV_PATH ", ioctl(RTC_RD_TIME) failed: %s(%d)\n", strerror(errno), errno);
        exit(1);
    }

    struct tm current_tm = rtc_to_tm(&rtc_time);
    ts.tv_sec = timegm(&current_tm);
    ts.tv_nsec = 0;

    printf("Changing system time from %llu to %llu\n",
        (unsigned long long) time(NULL),
        (unsigned long long) ts.tv_sec);


    if(time(NULL) + 2 >= ts.tv_sec) {
        // Don't make the system time go backward, this can cause applications crashes/aborts
        // We use a margin of 2s to check it
        fprintf(stderr, "Not updating system time to avoid going backward in the past\n");
    } else {
        result = clock_settime(CLOCK_REALTIME, &ts);
        if(result < 0) {
            fprintf(stderr, "Can't set time, clock_settime failed: %s(%d)\n", strerror(errno), errno);
        }
    }

    close(rtc_fd);
    
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    struct rtc_time previous_rtc_time;
    struct rtc_time current_rtc_time;
    int previous_result;
    int result;

    printf("guest-vm-sync-with-rtc version %s\n", VERSION);
    printf("Synchronizing system time on RTC jump (check period: %d sec)\n", RTC_CHECK_PERIOD_SEC);
    fflush(stdout);

    result = read_rtc(&current_rtc_time);

    while(1) {
        // Wait one minute
        sleep(RTC_CHECK_PERIOD_SEC);

        previous_result = result;
        previous_rtc_time = current_rtc_time;

        result = read_rtc(&current_rtc_time);

        if(result != 0 || previous_result != 0) {
            fprintf(stderr, "Failed to read " RTC_DEV_PATH ", skipping rtc jump check\n");
            continue;
        }

        if(check_rtc_jumped(&previous_rtc_time, &current_rtc_time, RTC_CHECK_PERIOD_SEC)) {
            update_system_time();
        }
    }
}