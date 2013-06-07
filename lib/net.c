/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef WIN32
    #define WINVER 0x0501
#endif
#include "include.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>


#ifdef WIN32
    #include <inttypes.h>
    #include <winsock2.h>
    #include <ctype.h>
    #include <ws2tcpip.h>
    #define UNUSED 
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/un.h>
    #include <net/if.h>
    #include <netinet/tcp.h>
#endif

#include <fcntl.h>

#include <evutil.h>

#include "net.h"


#ifdef WIN32

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#endif

#ifndef IN6ADDRSZ
#define IN6ADDRSZ       16
#endif

#ifndef INT16SZ
#define INT16SZ         2
#endif

#ifndef INADDRSZ
#define INADDRSZ        4
#endif

#ifndef inet_ntop
static const char *inet_ntop4(const u_char *src, char *dst, size_t size) 
{
    static const char fmt[] = "%u.%u.%u.%u";  
    char tmp[sizeof("255.255.255.255")];  
    int l;
    l = _snprintf(tmp, size, fmt, src[0], src[1], src[2], src[3]); 
    if (l <= 0 || l >= size) {   
        return (NULL);  
    }  
    strncpy(dst, tmp, size);  
    return (dst);
}
static const char *inet_ntop6(const u_char *src, char *dst, size_t size) 
{
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];  
    char *tp, *ep;  
    struct 
    { 
        int base, len; 
    } best, cur;  
    u_int words[IN6ADDRSZ / INT16SZ];  
    int i;
    int advance;  
    memset(words, '\0', sizeof(words));  
    for (i = 0; i < IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3)); 
    best.base = -1;
    cur.base = -1;
    best.len = -1;
    cur.len = -1;
    for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
        if (words[i] == 0) { 
            if (cur.base == -1) 
                cur.base = i, cur.len = 1;  
            else
                cur.len++; 
        } 
        else { 
            if (cur.base != -1) { 
                if (best.base == -1 || cur.len > best.len) 
                    best = cur; 
                cur.base = -1; 
            } 
        } 
    } 
    if (cur.base != -1) { 
        if (best.base == -1 || cur.len > best.len) 
            best = cur;  
    } 
    if (best.base != -1 && best.len < 2) 
        best.base = -1;  
    tp = tmp; 
    ep = tmp + sizeof(tmp); 
    for (i = 0; i < (IN6ADDRSZ / INT16SZ) && tp < ep; i++) { 
/** Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base && 
            i < (best.base + best.len)) { 
            if (i == best.base) { 
                if (tp + 1 >= ep)  
                    return (NULL); 
                *tp++ = ':'; 
            } 
            continue; 
        } 
/** Are we following an initial run of 0x00s or any real hex? */
        if (i != 0) { 
            if (tp + 1 >= ep) 
                return (NULL); 
            *tp++ = ':'; 
        } 
/** Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 && 
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) { 
            if (!inet_ntop4(src+12, tp, (size_t)(ep - tp))) 
                return (NULL); 
            tp += strlen(tp); 
            break; 
        } 
        advance = snprintf(tp, ep - tp, "%x", words[i]); 
        if (advance <= 0 || advance >= ep - tp) 
            return (NULL); 
        tp += advance; 
    } 
/** Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == (IN6ADDRSZ / INT16SZ)) { 
        if (tp + 1 >= ep) 
            return (NULL); 
        *tp++ = ':'; 
    } 
    if (tp + 1 >= ep) 
        return (NULL); 
    *tp++ = '\0'; 
 
/** 
 * Check for overflow, copy, and we're done. 
 */
    if ((size_t)(tp - tmp) > size) { 
        errno = ENOSPC;  
        return (NULL); 
    } 
    strncpy(dst, tmp, size);
    dst[size] = '\0';
    return (dst); 
}
const char *inet_ntop(int af, const void *src, char *dst, size_t size) 
{
    switch (af) { 
    case AF_INET: 
        return (inet_ntop4(src, dst, size)); 
    case AF_INET6: 
        return (inet_ntop6(src, dst, size)); 
    default: 
        return (NULL);  
    } 
/** NOTREACHED */
}
#endif //inet_ntop

#ifndef inet_aton
int inet_aton(const char *string, struct in_addr *addr)
{
    addr->s_addr = inet_addr(string);
    if (addr->s_addr != -1 || strcmp("255.255.255.255", string) == 0)
        return 1;
    return 0;
}
#endif

#ifndef inet_pton
/*
 *  Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

/* int
 * inet_pton4(src, dst, pton)
 *      when last arg is 0: inet_aton(). with hexadecimal, octal and shorthand.
 *      when last arg is 1: inet_pton(). decimal dotted-quad only.
 * return:
 *      1 if `src' is a valid input, else 0.
 * notice:
 *      does not touch `dst' unless it's returning 1.
 * author:
 *      Paul Vixie, 1996.
 */
int inet_pton4(const char *src, u_char *dst, int pton)
{
    u_int val;
    u_int digit;
    int base, n;
    unsigned char c;
    u_int parts[4];
    register u_int *pp = parts;

    c = *src;
    for (;;) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, isdigit=decimal.
         */
        if (!isdigit(c))
            return (0);
        val = 0; base = 10;
        if (c == '0') {
            c = *++src;
            if (c == 'x' || c == 'X')
                base = 16, c = *++src;
            else if (isdigit(c) && c != '9')
                base = 8;
        }
        /* inet_pton() takes decimal only */
        if (pton && base != 10)
            return (0);
        for (;;) {
            if (isdigit(c)) {
                digit = c - '0';
                if (digit >= base)
                    break;
                val = (val * base) + digit;
                c = *++src;
            } else if (base == 16 && isxdigit(c)) {
                digit = c + 10 - (islower(c) ? 'a' : 'A');
                if (digit >= 16)
                    break;
                val = (val << 4) | digit;
                c = *++src;
            } else
                break;
        }
        if (c == '.') {
            /*
             * Internet format:
             *      a.b.c.d
             *      a.b.c   (with c treated as 16 bits)
             *      a.b     (with b treated as 24 bits)
             *      a       (with a treated as 32 bits)
             */
            if (pp >= parts + 3)
                return (0);
            *pp++ = val;
            c = *++src;
        } else
            break;
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && !isspace(c))
        return (0);
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = pp - parts + 1;
    /* inet_pton() takes dotted-quad only.  it does not take shorthand. */
    if (pton && n != 4)
        return (0);
    switch (n) {

    case 0:
        return (0);             /* initial nondigit */

    case 1:                         /* a -- 32 bits */
        break;

    case 2:                         /* a.b -- 8.24 bits */
        if (parts[0] > 0xff || val > 0xffffff)
            return (0);
        val |= parts[0] << 24;
        break;

    case 3:                         /* a.b.c -- 8.8.16 bits */
        if ((parts[0] | parts[1]) > 0xff || val > 0xffff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
        if ((parts[0] | parts[1] | parts[2] | val) > 0xff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }
    if (dst) {
        val = htonl(val);
        memcpy(dst, &val, INADDRSZ);
    }
    return (1);
}

/* int
 * inet_pton6(src, dst)
 *      convert presentation level address to network order binary form.
 * return:
 *      1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *      (1) does not touch `dst' unless it's returning 1.
 *      (2) :: in a full address is silently ignored.
 * credit:
 *      inspired by Mark Andrews.
 * author:
 *      Paul Vixie, 1996.
 */
int inet_pton6(const char *src, u_char *dst)
{
    static const char xdigits_l[] = "0123456789abcdef",
        xdigits_u[] = "0123456789ABCDEF";
    u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit;
    u_int val;

    memset((tp = tmp), '\0', IN6ADDRSZ);
    endp = tp + IN6ADDRSZ;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return (0);
    curtok = src;
    saw_xdigit = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return (0);
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!saw_xdigit) {
                if (colonp)
                    return (0);
                colonp = tp;
                continue;
            } else if (*src == '\0')
                return (0);
            if (tp + INT16SZ > endp)
                return (0);
            *tp++ = (u_char) (val >> 8) & 0xff;
            *tp++ = (u_char) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
            inet_pton4(curtok, tp, 1) > 0) {
            tp += INADDRSZ;
            saw_xdigit = 0;
            break;  /* '\0' was seen by inet_pton4(). */
        }
        return (0);
    }
    if (saw_xdigit) {
        if (tp + INT16SZ > endp)
            return (0);
        *tp++ = (u_char) (val >> 8) & 0xff;
        *tp++ = (u_char) val & 0xff;
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        if (tp == endp)
            return (0);
        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return (0);
    memcpy(dst, tmp, IN6ADDRSZ);
    return (1);
}

/* int
 * inet_pton(af, src, dst)
 *      convert from presentation format (which usually means ASCII printable)
 *      to network format (which is usually some kind of binary format).
 * return:
 *      1 if the address was valid for the specified address family
 *      0 if the address wasn't valid (`dst' is untouched in this case)
 *      -1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *      Paul Vixie, 1996.
 */
int inet_pton(int af, const char *src, void *dst)
{
     switch (af) {
    case AF_INET:
        return (inet_pton4(src, dst, 1));
    case AF_INET6:
        return (inet_pton6(src, dst));
    default:
        errno = EAFNOSUPPORT;
        return (-1);
    }
    /* NOTREACHED */
}
#endif



#endif //WIN32

int
ccnet_netSetTOS (evutil_socket_t s, int tos)
{
#ifdef IP_TOS
    return setsockopt( s, IPPROTO_IP, IP_TOS, (char*)&tos, sizeof( tos ) );
#else
    return 0;
#endif
}

static evutil_socket_t
makeSocketNonBlocking (evutil_socket_t fd)
{
    if (fd >= 0)
    {
        if (evutil_make_socket_nonblocking(fd))
        {
            ccnet_warning ("Couldn't make socket nonblock: %s",
                           evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
            evutil_closesocket(fd);
            fd = -1;
        }
    }
    return fd;
}

static evutil_socket_t
createSocket (int family, int nonblock)
{
    evutil_socket_t fd;
    int ret;

    fd = socket (family, SOCK_STREAM, 0);

    if (fd < 0) {
        ccnet_warning("create Socket failed %d\n", fd);
    } else if (nonblock) {
        int nodelay = 1;

        fd = makeSocketNonBlocking( fd );

        ret = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY,
                          (char *)&nodelay, sizeof(nodelay));
        if (ret < 0) {
            ccnet_warning("setsockopt failed\n");
            evutil_closesocket(fd);
            return -1;
        }
    }

    return fd;
}

evutil_socket_t
ccnet_net_open_tcp (const struct sockaddr *sa, int nonblock)
{
    evutil_socket_t s;
    int sa_len;

    if( (s = createSocket(sa->sa_family, nonblock)) < 0 )
        return -1;

#ifndef WIN32
    if (sa->sa_family == AF_INET)
        sa_len = sizeof (struct sockaddr_in); 
    else
        sa_len = sizeof (struct sockaddr_in6);
#else
    if (sa->sa_family == AF_INET)
        sa_len = sizeof (struct sockaddr_in); 
    else
        return -1;
#endif


    if( (connect(s, sa, sa_len) < 0)
#ifdef WIN32
        && (sockerrno != WSAEWOULDBLOCK)
#endif
        && (sockerrno != EINPROGRESS) )
    {
        evutil_closesocket(s);
        s = -1;
    }

    return s;
}

evutil_socket_t
ccnet_net_bind_tcp (int port, int nonblock)
{
#ifndef WIN32
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;
    char buf[10];
        
    memset (&hints, 0,sizeof (struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf (buf, sizeof(buf), "%d", port);

    if ( (n = getaddrinfo(NULL, buf, &hints, &res) ) != 0) {
        ccnet_warning ("getaddrinfo fails: %s\n", gai_strerror(n));
        return -1;
    }

    ressave = res;
    
    do {
        int on = 1;

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;       /* error - try next one */

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			ccnet_warning ("setsockopt of SO_REUSEADDR error\n");
            continue;
        }

        if (nonblock)
            sockfd = makeSocketNonBlocking (sockfd);
        if (sockfd < 0)
            continue;       /* error - try next one */

        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;          /* success */

        close(sockfd);      /* bind error - close and try next one */
    } while ( (res = res->ai_next) != NULL);

    freeaddrinfo (ressave);

    if (res == NULL) {
        ccnet_warning ("bind fails: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
#else

    evutil_socket_t s;
    struct sockaddr_in sock;
    const int type = AF_INET;
#if defined( SO_REUSEADDR ) || defined( SO_REUSEPORT )
    int optval;
#endif

    if ((s = createSocket(type, nonblock)) < 0)
        return -1;

    optval = 1;
    setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

    memset(&sock, 0, sizeof(sock));
    sock.sin_family      = AF_INET;
    sock.sin_addr.s_addr = INADDR_ANY;
    sock.sin_port        = htons(port);

    if ( bind(s, (struct sockaddr *)&sock, sizeof(struct sockaddr_in)) < 0)
    {
        ccnet_warning ("bind fails: %s\n", strerror(errno));
        evutil_closesocket (s);
        return -1;
    }
    if (nonblock)
        s = makeSocketNonBlocking (s);
     
    return s;
#endif
}

evutil_socket_t
ccnet_net_accept (evutil_socket_t b, struct sockaddr_storage *cliaddr, 
                  socklen_t *len, int nonblock)
{
    evutil_socket_t s;
    /* int nodelay = 1; */
    
    s = accept (b, (struct sockaddr *)cliaddr, len);

    /* setsockopt (s, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)); */
    if (nonblock)
        makeSocketNonBlocking(s);

    return s;
}


evutil_socket_t
ccnet_net_bind_v4 (const char *ipaddr, int *port)
{
    evutil_socket_t sockfd;
    struct sockaddr_in addr;
    int on = 1;
        
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ccnet_warning("create socket failed: %s\n", strerror(errno));
        exit(-1);
    }

    memset (&addr, 0, sizeof (struct sockaddr_in));
    addr.sin_family = AF_INET;
    if (inet_aton(ipaddr, &addr.sin_addr) == 0) {
        ccnet_warning ("Bad ip address %s\n", ipaddr);
        return -1;
    }
    addr.sin_port = htons (*port);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        ccnet_warning ("setsockopt of SO_REUSEADDR error: %s\n",
                       strerror(errno));
        return -1;
    }

    if ( bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ccnet_warning ("Bind error: %s\n", strerror (errno));
        return -1;
    }


    if (*port == 0) {
        struct sockaddr_storage ss;
        socklen_t len;

        len = sizeof(ss);
        if (getsockname(sockfd, (struct sockaddr *)&ss, &len) < 0) {
            ccnet_warning ("getsockname error: %s\n", strerror(errno));
            return -1;
        }
        *port = sock_port ((struct sockaddr *)&ss);
    }

    return sockfd;
}



char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];       /* Unix domain is largest */

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in  *sin = (struct sockaddr_in *) sa;

        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
            return(NULL);
        return(str);
    }

#ifdef  IPv6
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str) - 1) == NULL)
            return(NULL);
        return (str);
    }
#endif

#ifndef WIN32
#ifdef  AF_UNIX 
    case AF_UNIX: {
        struct sockaddr_un  *unp = (struct sockaddr_un *) sa;

            /* OK to have no pathname bound to the socket: happens on
               every connect() unless client calls bind() first. */
        if (unp->sun_path[0] == 0)
            strcpy(str, "(no pathname bound)");
        else
            snprintf(str, sizeof(str), "%s", unp->sun_path);
        return(str);
    }
#endif
#endif

    default:
        snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
                 sa->sa_family, salen);
        return(str);
    }
    return (NULL);
}

int
sock_pton (const char *addr_str, uint16_t port, struct sockaddr_storage *sa)
{
    struct sockaddr_in  *saddr  = (struct sockaddr_in *) sa;

#ifndef WIN32
    struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) sa;
#endif

    if (inet_pton (AF_INET, addr_str, &saddr->sin_addr) == 1 ) {
        saddr->sin_family = AF_INET;
        saddr->sin_port = htons (port);
        return 0;
    } 
#ifndef WIN32
    else if (inet_pton (AF_INET6, addr_str, &saddr6->sin6_addr) == 1)
    {
        saddr6->sin6_family = AF_INET6;
        saddr6->sin6_port = htons (port);
        return 0;
    }
#endif

    return -1;
}

/* return 1 if addr_str is a valid ipv4 or ipv6 address */
int
is_valid_ipaddr (const char *addr_str)
{
    struct sockaddr_storage addr;
    if (!addr_str)
        return 0;
    if (sock_pton(addr_str, 0, &addr) < 0)
        return 0;
    return 1;
}

uint16_t
sock_port (const struct sockaddr *sa)
{
    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in  *sin = (struct sockaddr_in *) sa;
        return ntohs(sin->sin_port);
    }
#ifdef  IPv6
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

        return ntohs(sin6->sin6_port);
    }
#endif
    default:
        return 0;
    }
    return 0;
}


evutil_socket_t
udp_client (const char *host, const char *serv,
            struct sockaddr **saptr, socklen_t *lenp)
{
	evutil_socket_t sockfd;
    int n;
	struct addrinfo	hints, *res, *ressave;

	memset (&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
        ccnet_warning ("udp_client error for %s, %s: %s",
                       host, serv, gai_strerror(n));
        return -1;
    }
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0)
			break;		/* success */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {	/* errno set from final socket() */
		ccnet_warning ("udp_client error for %s, %s", host, serv);
        freeaddrinfo (ressave);
        return -1;
    }

	*saptr = malloc(res->ai_addrlen);
	memcpy(*saptr, res->ai_addr, res->ai_addrlen);
	*lenp = res->ai_addrlen;

	freeaddrinfo(ressave);

	return (sockfd);
}


int
family_to_level(int family)
{
	switch (family) {
	case AF_INET:
		return IPPROTO_IP;
#ifdef	IPV6
	case AF_INET6:
		return IPPROTO_IPV6;
#endif
	default:
		return -1;
	}
}

int
sockfd_to_family(evutil_socket_t sockfd)
{
	struct sockaddr_storage ss;
	socklen_t	len;

	len = sizeof(ss);
	if (getsockname(sockfd, (struct sockaddr *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}
