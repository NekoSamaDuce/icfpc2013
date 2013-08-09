#!/bin/bash

set -e

root_dir="$(dirname "$0")"
stage_dir="$root_dir/stage"

if [[ ! -d "$stage_dir" ]]; then
  virtualenv "$stage_dir"
fi

"$stage_dir/bin/pip" install -e "$root_dir"
