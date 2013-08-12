NekoSamaDuce
============

ICFP Programming Contest 2013 - Team "NekoSamaDuce"

Members
-------

- [@dmikurube](http://twitter.com/dmikurube)
- [@kinaba](http://twitter.com/kinaba)
- [@nya3jp](http://twitter.com/nya3jp)
- [@phoenixstarhiro](http://twitter.com/phoenixstarhiro)

Code Summary
------------

- solver/ ... solver backend in C++
- frontend/ ... solver frontend in Python
- util/ ... misc modules in Python

How to Run
----------

Our backend binaries need gflags / glog to compile.

    $ edit util/api.h
    ... fill in API key ...
    $ make -C solver/
    $ ./deploy.sh
    $ ./python.sh -m frontend.kirito --alice_solver=solver/alice --detail_log_dir=postmortem --mode=train --size=15 --operators= --train_count=3
    $ ./python.sh -m frontend.asuna --cardinal_solver=solver/cardinal --detail_log_dir=postmortem --mode=train --size=15 --operators= --train_count=3 --train_exclude_fold

Acknowledgements
----------------

Sword Art Online 13 - Alicization Dividing in stores now.

