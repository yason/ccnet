

/* 
   Steps to add a new property:

   1. modify ccnet_user_to_string()
   2. modify ccnet_user_update_from_user()
   3. add P_xxx and modify 
        get_property(),
        set_property(),
        define_properties()
 */



G_DEFINE_TYPE (CcnetUser, ccnet_user, G_TYPE_OBJECT);

enum {
    P_ID = 1,
    P_NAME,
    P_IS_SELF,
    P_TIMESTAMP,
    P_ROLE_TIMESTAMP,
    P_MYROLE_TIMESTAMP,
    P_ROLE_LIST,
    P_MY_ROLE_LIST,
    P_DEFAULT_RELAY,
    P_PUBKEY,
};

static void
get_property (GObject *object, guint property_id,
              GValue *v, GParamSpec *pspec)
{
    CcnetUser *user = (CcnetUser *)object;
    GString *buf;

    switch (property_id) {
    case P_ID:
        g_value_set_string (v, user->id);
        break;
    case P_NAME:
        g_value_set_string (v, user->name);
        break;
    case P_TIMESTAMP:
        g_value_set_int64 (v, user->timestamp);
        break;
    case P_ROLE_TIMESTAMP:
        g_value_set_int64 (v, user->role_timestamp);
        break;
    case P_MYROLE_TIMESTAMP:
        g_value_set_int64 (v, user->myrole_timestamp);
        break;
    case P_IS_SELF:
        g_value_set_boolean (v, user->is_self);
        break;
    case P_ROLE_LIST:
        buf = g_string_new (NULL);
        string_list_join (user->role_list, buf, ",");
        g_value_take_string (v, buf->str);
        g_string_free (buf, FALSE);
        break;
    case P_MY_ROLE_LIST:
        buf = g_string_new (NULL);
        string_list_join (user->myrole_list, buf, ",");
        g_value_take_string (v, buf->str);
        g_string_free (buf, FALSE);
        break;
    case P_DEFAULT_RELAY:
        g_value_set_string (v, user->relay_id);
        break;
    case P_PUBKEY:
#ifndef CCNET_LIB
        if (user->pubkey) {
            GString *str = public_key_to_gstring(user->pubkey);
            g_value_set_string (v, str->str);
            g_string_free(str, TRUE);
        } else
            g_value_set_string (v, NULL);
#else
        g_value_set_string (v, NULL);
#endif
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
set_roles (CcnetUser *user, const char *roles)
{
    if (!roles)
        return;
    GList *role_list = string_list_parse_sorted (roles, ",");
    
    string_list_free (user->role_list);
    user->role_list = role_list;
}

static void
set_my_roles (CcnetUser *user, const char *roles)
{
    if (!roles)
        return;
    GList *role_list = string_list_parse_sorted (roles, ",");
    
    string_list_free (user->myrole_list);
    user->myrole_list = role_list;
}

static void
set_property (GObject *object, guint property_id, 
              const GValue *v, GParamSpec *pspec)
{
    CcnetUser *user = (CcnetUser *)object;
#ifndef CCNET_LIB
    const char *pubkey_str;
#endif

    switch (property_id) {
    case P_ID:
        memcpy(user->id, g_value_get_string(v), 41);
        break;
    case P_NAME:
        if (g_value_get_string(v)) {
            g_free (user->name);
            user->name = g_value_dup_string(v);
            if (strlen (user->name) > 16)
                user->name[16] = '\0';
        }
        break;
    case P_IS_SELF:
        user->is_self = g_value_get_boolean(v);
        break;
    case P_TIMESTAMP:
        user->timestamp = g_value_get_int64 (v);
        break;
    case P_ROLE_TIMESTAMP:
        user->role_timestamp = g_value_get_int64 (v);
        break;
    case P_MYROLE_TIMESTAMP:
        user->myrole_timestamp = g_value_get_int64 (v);
        break;
    case P_ROLE_LIST:
        set_roles (user, g_value_get_string(v));
        break;
    case P_MY_ROLE_LIST:
        set_my_roles (user, g_value_get_string(v));
        break;
    case P_DEFAULT_RELAY:
        g_free (user->relay_id);
        user->relay_id = g_value_dup_string(v); 
        break;
    case P_PUBKEY:
#ifndef CCNET_LIB
        pubkey_str = g_value_get_string(v);
        if (pubkey_str) {
            if (user->pubkey)
                RSA_free(user->pubkey);
            user->pubkey = public_key_from_string ((char *)pubkey_str);
        }
#endif
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
        g_param_spec_string ("id", NULL, "User ID",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_NAME,
        g_param_spec_string ("name", NULL, "Name",
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_TIMESTAMP,
        g_param_spec_int64 ("timestamp", NULL, "Timestamp",
                            0, G_MAXINT64, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_ROLE_TIMESTAMP,
        g_param_spec_int64 ("role-timestamp", NULL, "Role Timestamp",
                            0, G_MAXINT64, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_MYROLE_TIMESTAMP,
        g_param_spec_int64 ("myrole-timestamp", NULL, "My Role Timestamp",
                            0, G_MAXINT64, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_IS_SELF,
        g_param_spec_boolean ("is-self", NULL, "Is self",
                              0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_ROLE_LIST,
        g_param_spec_string ("role-list", NULL, "Role list",
                              NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_MY_ROLE_LIST,
        g_param_spec_string ("myrole-list", NULL, "My role list",
                              NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_DEFAULT_RELAY,
        g_param_spec_string ("default-relay", NULL, "Default Relay", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_PUBKEY,
        g_param_spec_string ("pubkey", NULL, "Pubkey",
                              NULL, G_PARAM_READWRITE));
}

static void
user_finalize (GObject *gobject)
{
    CcnetUser *user = CCNET_USER(gobject);
    GList *ptr;

#ifndef CCNET_LIB
    if (user->pubkey)
        RSA_free (user->pubkey);
#endif

    g_free (user->name);
    g_free (user->relay_id);

#ifndef CCNET_LIB
    for (ptr = user->peers; ptr; ptr = ptr->next)
        g_free (ptr->data);
    g_list_free (user->peers);
#endif

    for (ptr = user->role_list; ptr; ptr = ptr->next)
        g_free (ptr->data);
    g_list_free (user->role_list);

    for (ptr = user->myrole_list; ptr; ptr = ptr->next)
        g_free (ptr->data);
    g_list_free (user->myrole_list);
    
    G_OBJECT_CLASS(ccnet_user_parent_class)->finalize (gobject);
}
