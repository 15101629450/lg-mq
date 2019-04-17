#!/usr/bin/python
#coding=utf-8

from lg_mq_client import lg_mq_client

mq = lg_mq_client()

mq.push('111')
mq.push('222')
mq.push('333')
mq.push('111')
mq.push('222')
mq.push('333')

print mq.total()
print mq.count()
print mq.event()

print mq.pop()
print mq.pop()
print mq.pop()
print mq.pop()

mq.push2('111')
mq.push2('222')
mq.push2('333')

print mq.pop()
print mq.pop()
print mq.pop()
print mq.pop()

print mq.total()
print mq.count()
print mq.event()



