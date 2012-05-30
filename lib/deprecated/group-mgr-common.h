/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

void
ccnet_group_manager_free (CcnetGroupManager *manager)
{
    g_hash_table_unref (manager->group_id_hash);
}

void
ccnet_group_manager_clear (CcnetGroupManager *manager)
{
    g_hash_table_remove_all (manager->group_id_hash);
}

CcnetGroup *
ccnet_group_manager_get_group (CcnetGroupManager *manager, const char *group_id)
{
    return g_hash_table_lookup (manager->group_id_hash, group_id);
}

CcnetGroup *
ccnet_group_manager_get_group_by_name (CcnetGroupManager *manager,
                                       const char *group_name)
{
    return NULL;
}

GList *
ccnet_group_manager_get_groups (CcnetGroupManager *manager)
{
    return g_hash_table_get_values (manager->group_id_hash);
}

static void on_group_updated (CcnetGroup *group, GParamSpec *pspec,
                              CcnetGroupManager *manager)
{
    g_signal_emit (manager, signals[UPDATED_SIG], 0, group);
}

gboolean
ccnet_group_manager_add_group (CcnetGroupManager *manager, CcnetGroup *group)
{
    CcnetGroup *old_group;

    if (!group) return FALSE;

    old_group = ccnet_group_manager_get_group (manager, group->id);
    if (old_group) {
        g_warning ("Failed to add group %s: group exists.", group->id);
        return FALSE;
    }

    g_hash_table_insert (manager->group_id_hash, group->id, group);

    g_signal_connect (group, "notify", G_CALLBACK(on_group_updated), manager);

    g_signal_emit (manager, signals[ADDED_SIG], 0, group);
    g_signal_emit (manager, signals[OBJECT_ADDED_SIG], 0, group);

    return TRUE;
}

static gboolean
remove_group_common (CcnetGroupManager *manager, CcnetGroup *group)
{
    if (!group) return FALSE;

    if (!ccnet_group_manager_get_group (manager, group->id))
        return FALSE;

    g_signal_emit (manager, signals[DELETING_SIG], 0, group);
    g_signal_emit (manager, signals[OBJECT_DELETING_SIG], 0, group);

    g_hash_table_remove (manager->group_id_hash, group->id);
    ccnet_group_free (group);

    return TRUE;
}

G_DEFINE_TYPE (CcnetGroupManager, ccnet_group_manager, G_TYPE_OBJECT);
