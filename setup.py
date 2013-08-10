import glob
import time

from distutils.core import setup, Extension

setup(
    name='icfpc2013',
    version=str(int(time.time())),
    packages=['frontend', 'sandbox', 'solver'],
    install_requires=[
        'python-gflags',
        'requests',
        ])
