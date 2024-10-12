//
// Created by jaskulr on 10/12/24.
//
#include "MacroDefs.h"

int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    //Stop the sleep timer if a signal is recieved, and return the remaining time.
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}