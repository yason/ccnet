
import cclient
from rpc import NetworkError
import Queue

class Client(cclient.Client):
    """ccnet.Client is a _builtin_ type. We need wrap it with a class if we
    want to write code like client.req_ids = {}.

    """
    pass

class ClientPool(object):
    """ccnet client pool."""
    
    def __init__(self, conf_dir, pool_size=5):
        """
        :param conf_dir: the ccnet configuration directory
        :param pool_size:
        """
        self.conf_dir = conf_dir
        self.pool_size = pool_size
        self._pool = Queue.Queue(pool_size)

    def _create_client(self):
        client = Client()
        client.load_confdir(self.conf_dir)
        client.req_ids = {}
        if (client.connect_daemon(cclient.CLIENT_SYNC) < 0):
            raise NetworkError("Can't connect to daemon")
        return client

    def get_client(self):
        try:
            client = self._pool.get(False)
        except:
            client = self._create_client()
        return client

    def return_client(self, client):
        self._pool.put(client, False)
