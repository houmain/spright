#!/bin/bash

resource() {
  id=$1
  path=$2
  if [ "$path" -nt "${id}.inc" ]; then
    hexdump -v -e '1/1 "0x%x," ""' "$path" > "${id}.inc"
  fi
}

resource Items.png Items.png
