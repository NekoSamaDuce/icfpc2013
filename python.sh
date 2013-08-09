#!/bin/bash

set -e

root_dir="$(dirname "$0")"
stage_dir="$root_dir/stage"

exec "$stage_dir/bin/python" "$@"
