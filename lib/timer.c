/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

#include <event.h>
#include <sys/time.h>
#include <glib.h>

#include "utils.h"
#include "timer.h"

#ifdef WIN32
#include "net.h"
#endif

struct CcnetTimer
{
    struct event   event;
    struct timeval tv;
    TimerCB        func;
    void          *user_data;
    uint8_t        inCallback;
};

static void
timer_callback (int fd, short event, void *vtimer)
{
    int more;
    struct CcnetTimer *timer = vtimer;

    timer->inCallback = 1;
    more = (*timer->func) (timer->user_data);
    timer->inCallback = 0;

    if (more)
        evtimer_add (&timer->event, &timer->tv);
    else
        ccnet_timer_free (&timer);
}

void
ccnet_timer_free (CcnetTimer **ptimer)
{
    CcnetTimer *timer;

    /* zero out the argument passed in */
    g_return_if_fail (ptimer);

    timer = *ptimer;
    *ptimer = NULL;

    /* destroy the timer directly or via the command queue */
    if (timer && !timer->inCallback)
    {
        event_del (&timer->event);
        g_free (timer);
    }
}


CcnetTimer*
ccnet_timer_new (TimerCB         func,
                 void           *user_data,
                 uint64_t        interval_milliseconds)
{
    CcnetTimer *timer = g_new0 (CcnetTimer, 1);

    timer->tv = timeval_from_msec (interval_milliseconds);
    timer->func = func;
    timer->user_data = user_data;

    evtimer_set (&timer->event, timer_callback, timer);
    evtimer_add (&timer->event, &timer->tv);

    return timer;
}
