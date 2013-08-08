#!/bin/bash

set -e

cd "$(dirname "$0")"

if [[ ! -d stage ]]; then
  virtualenv stage
fi

stage/bin/pip install -e .
