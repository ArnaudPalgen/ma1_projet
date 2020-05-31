import time
from socket import *
import sys
import threading

BUFF_SIZE = 1024

class Client(threading.Thread):
    def __init__(self, addr, sock):
        threading.Thread.__init__(self)
        self.sock = sock
        print ("new connection: ", addr)
    def run(self):
        while True:
            data = self.sock.recv(BUFF_SIZE)
            if data:
                print('[',int(round(time.time() * 1000)),']','data: ', data)#time in milliseconds
                #while True:
                self.sock.send(data)
                #time.sleep(1)#sleep 1s

if __name__ == "__main__":
    if(len(sys.argv) < 3):
        print("bad parameters")
        exit(0)
    ip = sys.argv[1]
    port = int(sys.argv[2])

    server = socket(AF_INET, SOCK_STREAM)
    server.setsockopt(SOL_SOCKET, SO_REUSEADDR, -1)
    server.bind((ip, port))
    print("server at "+ip+" started")
    print("listen on port", port)
    while True:
        server.listen(1)
        sock, addr = server.accept()
        newThread = Client(addr, sock)
        newThread.start()