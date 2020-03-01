#!/bin/bash

set -e

# Create $XDG_RUNTIME_DIR if not exists (to be removed when LP: #1738197 is fixed)
[ -n "$XDG_RUNTIME_DIR" ] && mkdir -p $XDG_RUNTIME_DIR -m 700

# Wayland socket management
wdisplay="wayland-0"
if [ -n "$WAYLAND_DISPLAY" ]; then
  wdisplay="$WAYLAND_DISPLAY"
fi
wayland_sockpath="$XDG_RUNTIME_DIR/../$wdisplay"
wayland_snappath="$XDG_RUNTIME_DIR/$wdisplay"

# If running on Classic, a Wayland socket may be in the usual XDG_RUNTIME_DIR
if [ ! -S "$wayland_snappath" ]; then
  # Either running on Core, or no Wayland socket to be found
  if [ ! -S "$wayland_sockpath" ]; then
    echo "Error: Unable to find a valid Wayland socket in $(dirname $XDG_RUNTIME_DIR)"
    echo "Is a Wayland server running?"

    # On Core, Xwayland needs to run as root (bug lp:1767372), so everything has to
    if [ "$EUID" -ne 0 ]; then
      echo "You could try running as root"
    fi
    # It may be that the socket isn't there yet, in case we will get restarted by systemd,
    # wait a couple seconds to give the other side time to prepare
    # For mir-kiosk, https://github.com/MirServer/mir/issues/586 would solve it proper
    sleep 1
  fi

  # if running under wayland, use it
  # create the compat symlink for now
  if [ ! -e "$wayland_snappath" ]; then
    ln -s "$wayland_sockpath" "$wayland_snappath"
  fi
fi

while [ "$(cat /run/user/0/wayland-0 2>&1|sed 's/.*: //')" = "Permission denied" ]; do
    echo "Wayland socket not readable, sleeping for 10 sec"
    echo "Please run snap connect $SNAP_NAME:wayland mir-kiosk:wayland"
    sleep 10
done
export SDL_RENDER_DRIVER=opengles2
$SNAP/bin/ripples $@
