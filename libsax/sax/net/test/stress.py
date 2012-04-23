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
done = False

def thread_read(client):
    try:
        global count
        while(True):
            client.recv_test()
            count += 1
    except Thrift.TException, tx:
        print "%s" % (tx.message)

def thread_run():
    try:
        global count
        global done
        host='192.168.203.110'
        port=55678

        socket = TSocket.TSocket(host, port)
        transport = TTransport.TFramedTransport(socket)
        protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)
        client = Client(protocol)
        transport.open()

        thread.start_new_thread(thread_read, (client,))

        holdon = 0
        while(True):
            if done:
                break
            if count > 0 or holdon < 3:
                for i in range(4000) :
                    req = TestReq(req='hello')
                    res = client.send_test(req)
                time.sleep(0.1)
                #if count == 0:
                #    holdon += 1
            #else:
            #    done = True
            #print res

        while True:
            time.sleep(10)

        #transport.close()

    except Thrift.TException, tx:
        print "%s" % (tx.message)

if __name__ == "__main__":
    try:
        thread_num = 1
        task_per_thread = 1
        
        time.clock()
        for i in xrange(thread_num):
            thread.start_new_thread(thread_run,())
            
        lastTime = 0
        while(True):
            if done:
                time.sleep(10)
            else:
                time.sleep(1)
                print "QPS : %f" %(count / (time.time()-lastTime))
                count = 0
                lastTime = time.time()
            
    except Thrift.TException, tx:
        print "%s" % (tx.message)
