import threading

from pysearpc import SearpcClient, searpc_func, SearpcError

_SC_CLIENT_CALL = "301"
_SS_CLIENT_CALL = "CLIENT CALL"
_SC_CLIENT_MORE = "302"
_SS_CLIENT_MORE = "MORE"
_SC_SERVER_RET = "311"
_SS_SERVER_RET = "SERVER RET"
_SC_SERVER_MORE = "312"
_SS_SERVER_MORE = "HAS MORE"
_SC_SERVER_ERR = "411"
_SS_SERVER_ERR = "FAIL TO CALL"
_SC_PROC_DEAD = "102"

class NetworkError(Exception):
    def __init__(self, msg):
        self.msg = msg

    def __str__(self):
        return self.msg

class DeadProcError(Exception):
    def __str__(self):
        return "Processor is dead"


class RpcClientBase(SearpcClient):
    
    def __init__(self, ccnet_client_pool, service_name, retry_num=1,
                 is_remote=False, remote_peer_id='', req_pool=False):
        self.pool = ccnet_client_pool
        self.service_name = service_name
        self.retry_num = retry_num
        self.is_remote = is_remote
        self.remote_peer_id = remote_peer_id
        self.req_pool = req_pool
        if self.is_remote and len(self.remote_peer_id) != 40:
            raise ValueError("Invalid remote peer id")

    def _start_service(self, client):
        req_id = client.get_request_id()
        req_str = self.service_name
        if self.is_remote:
            req_str = "remote " + self.remote_peer_id + " " + self.service_name
        client.send_request(req_id, req_str)
        if client.read_response() < 0:
            raise NetworkError("Read response error")

        rsp = client.response
        if rsp[0] != "200":
            raise SearpcError("Error received: %s %s (In _start_service)" % (rsp[0], rsp[1]))
        return req_id

    def _real_call(self, client, req_id, fcall_str):
        client.send_update(req_id, "301", "CLIENT CALL", fcall_str, len(fcall_str))
                
        if client.read_response() < 0:
            raise NetworkError("Read response error")

        rsp = client.response
        if rsp[0] == _SC_SERVER_RET:
            return rsp[2]
        elif rsp[0] == _SC_SERVER_MORE:
            buf = rsp[2]
            while rsp[0] == _SC_SERVER_MORE:
                client.send_update(req_id, _SC_CLIENT_MORE,
                                   _SS_CLIENT_MORE, "", 0)
                if client.read_response() < 0:
                    raise NetworkError("Read response error")
                rsp = client.response
                if rsp[0] == _SC_SERVER_ERR:
                    raise SearpcError("Error received: %s %s (In Read More)" % (rsp[0], rsp[1]))
                buf += rsp[2]
            return buf
        elif rsp[0] == _SC_PROC_DEAD:
            raise DeadProcError()
        else:
            raise SearpcError("Error received: %s %s" % (rsp[0], rsp[1]))

    def call_remote_func_sync(self, fcall_str):
        """Call remote function `fcall_str` and wait response."""

        retried = 0
        while True:
            try:
                client = self.pool.get_client()
                if self.req_pool:
                    req_id = client.req_ids.get(self.service_name, -1)
                    if req_id == -1:
                        req_id = self._start_service(client)
                        client.req_ids[self.service_name] = req_id
                    try:
                        ret = self._real_call(client, req_id, fcall_str)
                    except DeadProcError:
                        client.req_ids[self.service_name] = -1
                        self.pool.return_client(client)
                        if retried < self.retry_num:
                            retried = retried + 1
                            continue
                        else:
                            raise

                    self.pool.return_client(client)
                    return ret
                else:
                    # no req pool
                    req_id = self._start_service(client)
                    ret = self._real_call(client, req_id, fcall_str)
                    client.send_update(req_id, "103", "service is done", "", 0)
                    self.pool.return_client(client)
                    return ret
            except (NetworkError, SearpcError):
                # the client is not return to the pool and is freed automatically
                if retried < self.retry_num:
                    retried = retried + 1
                    continue
                else:
                    raise


class CcnetRpcClient(RpcClientBase):

    def __init__(self, ccnet_client_pool, retry_num=1, *args, **kwargs):
        RpcClientBase.__init__(self, ccnet_client_pool, "ccnet-rpcserver",
                               *args, **kwargs)

    @searpc_func("string", [])
    def list_peers(self):
        pass

    @searpc_func("objlist", [])
    def list_resolving_peers(self):
        pass

    @searpc_func("objlist", ["string"])
    def get_peers_by_role(self):
        pass
    
    @searpc_func("object", ["string"])
    def get_peer(self):
        pass
    

#    @searpc_func("string", [])
#    def list_groups(self):
#        pass

#    @searpc_func("object", ["string"], ret_obj_class=Group)
#    def get_group(self):
#        pass

    @searpc_func("object", [])
    def get_session_info(self):
        pass

    @searpc_func("int", ["string"])
    def add_client(self):
        pass

    @searpc_func("int", ["string", "string"])
    def add_role(self, peer_id, role):
        pass
    
    @searpc_func("int", ["string", "string"])
    def remove_role(self, peer_id, role):
        pass

    @searpc_func("objlist", ["int", "int"])
    def get_procs_alive(offset, limit):
        pass

    @searpc_func("int", [])
    def count_procs_alive():
        pass

    @searpc_func("objlist", ["int", "int"])
    def get_procs_dead(offset, limit):
        pass

    @searpc_func("int", [])
    def count_procs_dead():
        pass
    
    @searpc_func("string", ["string"])
    def get_config(self, key):
        pass

    @searpc_func("int", ["string", "string"])
    def set_config(self, key, value):
        pass        

    @searpc_func("int", ["string", "string", "string"])
    def login_relay(self, relay_id, email, passwd):
        pass
        
    @searpc_func("int", ["string"])
    def logout_relay(self, relay_id):
        pass


class CcnetThreadedRpcClient(RpcClientBase):

    def __init__(self, ccnet_client_pool, retry_num=1, *args, **kwargs):
        RpcClientBase.__init__(self, ccnet_client_pool, "ccnet-threaded-rpcserver",
                               *args, **kwargs)

    @searpc_func("int", ["string", "string", "int", "int"])
    def add_emailuser(self, email, passwd, is_staff, is_active):
        pass
    
    @searpc_func("int", ["string"])
    def remove_emailuser(self, email):
        pass
    
    @searpc_func("int", ["string", "string"])
    def validate_emailuser(self, email, passwd):
        pass

    @searpc_func("object", ["string"])
    def get_emailuser(self, email):
        pass

    @searpc_func("object", ["int"])
    def get_emailuser_by_id(self, user_id):
        pass

    @searpc_func("objlist", ["int", "int"])
    def get_emailusers(self, start, limit):
        pass
    
    @searpc_func("int", ["int", "string", "int", "int"])
    def update_emailuser(self, user_id, password, is_staff, is_active):
        pass

    @searpc_func("int", ["string", "string"])
    def add_binding(self, email, peer_id):
        pass
    
    @searpc_func("string", ["string"])
    def get_binding_email(self, peer_id):
        pass
    
    @searpc_func("string", ["string"])
    def get_binding_peerids(self, email):
        pass

    @searpc_func("int", ["string"])
    def remove_binding(self, email):
        pass

    @searpc_func("int", ["string", "string"])
    def remove_one_binding(self, email, peer_id):
        pass
    
    @searpc_func("objlist", ["string"])
    def get_peers_by_email(self, email):
        pass

    @searpc_func("int", ["string", "string"])
    def create_group(self, group_name, user_name):
        pass

    @searpc_func("int", ["int", "string", "string"])
    def create_org_group(self, org_id, group_name, user_name):
        pass
    
    @searpc_func("int", ["int", "string"])
    def remove_group(self, group_id, user_name):
        pass

    @searpc_func("int", ["int", "string", "string"])
    def group_add_member(self, group_id, user_name, member_name):
        pass
    
    @searpc_func("int", ["int", "string", "string"])
    def group_remove_member(self, group_id, user_name, member_name):
        pass
    
    @searpc_func("int", ["int", "string"])
    def quit_group(self, group_id, user_name):
        pass

    @searpc_func("objlist", ["string"])
    def get_groups(self, user_name):
        pass

    @searpc_func("objlist", ["int", "int"])
    def get_all_groups(self, start, limit):
        pass
    
    @searpc_func("object", ["int"])
    def get_group(self, group_id):
        pass

    @searpc_func("objlist", ["int"])
    def get_group_members(self, group_id):
        pass

    @searpc_func("int", ["int", "string"])
    def check_group_staff(self, group_id, username):
        pass

    @searpc_func("int", ["string"])
    def remove_group_user(self, username):
        pass
    
    @searpc_func("int", ["int", "string"])
    def is_group_user(self, group_id, user):
        pass
        
    @searpc_func("int", ["int", "string", "string"])
    def group_share_repo(self, group_id, user_name, repo_id):
        pass

    @searpc_func("int", ["int", "string", "string"])
    def group_unshare_repo(self, group_id, user_name, repo_id):
        pass

    @searpc_func("string", ["int"])
    def get_group_repoids(self, group_id):
        pass

    @searpc_func("objlist", ["string"])
    def get_group_my_share_repos(self, username):
        pass

    @searpc_func("objlist", ["string"])
    def get_group_user_share_repos(self, username):
        pass
    
    @searpc_func("string", ["string"])
    def get_group_repo_share_from(self, repo_id):
        pass

    @searpc_func("int", ["string", "string", "string"])
    def create_org(self, org_name, url_prefix, creator):
        pass

    @searpc_func("int", ["int"])
    def remove_org(self, org_id):
        pass
    
    @searpc_func("objlist", ["int", "int"])
    def get_all_orgs(self, start, limit):
        pass

    @searpc_func("object", ["string"])
    def get_org_by_url_prefix(self, url_prefix):
        pass

    @searpc_func("object", ["string"])
    def get_org_by_id(self, org_id):
        pass
    
    @searpc_func("int", ["int", "string", "int"])
    def add_org_user(self, org_id, email, is_staff):
        pass

    @searpc_func("int", ["int", "string"])
    def remove_org_user(self, org_id, email):
        pass
    
    @searpc_func("objlist", ["string"])
    def get_orgs_by_user(self, email):
        pass
    
    @searpc_func("objlist", ["string", "int", "int"])
    def get_org_emailusers(self, url_prefix, start, limit):
        pass

    @searpc_func("int", ["int", "int"])
    def add_org_group(self, org_id, group_id):
        pass

    @searpc_func("int", ["int", "int"])
    def remove_org_group(self, org_id, group_id):
        pass

    @searpc_func("int", ["int"])
    def is_org_group(self, group_id):
        pass

    @searpc_func("int", ["int"])
    def get_org_id_by_group(self, group_id):
        pass
    
    @searpc_func("objlist", ["int", "int", "int"])
    def get_org_groups(self, org_id, start, limit):
        pass
    
    @searpc_func("int", ["int", "string"])
    def org_user_exists(self, org_id, email):
        pass

    @searpc_func("int", ["int", "string"])
    def is_org_staff(self, org_id, user):
        pass
    
    
