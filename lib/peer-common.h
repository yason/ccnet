

G_DEFINE_TYPE (CcnetPeer, ccnet_peer, G_TYPE_OBJECT);


enum {
    P_ID = 1,
    P_IS_SELF,
    P_NAME,
    P_PUBLIC_ADDR,
    P_PUBLIC_PORT,
    P_SERVICE_URL,
    P_IP,
    P_PORT,
    P_AUTH_STATE,
    P_NET_STATE,
    P_PUBKEY,
    P_CAN_CONNECT,              /* can be connected */
    P_IN_LOCAL_NET,
    P_LOGIN_STARTED,
    P_LOGIN_ERROR,
    P_LOGOUT_STARTED,
    P_IN_CONNECTION,
    P_ROLE_LIST,
    P_MY_ROLE_LIST,
    P_BIND_STATUS,
    P_BIND_EMAIL,
};

static void
get_property (GObject *object, guint property_id,
              GValue *v, GParamSpec *pspec)
{
    CcnetPeer *peer = (CcnetPeer *)object;
    GString *buf;

    switch (property_id) {
    case P_ID:
        g_value_set_string (v, peer->id);
        break;
    case P_IS_SELF:
        g_value_set_boolean (v, peer->is_self);
        break;
    case P_NAME:
        g_value_set_string (v, peer->name);
        break;
    case P_PUBLIC_ADDR:
        g_value_set_string (v, peer->public_addr);
        break;
    case P_PUBLIC_PORT:
        g_value_set_int (v, peer->public_port);
        break;
    case P_IP:
        g_value_set_string (v, peer->addr_str);
        break;
    case P_PORT:
        g_value_set_int (v, peer->port);
        break;
    case P_SERVICE_URL:
        g_value_set_string (v, peer->service_url);
        break;
    case P_NET_STATE:
        g_value_set_int (v, peer->net_state);
        break;
    case P_PUBKEY:
 #ifndef CCNET_LIB
        if (peer->pubkey) {
            GString *str = public_key_to_gstring(peer->pubkey);
            g_value_set_string (v, str->str);
            g_string_free(str, TRUE);
        } else
            g_value_set_string (v, NULL);
#else
        g_value_set_string (v, NULL);
#endif
        break;
    case P_CAN_CONNECT:
        g_value_set_boolean (v, peer->can_connect);
        break;
    case P_IN_LOCAL_NET:
        g_value_set_boolean (v, peer->in_local_network);
        break;
    case P_LOGIN_ERROR:
        g_value_set_string (v, peer->login_error);
        break;
    case P_LOGIN_STARTED:
        g_value_set_boolean (v, peer->login_started);
        break;
    case P_LOGOUT_STARTED:
        g_value_set_boolean (v, peer->logout_started);
        break;
    case P_IN_CONNECTION:
        g_value_set_boolean (v, peer->in_connection);
        break;
    case P_ROLE_LIST:
        buf = g_string_new (NULL);
        string_list_join (peer->role_list, buf, ",");
        g_value_take_string (v, buf->str);
        g_string_free (buf, FALSE);
        break;
    case P_MY_ROLE_LIST:
        buf = g_string_new (NULL);
        string_list_join (peer->myrole_list, buf, ",");
        g_value_take_string (v, buf->str);
        g_string_free (buf, FALSE);
        break;
    case P_BIND_STATUS:
        g_value_set_int (v, peer->bind_status);
        break;
    case P_BIND_EMAIL:
        g_value_set_string (v, peer->bind_email);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
set_roles (CcnetPeer *peer, const char *roles)
{
    if (!roles)
        return;
    GList *role_list = string_list_parse_sorted (roles, ",");
    
    string_list_free (peer->role_list);
    peer->role_list = role_list;
}

static void
set_my_roles (CcnetPeer *peer, const char *roles)
{
    if (!roles)
        return;
    GList *role_list = string_list_parse_sorted (roles, ",");
    
    string_list_free (peer->myrole_list);
    peer->myrole_list = role_list;
}

static void
set_property_common (GObject *object, guint property_id, 
              const GValue *v, GParamSpec *pspec)
{
    CcnetPeer *peer = (CcnetPeer *)object;

    switch (property_id) {
    case P_ID:
        memcpy(peer->id, g_value_get_string(v), 41);
        break;
    case P_NAME:
        g_free (peer->name);
        peer->name = g_value_dup_string(v);
        break;
    case P_IS_SELF:
        peer->is_self = g_value_get_boolean(v);
        break;
    case P_PUBLIC_ADDR:
        g_free (peer->public_addr);
        peer->public_addr = g_value_dup_string(v);
        break;
    case P_PUBLIC_PORT:
        peer->public_port = g_value_get_int (v);
        break;
    case P_SERVICE_URL:
        g_free (peer->service_url);
        peer->service_url = g_value_dup_string(v);
        break;
    case P_IP:
        g_free (peer->addr_str);
        peer->addr_str = g_value_dup_string(v);
        break;
    case P_PORT:
        peer->port = g_value_get_int (v);
        break;
    case P_NET_STATE:
        peer->net_state = g_value_get_int (v);
        break;
    case P_PUBKEY:
#ifndef CCNET_LIB
        if (peer->pubkey)
            RSA_free(peer->pubkey);
        peer->pubkey = public_key_from_string ((char *)g_value_get_string(v));
#endif
        break;
    case P_CAN_CONNECT:
        peer->can_connect = g_value_get_boolean (v);
        break;
    case P_IN_LOCAL_NET:
        peer->in_local_network = g_value_get_boolean (v);
        break;   
    case P_LOGIN_STARTED:
        peer->login_started = g_value_get_boolean (v);
        break;   
    case P_LOGIN_ERROR:
        g_free (peer->login_error);
        peer->login_error = g_value_dup_string (v);
        break;   
    case P_LOGOUT_STARTED:
        peer->logout_started = g_value_get_boolean (v);
        break;   
    case P_IN_CONNECTION:
        peer->in_connection = g_value_get_boolean (v);
        break;
    case P_ROLE_LIST:
        set_roles (peer, g_value_get_string(v));
        break;
    case P_MY_ROLE_LIST:
        set_my_roles (peer, g_value_get_string(v));
        break;
    case P_BIND_STATUS:
        peer->bind_status = g_value_get_int (v);
        break;
    case P_BIND_EMAIL:
        g_free (peer->bind_email);
        peer->bind_email = g_value_dup_string (v);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        return;
    }
}

static void
define_properties (GObjectClass *gobject_class)
{
    g_object_class_install_property (gobject_class, P_ID,
        g_param_spec_string ("id", NULL, "Node ID",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_NAME,
        g_param_spec_string ("name", NULL, "Hostname", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_PUBLIC_ADDR,
        g_param_spec_string ("public-addr", NULL, "Public Addrress",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_PUBLIC_PORT,
        g_param_spec_int ("public-port", NULL, "Public Port",
                          0, 65535, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_SERVICE_URL,
        g_param_spec_string ("service-url", NULL, "Service Url",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_IP,
        g_param_spec_string ("ip", NULL, "Dynamic IP", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_PORT,
        g_param_spec_int ("port", NULL, "Dynamic Port",
                          0, 65535, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_NET_STATE,
        g_param_spec_int ("net-state", NULL, "Network State",
                          -1, 3, 0, 
                          G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_IS_SELF,
        g_param_spec_boolean ("is-self", NULL, "Is self",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_PUBKEY,
        g_param_spec_string ("pubkey", NULL, "Public Key", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_CAN_CONNECT,
        g_param_spec_boolean ("can-connect", NULL, "Can be connect via TCP",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_IN_LOCAL_NET,
        g_param_spec_boolean ("in-local-network", NULL, "In local network",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_LOGIN_STARTED,
        g_param_spec_boolean ("login-started", NULL, "login has been started",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_LOGIN_ERROR,
        g_param_spec_string ("login-error", NULL, "login error",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_LOGOUT_STARTED,
        g_param_spec_boolean ("logout-started", NULL, "logout has been started",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_IN_CONNECTION,
        g_param_spec_boolean ("in-connection", NULL, "in connection",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_ROLE_LIST,
        g_param_spec_string ("role-list", NULL, "Role list",
                              NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_MY_ROLE_LIST,
        g_param_spec_string ("myrole-list", NULL, "My role list",
                              NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_BIND_STATUS,
        g_param_spec_int ("bind-status", NULL, "Bind Status",
                          0, 8, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_BIND_EMAIL,
        g_param_spec_string ("bind-email", NULL, "bind email",
                              NULL, G_PARAM_READWRITE));
}


const char*
ccnet_peer_get_net_state_string (int net_state)
{
    switch (net_state) {
    case PEER_DOWN: 
        return "down";
    case PEER_CONNECTED:
        return "connected";
    default:
        return "unknown";
    }
}
