import time
from socket import *
import sys

IP = sys.argv[1]
PORT = 5005
BUFF_SIZE = 1024

try:
        print("create socket for ip : ", IP)
        s = socket(AF_INET, SOCK_STREAM)
        s.setsockopt(SOL_SOCKET, SO_REUSEADDR, -1)
        s.bind((IP, PORT))
        s.listen(1)
        print("listen on port", PORT)
        conn, addr = s.accept()
        print(addr, "connected!")

        while True:
                data = conn.recv(BUFF_SIZE)
                if data:
                        print('[',int(round(time.time() * 1000)),']','data: ', data)#time in milliseconds
except KeyboardInterrupt:
        print('\n Good Bye...')
        s.close()
        sys.exit(0)