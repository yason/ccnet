from gevent import monkey
monkey.patch_all()

import os
import logging

from pysearpc import searpc_server
from ccnet import AsyncClient, RpcServerProc

RPC_SERVICE_NAME = 'test-rpcserver'
CCNET_CONF_DIR = os.path.expanduser('~/.ccnet')

def init_logging():
    """Configure logging module"""
    level = logging.DEBUG

    kw = {
        'format': '[%(asctime)s] %(message)s',
        'datefmt': '%m/%d/%Y %H:%M:%S',
        'level': level,
    }

    logging.basicConfig(**kw)

def register_rpc_functions(session):
    def str_mul(a, b):
        return a * b

    searpc_server.create_service(RPC_SERVICE_NAME)
    searpc_server.register_function(RPC_SERVICE_NAME, str_mul)

    session.register_service(RPC_SERVICE_NAME, 'basic', RpcServerProc)

def main():
    init_logging()
    session = AsyncClient(CCNET_CONF_DIR)
    session.connect_daemon()
    register_rpc_functions(session)
    session.main_loop()

if __name__ == '__main__':    
    main()