import time

from distutils.core import setup, Extension

module1 = Extension(
    'solver.yui',
    sources=['solver/yui.cc'],
    extra_compile_args=['-std=c++0x'],
    libraries=['glog'])

setup(
    name='icfpc2013',
    version=str(int(time.time())),
    packages=['sandbox'],
    ext_modules=[module1],
    install_requires=[
        'python-gflags',
        'requests',
        ])
