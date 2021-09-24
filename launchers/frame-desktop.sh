#!/bin/sh
set -xe

frame="${1:-ubuntu-frame}"

sudo "${frame}" --help > /dev/null 2>&1 || true
sudo cp "${XAUTHORITY:-~/.Xauthority}" "/root/snap/${frame}/current/.Xauthority"
XAUTHORITY="/root/snap/${frame}/current/.Xauthority" exec sudo "${frame}"
