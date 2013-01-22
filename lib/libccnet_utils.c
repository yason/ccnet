/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <config.h>

#include "libccnet_utils.h"

#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <Rpc.h>
    #include <shlobj.h>
    #include <psapi.h>
#else
    #include <arpa/inet.h>
#endif

#ifndef WIN32
#include <pwd.h>
#include <uuid/uuid.h>
#endif

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>


#include <glib.h>
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <searpc-utils.h>

#ifdef WIN32
int
ccnet_util_pgpipe (ccnet_pipe_t handles[2])
{
    SOCKET s;
    struct sockaddr_in serv_addr;
    int len = sizeof( serv_addr );

    handles[0] = handles[1] = INVALID_SOCKET;

    if ( ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
    {
        g_debug("pgpipe failed to create socket: %d\n", WSAGetLastError());
        return -1;
    }

    memset( &serv_addr, 0, sizeof( serv_addr ) );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(0);
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (SOCKADDR *) & serv_addr, len) == SOCKET_ERROR)
    {
        g_debug("pgpipe failed to bind: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }
    if (listen(s, 1) == SOCKET_ERROR)
    {
        g_debug("pgpipe failed to listen: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }
    if (getsockname(s, (SOCKADDR *) & serv_addr, &len) == SOCKET_ERROR)
    {
        g_debug("pgpipe failed to getsockname: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }
    if ((handles[1] = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        g_debug("pgpipe failed to create socket 2: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }

    if (connect(handles[1], (SOCKADDR *) & serv_addr, len) == SOCKET_ERROR)
    {
        g_debug("pgpipe failed to connect socket: %d\n", WSAGetLastError());
        closesocket(s);
        return -1;
    }
    if ((handles[0] = accept(s, (SOCKADDR *) & serv_addr, &len)) == INVALID_SOCKET)
    {
        g_debug("pgpipe failed to accept socket: %d\n", WSAGetLastError());
        closesocket(handles[1]);
        handles[1] = INVALID_SOCKET;
        closesocket(s);
        return -1;
    }
    closesocket(s);
    return 0;
}
#endif

struct timeval
ccnet_util_timeval_from_msec (uint64_t milliseconds)
{
    struct timeval ret;
    const uint64_t microseconds = milliseconds * 1000;
    ret.tv_sec  = microseconds / 1000000;
    ret.tv_usec = microseconds % 1000000;
    return ret;
}

int
ccnet_util_checkdir (const char *dir)
{
    struct stat st;

#ifdef WIN32
    /* remove trailing '\\' */
    char *path = g_strdup(dir);
    char *p = (char *)path + strlen(path) - 1;
    while (*p == '\\' || *p == '/') *p-- = '\0';
    if ((g_stat(dir, &st) < 0) || !S_ISDIR(st.st_mode)) {
        g_free (path);
        return -1;
    }
    g_free (path);
    return 0;
#else
    if ((g_stat(dir, &st) < 0) || !S_ISDIR(st.st_mode))
        return -1;
    return 0;
#endif
}

int
ccnet_util_checkdir_with_mkdir (const char *dir)
{
#ifdef WIN32
    int ret;
    char *path = g_strdup(dir);
    char *p = (char *)path + strlen(path) - 1;
    while (*p == '\\' || *p == '/') *p-- = '\0';
    ret = g_mkdir_with_parents(path, 0755);
    g_free (path);
    return ret;
#else
    return g_mkdir_with_parents(dir, 0755);
#endif
}


ssize_t						/* Read "n" bytes from a descriptor. */
ccnet_util_recvn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
#ifndef WIN32
        if ( (nread = read(fd, ptr, nleft)) < 0)
#else
        if ( (nread = recv(fd, ptr, nleft, 0)) < 0)
#endif
        {
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;				/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		/* return >= 0 */
}

ssize_t						/* Write "n" bytes to a descriptor. */
ccnet_util_sendn (int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
#ifndef WIN32
        if ( (nwritten = write(fd, ptr, nleft)) <= 0)
#else
        if ( (nwritten = send(fd, ptr, nleft, 0)) <= 0)
#endif
        {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

char*
ccnet_util_expand_path (const char *src)
{
#ifdef WIN32
    char new_path[PATH_MAX + 1];
    char *p = new_path;
    const char *q = src;

    memset(new_path, 0, sizeof(new_path));
    if (*src == '~') {
        const char *home = g_get_home_dir();
        memcpy(new_path, home, strlen(home));
        p += strlen(new_path);
        q++;
    }
    memcpy(p, q, strlen(q));

    /* delete the charactor '\' or '/' at the end of the path
     * because the function stat faied to deal with directory names
     * with '\' or '/' in the end */
    p = new_path + strlen(new_path) - 1;
    while(*p == '\\' || *p == '/') *p-- = '\0';

    return strdup (new_path);
#else
    const char *next_in, *ntoken;
    char new_path[PATH_MAX + 1];
    char *next_out;
    int len;

   /* special cases */
    if (!src || *src == '\0')
        return NULL;
    if (strlen(src) > PATH_MAX)
        return NULL;

    next_in = src;
    next_out = new_path;
    *next_out = '\0';

    if (*src == '~') {
        /* handle src start with '~' or '~<user>' like '~plt' */
        struct passwd *pw = NULL;

        for ( ; *next_in != '/' && *next_in != '\0'; next_in++) ;

        len = next_in - src;
        if (len == 1) {
            pw = getpwuid (geteuid());
        } else {
            /* copy '~<user>' to new_path */
            memcpy (new_path, src, len);
            new_path[len] = '\0';
            pw = getpwnam (new_path + 1);
        }
        if (pw == NULL)
            return NULL;

        len = strlen (pw->pw_dir);
        memcpy (new_path, pw->pw_dir, len);
        next_out = new_path + len;
        *next_out = '\0';

        if (*next_in == '\0')
            return strdup (new_path);
    } else if (*src != '/') {       
        getcwd (new_path, PATH_MAX);
        for ( ; *next_out; next_out++) ; /* to '\0' */
    }

    while (*next_in != '\0') {
        /* move ntoken to the next not '/' char  */
        for (ntoken = next_in; *ntoken == '/'; ntoken++) ;

        for (next_in = ntoken; *next_in != '/'
                 && *next_in != '\0'; next_in++) ;

        len = next_in - ntoken;

        if (len == 0) {
            /* the path ends with '/', keep it */
            *next_out++ = '/';
            *next_out = '\0';
            break;
        }

        if (len == 2 && ntoken[0] == '.' && ntoken[1] == '.')
        {
            /* '..' */
            for (; next_out > new_path && *next_out != '/'; next_out--)
                ;
            *next_out = '\0';
        } else if (ntoken[0] != '.' || len != 1) {
            /* not '.' */
            *next_out++ = '/';
            memcpy (next_out, ntoken, len);
            next_out += len;
            *next_out = '\0';
        }
    }

    /* the final special case */
    if (new_path[0] == '\0') {
        new_path[0] = '/';
        new_path[1] = '\0';
    }
    return strdup (new_path);
#endif
}

#ifndef WIN32
char* ccnet_util_gen_uuid ()
{
    char *uuid_str = g_malloc (37);
    uuid_t uuid;

    uuid_generate (uuid);
    uuid_unparse_lower (uuid, uuid_str);

    return uuid_str;
}

#else
char* ccnet_util_gen_uuid ()
{
    char *uuid_str = g_malloc (37);
    unsigned char *str = NULL;
    UUID uuid;

    UuidCreate(&uuid);
    UuidToString(&uuid, &str);
    memcpy(uuid_str, str, 37);
    RpcStringFree(&str);
    return uuid_str;
}
#endif

char* ccnet_util_strjoin_n (const char *seperator, int argc, char **argv)
{
    GString *buf;
    int i;
    char *str;

    if (argc == 0)
        return NULL;

    buf = g_string_new (argv[0]);
    for (i = 1; i < argc; ++i) {
        g_string_append (buf, seperator);
        g_string_append (buf, argv[i]);
    }

    str = buf->str;
    g_string_free (buf, FALSE);
    return str;
}

/**
 * handle the empty string problem.
 */
gchar*
ccnet_util_key_file_get_string (GKeyFile *keyf,
                                const char *category,
                                const char *key)
{
    gchar *v;

    if (!g_key_file_has_key (keyf, category, key, NULL))
        return NULL;

    v = g_key_file_get_string (keyf, category, key, NULL);
    if (v != NULL && v[0] == '\0') {
        g_free(v);
        return NULL;
    }

    return v;
}

void
ccnet_util_string_list_free (GList *str_list)
{
    GList *ptr = str_list;

    while (ptr) {
        g_free (ptr->data);
        ptr = ptr->next;
    }

    g_list_free (str_list);
}

void
ccnet_util_string_list_join (GList *str_list, GString *str, const char *seperator)
{
    GList *ptr;
    if (!str_list)
        return;

    ptr = str_list;
    g_string_append (str, ptr->data);

    for (ptr = ptr->next; ptr; ptr = ptr->next) {
        g_string_append (str, seperator);
        g_string_append (str, (char *)ptr->data);
    }
}

static GList *
string_list_parse (const char *list_in_str, const char *seperator)
{
    if (!list_in_str)
        return NULL;

    GList *list = NULL;
    char **array = g_strsplit (list_in_str, seperator, 0);
    char **ptr;

    for (ptr = array; *ptr; ptr++) {
        list = g_list_prepend (list, g_strdup(*ptr));
    }
    list = g_list_reverse (list);

    g_strfreev (array);
    return list;
}

GList *
ccnet_util_string_list_parse_sorted (const char *list_in_str, const char *seperator)
{
    GList *list = string_list_parse (list_in_str, seperator);

    return g_list_sort (list, (GCompareFunc)g_strcmp0);
}

static unsigned hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return ~0;
}

int
ccnet_util_hex_to_rawdata (const char *hex_str,
                           unsigned char *rawdata,
                           int n_bytes)
{
    int i;
    for (i = 0; i < n_bytes; i++) {
        unsigned int val = (hexval(hex_str[0]) << 4) | hexval(hex_str[1]);
        if (val & ~0xff)
            return -1;
        *rawdata++ = val;
        hex_str += 2;
    }
    return 0;
}
