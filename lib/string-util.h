/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * Copyright (C) 2010, GongGeng
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

#ifndef CCNET_STRING_H
#define CCNET_STRING_H

#include <stdio.h>

#define END_0(buf,len) (buf[(len)-1] == '\0')


static inline int get_version(char *str)
{
    int v;

    if (str[0] != 'v')
        return 0;
    if (sscanf(str+1, "%d", &v) != 1)
        return 0;
    return v;
}



#define sgoto_next(p) do {      \
        while (*p != ' ' && *p) ++p;            \
        if (*p != ' ')                          \
            goto error;                         \
        *p = '\0';                              \
        ++p;                                    \
    } while (0)

#define sget_len(val, p) do { \
        char *tmp = p;        \
        sgoto_next(p);        \
        val = atoi(tmp);        \
        if (val == 0)         \
            goto error;       \
    } while (0)


/* get a string with format "%s " */
#define sget_str(str, p) do { \
        str = p;              \
        sgoto_next(p);        \
    } while (0)

/* get a string with format "%d %s " */
#define sget_str_with_len(str, p) do { \
        int len;                       \
        sget_len(len, p);              \
        str = p;                       \
        p += len;                      \
        *p = '\0';                     \
        ++p;                           \
    } while (0)



#endif
