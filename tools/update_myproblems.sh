#!/bin/bash

cd "$(dirname "$0")/.."

set -ex

./deploy.sh

./python.sh -m util.list_myproblems > data/myproblems__do_not_try_before_we_get_ready.tsv
