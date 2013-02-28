import os
from ccnet import ClientPool, RpcClientBase
from pysearpc import searpc_func

RPC_SERVICE_NAME = 'test-rpcserver'
CCNET_CONF_DIR = os.path.expanduser('~/.ccnet')

class TestRpcClient(RpcClientBase):
    def __init__(self, client_pool, *args, **kwargs):
        RpcClientBase.__init__(self, client_pool, RPC_SERVICE_NAME, *args, **kwargs)

    @searpc_func('string', ['string', 'int'])
    def str_mul(self, s, n):
        pass

def main():
    rpcclient = TestRpcClient(ClientPool(CCNET_CONF_DIR))
    s = 'abcdef'
    n = 100
    try:
        assert rpcclient.str_mul(s, n) == s * n
    except AssertionError:
        print 'test failed'
    else:
        print 'test passed'

if __name__ == '__main__':    
    main()