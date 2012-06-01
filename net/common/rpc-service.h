/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

struct CcnetSession;

void ccnet_start_rpc(CcnetSession *session);

char *ccnet_rpc_list_peers(GError **error);
GList *ccnet_rpc_list_resolving_peers (GError **error);


char *ccnet_rpc_get_peers_by_role(const char *role, GError **error);
char *ccnet_rpc_get_peers_by_myrole(const char *myrole, GError **error);

GObject *ccnet_rpc_get_peer(const char *peerid, GError **error);


GObject *ccnet_rpc_get_peer_by_idname(const char *idname, GError **error);

char *
ccnet_rpc_list_users(GError **error);

GObject *
ccnet_rpc_get_user(const char *userid, GError **error);


GObject *
ccnet_rpc_get_user_of_peer(const char *peerid, GError **error);


GObject *ccnet_rpc_get_session_info(GError **error);

int
ccnet_rpc_add_client(const char *user_id, GError **error);

int
ccnet_rpc_add_role(const char *user_id, const char *role, GError **error);

int
ccnet_rpc_remove_role(const char *user_id, const char *role, GError **error);


GList *ccnet_rpc_get_events(int offset, int limit, GError **error);
int ccnet_rpc_count_event (GError **error);

GList *ccnet_rpc_get_procs_alive(int offset, int limit, GError **error);
int ccnet_rpc_count_procs_alive(GError **error);

GList *ccnet_rpc_get_procs_dead(int offset, int limit, GError **error);
int ccnet_rpc_count_procs_dead(GError **error);


/**
 * ccnet_get_config:
 * 
 * Return the config value with key @key
 */
char *
ccnet_rpc_get_config (const char *key, GError **error);

/**
 * ccnet_rpc_set_config:
 *
 * Set the value of config item with key @key to @value
 */
int
ccnet_rpc_set_config (const char *key, const char *value, GError **error);


/**
 * ccnet_rpc_upload_profile:
 *
 * Upload profile to the relay who is @relay_id
 */
int
ccnet_rpc_upload_profile (const char *relay_id, GError **error);

#ifdef CCNET_SERVER
int
ccnet_rpc_add_emailuser (const char *email, const char *passwd,
                         int is_staff, int is_active, GError **error);

int
ccnet_rpc_remove_emailuser (const char *email, GError **error);

int
ccnet_rpc_validate_emailuser (const char *email, const char *passwd, GError **error);

GObject*
ccnet_rpc_get_emailuser (const char *email, GError **error);

GObject*
ccnet_rpc_get_emailuser_by_id (int id, GError **error);

GList*
ccnet_rpc_get_emailusers (int start, int limit, GError **error);

int
ccnet_rpc_update_emailuser (int id, const char* passwd, int is_staff, int is_active,
                            GError **error);

int
ccnet_rpc_add_binding (const char *email, const char *peer_id, GError **error);

char *
ccnet_rpc_get_binding_email (const char *peer_id, GError **error);

char *
ccnet_rpc_get_binding_peerids (const char *email, GError **error);

int
ccnet_rpc_remove_binding (const char *email, GError **error);

int
ccnet_rpc_remove_one_binding (const char *email, const char *peer_id,
                              GError **error);

GList *
ccnet_rpc_get_peers_by_email (const char *email, GError **error);

char *
ccnet_rpc_sign_message (const char *message, GError **error);

int
ccnet_rpc_verify_message (const char *message,
                          const char *sig_base64,
                          const char *peer_id,
                          GError **error);

int
ccnet_rpc_create_group (const char *group_name, const char *user_name,
                        GError **error);
int
ccnet_rpc_remove_group (int group_id, const char *user_name, GError **error);

int
ccnet_rpc_group_add_member (int group_id, const char *user_name,
                            const char *member_name, GError **error);
int
ccnet_rpc_group_remove_member (int group_id, const char *user_name,
                               const char *member_name, GError **error);

int
ccnet_rpc_quit_group (int group_id, const char *user_name, GError **error);

GList *
ccnet_rpc_get_groups (const char *username, GError **error);

GList *
ccnet_rpc_get_all_groups (int start, int limit, GError **error);

GObject *
ccnet_rpc_get_group (int group_id, GError **error);

GList *
ccnet_rpc_get_group_members (int group_id, GError **error);

int
ccnet_rpc_check_group_staff (int group_id, const char *user_name,
                             GError **error);

#endif /* CCNET_SERVER */

/**
 * ccnet_rpc_login_to_relay:
 *
 * send email/passwd info to relay("after registration"), to get a "MyClient" role on relay
 */
int
ccnet_rpc_login_relay (const char *relay_id, const char *email,
                       const char *passwd, GError **error);

/**
 * ccnet_rpc_logout_to_relay:
 *
 * ask the relay to delete i) 'MyClient' info, ii) previous binding to an email 
 */
int
ccnet_rpc_logout_relay (const char *relay_id, GError **error);

#endif /* RPC_SERVICE_H */
