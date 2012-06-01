/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>

#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "string-array.h"

#define MIN_ARRAY_SIZE 4
#define MAX_LEN 65535


StringArray*
string_array_new ()
{
    StringArray *array;

    array = g_new0 (StringArray, 1);
    
    return array;
}

void
string_array_free (StringArray *array, gboolean free_content)
{
    if (free_content) {
        int i;
        for (i = 0; i < array->len; i++)
            g_free (array->strs[i]);
    }

    g_free (array->strs);
    g_free (array);
}

static gint
nearest_pow (int num)
{
  int n = 1;

  while (n < num)
    n <<= 1;

  return n;
}


static inline void
maybe_expand (StringArray *array, int len)
{
    if (array->len + len > array->alloc) {
        array->alloc = nearest_pow (array->len + len);
        array->alloc = MAX (array->alloc, MIN_ARRAY_SIZE);
        array->strs = g_realloc (array->strs, sizeof(char *) * array->alloc);
    }
}

void
string_array_append (StringArray *array, char *string)
{
    maybe_expand (array, 1);
    array->strs[array->len++] = string;
}

void
string_array_to_string_buf (StringArray *array, GString *buf)
{
    int l, i;

    for (i = 0; i < array->len; i++) {
        l = strlen(array->strs[i]);
        g_string_append_printf (buf, "%d ", l);
        g_string_append (buf, array->strs[i]);
    }
}

char*
string_array_to_text (StringArray *array)
{
    GString *buf = g_string_new (NULL);
    string_array_to_string_buf (array, buf);

    char *str = buf->str;
    g_string_free (buf, FALSE);
    return str;
}


StringArray* string_array_from_text (const char *text)
{
    const char *p, *end;
    char *str;
    int len;
    StringArray *array = string_array_new ();
    
    
    p = text;
    while (1) {
        len = atoi(p);
        if (len < 0 || len > MAX_LEN) goto error;
        while (*p != ' ' && *p != '\0') p++;
        if (*p == '\0') goto error;
        
        p++;
        end = p + len;
        str = g_malloc (len + 1);
        memcpy (str, p, len);
        str[len] = '\0';
        string_array_append (array, str);
        
        if (*end == '\0') break;
        
        p = end;
    }
    
    return array;

error:
    string_array_free (array, TRUE);
    return NULL;
}


void
strings_to_string_buf (GString *buf, ...)
{
    va_list   args;
    const char *string;

	va_start (args, buf);

    string = va_arg (args, const char *);
    while (string != NULL) {
        int l = strlen(string);
        g_string_append_printf (buf, "%d ", l);
        g_string_append (buf, string);

        string = va_arg (args, const char *);
    }

    va_end (args);
}

