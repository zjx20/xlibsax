#!/usr/bin/env python

import sys, os
import time
import thread
sys.path=[os.path.abspath(os.path.join(__file__, '..', 'gen-py'))] + sys.path

from test.Test import Client
from test.ttypes import *
from test.constants import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

count = 0

    
if __name__ == "__main__":
    try:
        host='192.168.203.110'
        port=55678

        socket = TSocket.TSocket(host, port)
        transport = TTransport.TFramedTransport(socket)
        protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)
        client = Client(protocol)
        transport.open()

        req = TestReq(req='hello')
        res = client.test(req)
        print res

        transport.close()
       
    except Thrift.TException, tx:
        print "%s" % (tx.message)
