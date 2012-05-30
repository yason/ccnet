/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


#define OBJECT_TYPE_STRING "group"

static void install_properties (GObjectClass *gobject_class);
static void set_property (GObject *object, guint property_id, 
                          const GValue *v, GParamSpec *pspec);
static void get_property (GObject *object, guint property_id, 
                          GValue *v, GParamSpec *pspec);

static void finalize (GObject *object)
{
    CcnetGroup *group = (CcnetGroup *) object;
    
    if (group->id)
        g_free (group->id);
    if (group->name)
        g_free (group->name);
    if (group->creator)
        g_free (group->creator);

    g_hash_table_destroy (group->members);
    g_hash_table_destroy (group->followers);
    g_hash_table_destroy (group->maintainers);
}

static void
ccnet_group_class_init (CcnetGroupClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
    gobject_class->finalize = finalize;

    install_properties (gobject_class);
}

static void
ccnet_group_init (CcnetGroup *group)
{
    group->members = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    group->followers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    group->maintainers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

void
ccnet_group_free (CcnetGroup *group)
{
    g_object_unref (group);
}

gboolean
ccnet_group_is_rendezvous (CcnetGroup *group, const char *peer_id)
{
    if (g_strcmp0(group->rendezvous->id, peer_id) == 0)
        return TRUE;
    return FALSE;
}

#ifdef CCNET_DAEMON
static void
set_members (CcnetGroup *group, char *string)
{
    char *id = string, *end, *id_copy;
    CcnetUser *member;
    CcnetUserManager *user_mgr = group->manager->session->user_mgr;

    g_hash_table_remove_all (group->members);

    end = id + 40;
    while (*end != '\0') {
        *end = '\0';

        if (!user_id_valid(id)) {
            g_warning ("[group] user id %s is not valid\n", id);
            return;
        }
        member = ccnet_user_manager_get_user (user_mgr, id);
        if (!member) {
            member = ccnet_user_new (id);
            ccnet_user_manager_add_user (user_mgr, member);
        }

        id_copy = g_strdup(id);
        g_hash_table_insert (group->members, id_copy, id_copy);

        id = end + 1;
        end = id + 40;
    }

    member = ccnet_user_manager_get_user (user_mgr, id);
    if (!member) {
        member = ccnet_user_new (id);
        ccnet_user_manager_add_user (user_mgr, member);
    }

    id_copy = g_strdup(id);
    g_hash_table_insert (group->members, id_copy, id_copy);
}

static void
set_followers (CcnetGroup *group, char *string)
{
    char *id = string, *end, *id_copy;
    CcnetUser *follower;
    CcnetUserManager *user_mgr = group->manager->session->user_mgr;

    if (ccnet_group_is_rendezvous (group, group->manager->session->base.id)
        && g_hash_table_size (group->followers) != 0)
        return;

    g_hash_table_remove_all (group->followers);

    end = id + 40;
    while (*end != '\0') {
        *end = '\0';

        follower = ccnet_user_manager_get_user (user_mgr, id);
        if (!follower) {
            follower = ccnet_user_new (id);
            ccnet_user_manager_add_user (user_mgr, follower);
        }
        id_copy = g_strdup(id);
        g_hash_table_insert (group->followers, id_copy, id_copy);

        id = end + 1;
        end = id + 40;
    }

    follower = ccnet_user_manager_get_user (user_mgr, id);
    if (!follower) {
        follower = ccnet_user_new (id);
        ccnet_user_manager_add_user (user_mgr, follower);
    }
    id_copy = g_strdup(id);
    g_hash_table_insert (group->followers, id_copy, id_copy);
}

static void
set_maintainers (CcnetGroup *group, char *string)
{
    char *id = string, *end, *id_copy;
    CcnetUser *maintainer;
    CcnetUserManager *user_mgr = group->manager->session->user_mgr;

    g_hash_table_remove_all (group->maintainers);

    end = id + 40;
    while (*end != '\0') {
        *end = '\0';

        maintainer = ccnet_user_manager_get_user (user_mgr, id);
        if (!maintainer) {
            maintainer = ccnet_user_new (id);
            ccnet_user_manager_add_user (user_mgr, maintainer);
        }

        id_copy = g_strdup(id);
        g_hash_table_insert (group->maintainers, id_copy, id_copy);

        id = end + 1;
        end = id + 40;
    }

    maintainer = ccnet_user_manager_get_user (user_mgr, id);
    if (!maintainer) {
        maintainer = ccnet_user_new (id);
        ccnet_user_manager_add_user (user_mgr, maintainer);
    }

    id_copy = g_strdup(id);
    g_hash_table_insert (group->maintainers, id_copy, id_copy);
}
#else
static void
set_members (CcnetGroup *group, char *string)
{
}
static void
set_followers (CcnetGroup *group, char *string)
{
}
static void
set_maintainers (CcnetGroup *group, char *string)
{
}
#endif

inline static void
user_table_to_string_buf (GHashTable *user_table, GString *str)
{
    const gchar *user_id;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, user_table);

    /* Should have at least one user */
    if (!g_hash_table_iter_next (&iter, &key, &value))
        return;
    user_id = value;
    g_string_append_printf (str, "%s", user_id);

    while (g_hash_table_iter_next (&iter, &key, &value)) {
        user_id = value;
        g_string_append_printf (str, " %s", user_id);
    }
}

void
ccnet_group_update_from_string (CcnetGroup *group, char *string)
{
    char *ptr, *start = string;
    
    if ( !(ptr = strchr(start, '\n')) ) return;
    *ptr = '\0';
    char *object_id = start;
    start = ptr + 1;
    
    char *object_type = ccnet_object_type_from_id (object_id);
    if (strcmp(object_type, OBJECT_TYPE_STRING) != 0)
        goto out;

    char *pure_id = object_id + strlen(object_type) + 1;
    g_return_if_fail (strcmp(pure_id, group->id) == 0);

    parse_key_value_pairs (
        start, (KeyValueFunc)ccnet_update_object_name_value, group);

out:
    g_free (object_type);
}

CcnetGroup*
ccnet_group_from_string (char *string, CcnetGroupManager *manager)
{
    char *ptr, *start = string;
    CcnetGroup *group = NULL;
    
    if ( !(ptr = strchr(start, '\n')) ) return NULL;
    *ptr = '\0';
    char *object_id = start;
    start = ptr + 1;
    
    char *object_type = ccnet_object_type_from_id (object_id);
    if (strcmp(object_type, OBJECT_TYPE_STRING) != 0)
        goto out;

    char *pure_id = object_id + strlen(object_type) + 1;
    g_assert (strlen(pure_id) == 36);

    group = ccnet_group_new (pure_id, manager);
    parse_key_value_pairs (
        start, (KeyValueFunc)ccnet_update_object_name_value, group);

out:
    g_free (object_type);
    return group;
}


void
ccnet_group_append_to_string (CcnetGroup *group, GString *buf)
{
    g_string_append_printf (buf, "group/%s\n", group->id);
    g_string_append_printf (buf, "name %s\n", group->name);
    g_string_append_printf (buf, "creator %s\n", group->creator);
    g_string_append_printf (buf, "timestamp %" G_GUINT64_FORMAT "\n",
                            group->timestamp);
    if (group->rendezvous)
        g_string_append_printf (buf, "rendezvous %s\n",
                                group->rendezvous->id);

    if (g_hash_table_size (group->members) != 0) {
        g_string_append (buf, "members ");
        user_table_to_string_buf (group->members, buf);
        g_string_append (buf, "\n");
    }

    if (g_hash_table_size (group->followers) != 0) {
        g_string_append (buf, "followers ");
        user_table_to_string_buf (group->followers, buf);
        g_string_append (buf, "\n");
    }

    if (g_hash_table_size (group->maintainers) != 0) {
        g_string_append (buf, "maintainers ");
        user_table_to_string_buf (group->maintainers, buf);
        g_string_append (buf, "\n");
    }
}


GString*
ccnet_group_to_string (CcnetGroup *group)
{
    GString *buf = g_string_new (NULL);

    ccnet_group_append_to_string (group, buf);

    return buf;
}


G_DEFINE_TYPE (CcnetGroup, ccnet_group, G_TYPE_OBJECT);

enum {
    P_ID = 1,
    P_NAME,
    P_CREATOR,
    P_RENDZVOUS,
    P_TIMESTAMP,
    P_MEMBERS,
    P_FOLLOWERS,
    P_MAINTAINERS,
};

static void
install_properties (GObjectClass *gobject_class)
{
    g_object_class_install_property (gobject_class, P_ID,
        g_param_spec_string ("id", NULL, "Unique Identity", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_NAME,
        g_param_spec_string ("name", NULL, "Name of the Group", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_CREATOR,
        g_param_spec_string ("creator", NULL, "Creator of the Group", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_RENDZVOUS,
        g_param_spec_string ("rendezvous", NULL, "The rendezvous of the Group", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_TIMESTAMP,
        g_param_spec_int64 ("timestamp", NULL, "Timestamp of the Group", 
                            0, G_MAXINT64, 0, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_MEMBERS,
        g_param_spec_string ("members", NULL, "Member list of the Group", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_FOLLOWERS,
        g_param_spec_string ("followers", NULL, "Peer list of the Group", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, P_MAINTAINERS,
        g_param_spec_string ("maintainers", NULL, "Maintainer list of the Group", 
                             NULL, G_PARAM_READWRITE));
}

/* 
 * Note: when a group is used in web, group manager is not accessible,
 * the member list, follow list, maintainer list is stored in string
 * not hash table.
 */

static void
get_property (GObject *object, guint property_id,
              GValue *v, GParamSpec *pspec)
{
    CcnetGroup *group = (CcnetGroup *)object;
    GString *buf;

    switch (property_id) {
    case P_ID:
        g_value_set_string (v, group->id);
        break;
    case P_NAME:
        g_value_set_string (v, group->name);
        break;
    case P_CREATOR:
        g_value_set_string (v, group->creator);
        break;
    case P_RENDZVOUS:
        if (group->manager) {
            g_value_set_string (v, group->rendezvous->id);
        } else
            g_value_set_string (v, group->rendezvous_str);
        break;
    case P_TIMESTAMP:
        g_value_set_int64 (v, group->timestamp);
        break;
    case P_MEMBERS:
        if (group->manager) {
            buf = g_string_new (NULL);
            user_table_to_string_buf (group->members, buf);
            g_value_take_string (v, buf->str);
            g_string_free (buf, FALSE);
        } else
            g_value_set_string (v, group->members_str);
        break;
    case P_FOLLOWERS:
        if (group->manager) {
            buf = g_string_new (NULL);
            user_table_to_string_buf (group->followers, buf);
            g_value_take_string (v, buf->str);
            g_string_free (buf, FALSE);
        } else
            g_value_set_string (v, group->followers_str);
        break;
    case P_MAINTAINERS:
        if (group->manager) {
            buf = g_string_new (NULL);
            user_table_to_string_buf (group->maintainers, buf);
            g_value_take_string (v, buf->str);
            g_string_free (buf, FALSE);
        } else
            g_value_set_string (v, group->maintainers_str);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
set_property (GObject *object, guint property_id, 
              const GValue *v, GParamSpec *pspec)
{
    CcnetGroup *group = (CcnetGroup *)object;
    char *members, *maintainers, *followers;
#ifdef CCNET_DAEMON
    CcnetPeerManager *peer_mgr;
    CcnetPeer *peer = NULL;
#endif
    char *id = NULL;

    switch (property_id) {
    case P_ID:
        group->id = g_value_dup_string (v);
        break;
    case P_NAME:
        group->name = g_value_dup_string (v);
        break;
    case P_CREATOR:
        group->creator = g_value_dup_string (v);
        break;
    case P_RENDZVOUS:
        id = (char *)g_value_get_string (v);
#ifdef CCNET_DAEMON
        if (group->manager) {
            peer_mgr = group->manager->session->peer_mgr;
            peer = ccnet_peer_manager_get_peer (peer_mgr, id);
            if (!peer) {
                peer = ccnet_peer_new (id);
                ccnet_peer_manager_add_peer (peer_mgr, peer);
            }
            group->rendezvous = peer;
        } else
#endif
            group->rendezvous_str = g_strdup(id);
        break;
    case P_TIMESTAMP:
        group->timestamp = g_value_get_int64 (v);
        break;
    case P_MEMBERS:
        members = g_value_dup_string (v);
        if (group->manager) {
            set_members (group, members);
            g_free (members);
        } else
            group->members_str = members;
        break;
    case P_FOLLOWERS:
        followers = g_value_dup_string (v);
        if (group->manager) {
            set_followers (group, followers);
            g_free (followers);
        } else
            group->followers_str = followers;
        break;
    case P_MAINTAINERS:
        maintainers = g_value_dup_string (v);
        if (group->manager) {
            set_maintainers (group, maintainers);
            g_free (maintainers);
        } else
            group->maintainers_str = maintainers;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

