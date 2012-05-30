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


#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

typedef struct StringArray StringArray;

struct StringArray {
    char **strs;
    int len;
    int alloc;
};

#define string_array_index(array,idx) ((array)->strs)[idx]

StringArray* string_array_new ();
void string_array_free (StringArray *array, gboolean free_content);

/* caller should call strdup() if necessary */
void string_array_append (StringArray *array, char *string);

StringArray* string_array_from_text (const char *text);
void string_array_to_string_buf (StringArray *array, GString *buf);
char* string_array_to_text (StringArray *array);

void strings_to_string_buf (GString *buf, ...) G_GNUC_NULL_TERMINATED;

#endif
