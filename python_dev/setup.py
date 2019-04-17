#!/usr/bin/python

from distutils.core import setup, Extension
setup(name = 'lg_mq', version = '1.0', ext_modules = [Extension('lg_mq', ['main.c'])])

