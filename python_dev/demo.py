#!/usr/bin/python
#coding=utf-8

import socket
import time

obj = socket.socket()
obj.connect(("127.0.0.1", 6379))

obj.send("$2")
time.sleep(1)
obj.send("$ok")
time.sleep(1)


