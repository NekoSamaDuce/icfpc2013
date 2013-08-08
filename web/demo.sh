#!/bin/bash

cd "$(dirname "$0")"

set -ex

./deploy.sh
stage/bin/python -m app.main
