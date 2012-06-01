/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

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
