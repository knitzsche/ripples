#!/bin/bash
id | grep uid=0 -q
if [[ "$?" == "1" ]]; then
  echo "You must use sudo to run this app."
  exit 1
fi;
exec "$@"
