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

#ifndef CCNET_TIMER_H
#define CCNET_TIMER_H

/* return TRUE to reschedule the timer, return FALSE to cancle the timer */
typedef int (*TimerCB) (void *data);

struct CcnetTimer;

typedef struct CcnetTimer CcnetTimer;

/**
 * Calls timer_func(user_data) after the specified interval.
 * The timer is freed if timer_func returns zero.
 * Otherwise, it's called again after the same interval.
 */
CcnetTimer* ccnet_timer_new (TimerCB           func,
                             void             *user_data,
                             uint64_t          timeout_milliseconds);

/**
 * Frees a timer and sets the timer pointer to NULL.
 */
void ccnet_timer_free (CcnetTimer **timer);


#endif
