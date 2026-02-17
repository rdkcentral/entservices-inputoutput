#!/bin/bash

# Define ANSI color codes
GREEN='\033[0;32m'     # Green text
YELLOW='\033[1;33m'    # Yellow text
NC='\033[0m'           # No color (reset)

set -x
# Set up directories
SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=$(dirname "$SCRIPT")
RDK_DIR="$SCRIPTS_DIR/../"
WORKSPACE="$SCRIPTS_DIR/workspace"

# rm -rf "$WORKSPACE"
# mkdir "$WORKSPACE"

echo -e "${GREEN}========================================Third Party===============================================${NC}"
cd $WORKSPACE
mkdir -p deps/third-party/

cd $WORKSPACE/deps/third-party/
echo -e "${GREEN}========================================flux===============================================${NC}"
git clone git@github.com:deniskropp/flux.git || { echo "❌ Git clone failed."; exit 1; }

echo -e "${GREEN}========================================directfb===============================================${NC}"
git clone git@github.com:Distrotech/DirectFB || { echo "❌ Git clone failed."; exit 1; }

echo -e "${GREEN}========================================log4c===============================================${NC}"
wget http://downloads.sourceforge.net/project/log4c/log4c/1.2.4/log4c-1.2.4.tar.gz && tar -xzvf log4c-1.2.4.tar.gz || { echo "❌ Git clone failed."; exit 1; }

echo -e "${GREEN}========================================safeclib===============================================${NC}"
git clone git@github.com:rurban/safeclib.git || { echo "❌ Git clone failed."; exit 1; }

echo -e "${GREEN}========================================dbus===============================================${NC}"
wget http://dbus.freedesktop.org/releases/dbus/dbus-1.6.8.tar.gz && tar -xzvf dbus-1.6.8.tar.gz || { echo "❌ Git clone failed."; exit 1; }

echo -e "${GREEN}========================================dbus===============================================${NC}"
wget https://download.gnome.org/sources/glib/2.72/glib-2.72.3.tar.xz && tar -xJvf glib-2.72.3.tar.xz || { echo "❌ Git clone failed."; exit 1; }