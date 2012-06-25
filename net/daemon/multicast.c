#define MULT_RECV_TIMEOUT                 180

#define MULTICAST_PACKET_LEN 4096

#define CCNET_MULTICAST_ADDR   "224.0.1.4"
#define CCNET_MULTICAST_PORT   "10002"

static void check_peers_in_lan (CcnetConnManager *manager)
{
    int timeout = time(NULL) - MULT_RECV_TIMEOUT;
    GHashTableIter iter;
    gpointer key, value;
    CcnetPeer *peer;

    g_hash_table_iter_init (&iter, manager->session->peer_mgr->peer_hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        peer = value;
        if (peer->in_local_network && peer->last_mult_recv < timeout)
            peer->in_local_network = 0;
    }
}

static void send_multicast_packet (CcnetConnManager *manager)
{
    int len;
    const char *id = manager->session->base.id;
    char buf[MULTICAST_PACKET_LEN];
    static char *msg = NULL;
    ccnet_packet *packet = (ccnet_packet *)buf;

    if (!msg) {
        GString *str = ccnet_peer_to_string (manager->session->myself);
        msg = str->str;
        g_string_free (str, FALSE);
    }

    packet->header.version = 1;
    packet->header.type = CCNET_MSG_HANDSHAKE;
    /* note, if we use snprintf(packet->data, ...) here, We would receive
     * a warning when use CFLAGS="-g -O2": 
     *   call to __builtin___snprintf_chk will always overflow destination buffer
     * This warning will cause runtime buffer overflow.
     */
    len = snprintf (buf + sizeof(struct ccnet_header), 
                    MULTICAST_PACKET_LEN - sizeof(struct ccnet_header),
                    "%s %d\n%s", id,
                    manager->session->base.public_port, msg);
    if (len >= MULTICAST_PACKET_LEN) {
        /* see the man of snprintf() */
        ccnet_warning ("packet length exceeds multicast limitation.\n");
        return;
    }
    len += CCNET_PACKET_LENGTH_HEADER;
    packet->header.length = htons (len);
    packet->header.id = 0;

    if (sendto (manager->mcsnd_socket, buf, len, 0,
                manager->mc_sasend, manager->mc_salen) < 0) {
        ccnet_warning ("multicast send error: %s\n", strerror(errno));
        manager->multicast_error = 1;
    }

    check_peers_in_lan (manager);
    /* ccnet_debug ("[Conn] Send multicast packet %d\n", len); */
}

static void
multicast_recv (evutil_socket_t fd, short event, void *vmanager)
{
    CcnetConnManager *manager = vmanager;
    struct sockaddr_storage from;
    int len = sizeof (struct sockaddr_storage);
    int n;
    char buf[MULTICAST_PACKET_LEN];
    ccnet_packet *packet = (ccnet_packet *)buf;
    char *id, *ptr, *end, *info;
    unsigned short port;
    
    /* fprintf (stderr, "Recv a multicast message\n"); */

    n = recvfrom (fd, buf, MULTICAST_PACKET_LEN, 0,
                  (struct sockaddr *)&from, (socklen_t *)&len);
    buf[n] = '\0';

    if (G_UNLIKELY (n < 42 + CCNET_PACKET_LENGTH_HEADER 
                    || packet->data[40] != ' ')) {
        ccnet_warning ("Bad broadcast message from %s %.10s\n",
                       sock_ntop((struct sockaddr *)&from, len), buf);
        return;
    }

    id = packet->data;
    packet->data[40] = '\0';
    end = buf + n;
    for (ptr = packet->data + 40; *ptr != '\n' && ptr < end; ++ptr);
    if (ptr == end) {
        ccnet_warning ("Bad broadcast message from %s\n",
                       sock_ntop((struct sockaddr *)&from, len));
        return;
    }
    *ptr = '\0';
    port = atoi (packet->data + 41);
    info = ptr+1;

    if (port != 0) {
        CcnetPeer *peer;
        CcnetPeerManager *peerMgr = manager->session->peer_mgr;

        peer = ccnet_peer_manager_get_peer (peerMgr, id);
        if (!peer) {
            peer = ccnet_peer_from_string (info);
            if (!peer) {
                ccnet_debug ("[Conn] Multicast packet containing bad peer info\n");
                return;
            }
            ccnet_peer_manager_add_peer (peerMgr, peer);
        }

        peer->last_mult_recv = time(NULL);

        if (peer->is_self || peer->net_state == PEER_CONNECTED) {
            g_object_unref (peer);
            return;
        }

        if (!peer->in_local_network) {
            notify_found_peer (manager->session, peer);
            peer->in_local_network = 1;
        }
        ccnet_peer_update_address (peer, 
                 sock_ntop((struct sockaddr *)&from, len), port);
        g_object_unref (peer);
    }
}

static
void multicast_start (CcnetConnManager *manager)
{
    evutil_socket_t     sendfd, recvfd;
    socklen_t           salen;
    struct sockaddr     *sasend;

    ccnet_message ("[Conn] Multicast start\n");

    sendfd = udp_client (CCNET_MULTICAST_ADDR, CCNET_MULTICAST_PORT, 
                         &sasend, &salen);

    if (sendfd < 0) {
        ccnet_debug ("Multicast failed to create udp client\n");
        return;
    }

    recvfd = create_multicast_sock (sasend, salen);
    if (recvfd < 0)
        return;

    event_set(&manager->mc_event, recvfd, EV_READ | EV_PERSIST, 
              multicast_recv, manager);
    event_add(&manager->mc_event, NULL);

    manager->mcsnd_socket = sendfd;
    manager->mcrcv_socket = recvfd;
    manager->mc_salen = salen;
    manager->mc_sasend = sasend;
}

static void stop_multicast(CcnetConnManager *manager)
{
    if (!manager->session->disable_multicast) {
        event_del (&manager->mc_event);
        evutil_closesocket (manager->mcsnd_socket);
        evutil_closesocket (manager->mcrcv_socket);
        manager->mcsnd_socket = 0;
        manager->mcrcv_socket = 0;
    }
}
