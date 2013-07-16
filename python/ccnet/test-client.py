import sys
import os
from ccnet import ClientPool, RpcClientBase
from ccnet.async import AsyncTask, ThreadPool
from pysearpc import searpc_func
import threading
import libevent

RPC_SERVICE_NAME = 'test-rpcserver'
CCNET_CONF_DIR = os.path.expanduser('~/.ccnet')

class TestRpcClient(RpcClientBase):
    def __init__(self, client_pool, *args, **kwargs):
        RpcClientBase.__init__(self, client_pool, RPC_SERVICE_NAME, *args, **kwargs)

    @searpc_func('string', ['string', 'int'])
    def str_mul(self, s, n):
        pass


class Worker(threading.Thread):        
    def __init__(self, rpc):
        threading.Thread.__init__(self)
        self.rpc = rpc
        
    def run(self):
        s = 'abcdef'
        n = 100
        assert self.rpc.str_mul(s, n) == s * n

class TestTask(AsyncTask):
    def __init__(self, ev_base, rpcclient, s, n):
        AsyncTask.__init__(self, ev_base)
        self.rpcclient = rpcclient
        self.s = s
        self.n = n

    def do_in_background(self):
        return self.rpcclient.str_mul(self.s, self.n)

    def on_post_execute(self, result):
        assert len(result) == len(self.s) * self.n
        

def test(n):
    event_base = libevent.Base()
    rpcclient = TestRpcClient(ClientPool(CCNET_CONF_DIR))
    s = 'hello'
    m = 100

    threadpool = ThreadPool()
    threadpool.start()
    for i in xrange(n):
        task = TestTask(event_base, rpcclient, s, m)
        task.execute(threadpool)

    event_base.loop()
    threadpool.set_exit()
    threadpool.joinall()

def main():
    if len(sys.argv) > 1:
        test(int(sys.argv[1]))
    else:
        test(100)

    print 'test passed'

if __name__ == '__main__':    
    main()