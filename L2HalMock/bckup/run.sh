#!/bin/bash

#Fetch the Args
GivenPlugins="$1"

echo "Starting Services for Plugins: $GivenPlugins"

echo "Found: $GivenPlugins"


# Define ANSI color codes for green
GREEN='\033[0;32m'     # Green text
NC='\033[0m'           # No color (resets to default)

# Define ANSI color codes for green
GREEN='\033[0;32m'     # Green text
NC='\033[0m'           # No color (resets to default)

echo -e "${GREEN}========================================Clear cache to start Dbus during intitialize===============================================${NC}"
#rm -f /usr/local/var/run/dbus/pid
#rm -f /usr/local/var/run/dbus/system_bus_socket
#rm -rf /usr/local/var/run/dbus/*
#mkdir -p /usr/local/var/run/dbus
#sed -i '/_prefix/d' /etc/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf
#sed -i '/_prefix/d' /usr/share/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf
#service dbus restart
#ls -l /run/dbus/system_bus_socket


echo "===== DBus policy fix ====="
sed -i '/_prefix/d' /etc/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf 2>/dev/null || true
sed -i '/_prefix/d' /usr/share/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf 2>/dev/null || true

echo "===== DBus runtime dirs ====="
mkdir -p /run/dbus /usr/local/var/run/dbus

echo "===== Stop + kill leftovers to prevent multi-daemon ====="
service dbus stop || true
pkill -f "dbus-daemon --system" || true
sleep 1

echo "===== Cleanup stale files ====="
rm -f /run/dbus/pid /run/dbus/system_bus_socket
rm -f /var/run/dbus/pid /var/run/dbus/system_bus_socket
rm -f /usr/local/var/run/dbus/pid /usr/local/var/run/dbus/system_bus_socket

echo "===== Start DBus ONCE ====="
service dbus start

echo "===== Verify /run socket ====="
ls -l /run/dbus/system_bus_socket || { echo "FATAL: no system bus socket"; exit 1; }

echo "===== Create /usr/local compatibility symlink ====="
ln -sf /run/dbus/system_bus_socket /usr/local/var/run/dbus/system_bus_socket
ls -l /usr/local/var/run/dbus/system_bus_socket

echo "===== Verify only one system dbus-daemon ====="
ps -ef | grep dbus-daemon | grep -- --system | grep -v grep

#echo "===== DBus policy fix ====="
#sed -i '/_prefix/d' /etc/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf
#sed -i '/_prefix/d' /usr/share/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf

#echo "===== DBus runtime ====="
#mkdir -p /run/dbus
#rm -f /run/dbus/pid /var/run/dbus/pid /usr/local/var/run/dbus/pid

#echo "===== Restart system DBus (no manual daemon start, no session fallback) ====="
#service dbus restart

#echo "===== Verify system bus socket ====="
#ls -l /run/dbus/system_bus_socket

#echo "===== Provide compatibility symlink (if some components expect /usr/local path) ====="
#mkdir -p /usr/local/var/run/dbus
#rm -f /usr/local/var/run/dbus/system_bus_socket
#ln -sf /run/dbus/system_bus_socket /usr/local/var/run/dbus/system_bus_socket
#ls -l /usr/local/var/run/dbus/system_bus_socket
#``




SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=`dirname "$SCRIPT"`
WORKSPACE=$SCRIPTS_DIR/workspace

# Define the port to check and maximum number of seconds to wait
flask_port=8000
timeout_duration=60

# Function to wait for a port to become available
wait_for_port() {
    local port="$1"
    local timeout="$2"

    for ((i=0; i<timeout; i++)); do
        if nc -z -w 1 localhost "$port"; then
            echo "Port $port is listening. Your process is up and running."
            return 0
        fi

        if [ $i -eq $((timeout - 1)) ]; then
            echo "Timeout: Port $port is not listening after waiting for $timeout seconds."
            return 1
        fi

        sleep 1
    done
}

echo -e "${GREEN}========================================Setup D-Bus===============================================${NC}"
# Copy dbus libs if needed
cp -r /usr/lib/x86_64-linux-gnu/dbus-1.0 /usr/lib/aarch64-linux-gnu/ 2>/dev/null || true

# Create messagebus user if it doesn't exist
if ! id -u messagebus &>/dev/null; then
    useradd -r -s /usr/sbin/nologin messagebus 2>/dev/null || true
fi

# Backup original dbus configs (only if not already backed up)
if [ -f /usr/share/dbus-1/system.conf ] && [ ! -f /usr/share/dbus-1/system.conf_org_1 ]; then
    mv /usr/share/dbus-1/system.conf /usr/share/dbus-1/system.conf_org_1
fi
if [ -f /usr/share/dbus-1/session.conf ] && [ ! -f /usr/share/dbus-1/session.conf_org_1 ]; then
    mv /usr/share/dbus-1/session.conf /usr/share/dbus-1/session.conf_org_1
fi

# Copy custom dbus configs
DBUS_CONF_DIR=$SCRIPTS_DIR/dbus
cp $DBUS_CONF_DIR/system.conf /usr/share/dbus-1/system.conf
cp $DBUS_CONF_DIR/session.conf /usr/share/dbus-1/session.conf

# Create dbus run directories with proper permissions
mkdir -p /run/dbus
mkdir -p /var/run/dbus
mkdir -p /usr/local/var/run/dbus
chown messagebus:messagebus /run/dbus 2>/dev/null || true
chown messagebus:messagebus /var/run/dbus 2>/dev/null || true
chmod 755 /run/dbus /var/run/dbus

# Stop existing dbus
killall -9 dbus-daemon 2>/dev/null || true
rm -f /var/run/dbus/pid /run/dbus/pid 2>/dev/null || true
rm -f /run/dbus/system_bus_socket /var/run/dbus/system_bus_socket 2>/dev/null || true

# Generate dbus machine-id if missing
if [ ! -f /var/lib/dbus/machine-id ] && [ ! -f /etc/machine-id ]; then
    mkdir -p /var/lib/dbus
    dbus-uuidgen > /var/lib/dbus/machine-id 2>/dev/null || true
fi

# Start dbus daemon with explicit config
#echo "Starting D-Bus daemon..."
#dbus-daemon --system --fork --print-address 2>&1 || {
#    echo "Trying alternative D-Bus start..."
    # Try running as root if messagebus user fails
#    dbus-daemon --system --fork --address=unix:path=/run/dbus/system_bus_socket 2>&1 || true
#}

# Wait for socket to be created
sleep 1

# Create symlink for IARM
if [ -S /run/dbus/system_bus_socket ]; then
    ln -sf /run/dbus/system_bus_socket /usr/local/var/run/dbus/system_bus_socket
elif [ -S /var/run/dbus/system_bus_socket ]; then
    ln -sf /var/run/dbus/system_bus_socket /usr/local/var/run/dbus/system_bus_socket
fi

# Verify dbus socket exists
#echo -e "${GREEN}========================================Verify D-Bus===============================================${NC}"
#if [ -S /run/dbus/system_bus_socket ] || [ -S /var/run/dbus/system_bus_socket ] || [ -S /usr/local/var/run/dbus/system_bus_socket ]; then
#    echo "D-Bus socket found"
#    ls -l /run/dbus/system_bus_socket 2>/dev/null || ls -l /var/run/dbus/system_bus_socket 2>/dev/null
#    ls -l /usr/local/var/run/dbus/system_bus_socket 2>/dev/null || true
#else
#    echo "ERROR: D-Bus socket not found! Attempting manual socket creation..."
#    # Last resort: try to start dbus without config restrictions
#    dbus-daemon --session --fork --address=unix:path=/usr/local/var/run/dbus/system_bus_socket 2>&1 || true
#    sleep 1
#    if [ -S /usr/local/var/run/dbus/system_bus_socket ]; then
#        echo "D-Bus session socket created as fallback"
#    else
#        echo "FATAL: Could not create D-Bus socket. IARM will fail."
#    fi
#fi

echo -e "${GREEN}========================================Verify D-Bus===============================================${NC}"

echo -e "${GREEN}========================================Setup & Verify D-Bus===============================================${NC}"

echo "===== DBus policy fix (remove _prefix) ====="
sed -i '/_prefix/d' /etc/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf 2>/dev/null || true
sed -i '/_prefix/d' /usr/share/dbus-1/system.d/com.comcast.rdk.iarm.bus.conf 2>/dev/null || true

echo "===== Ensure runtime directories exist ====="
mkdir -p /run/dbus /var/run/dbus /usr/local/var/run/dbus

# Optional permissions (safe). If messagebus user doesn't exist, these will be ignored.
chown messagebus:messagebus /run/dbus /var/run/dbus 2>/dev/null || true
chmod 755 /run/dbus /var/run/dbus 2>/dev/null || true

echo "===== Cleanup stale pid/socket files (NO process kill here) ====="
rm -f /run/dbus/pid /run/dbus/system_bus_socket 2>/dev/null || true
rm -f /var/run/dbus/pid /var/run/dbus/system_bus_socket 2>/dev/null || true
rm -f /usr/local/var/run/dbus/pid /usr/local/var/run/dbus/system_bus_socket 2>/dev/null || true

echo "===== Start DBus via service (single start) ====="
# Use restart if you prefer; start is safer after you kill old processes yourself.
service dbus start >/dev/null 2>&1 || service dbus restart >/dev/null 2>&1 || {
    echo "FATAL: Could not start dbus service"
    exit 1
}

echo "===== Wait for system bus socket ====="
# Wait up to 5 seconds (50 × 0.1s)
for i in $(seq 1 50); do
    if [ -S /run/dbus/system_bus_socket ] || [ -S /var/run/dbus/system_bus_socket ]; then
        break
    fi
    sleep 0.1
done

echo "===== Verify system bus socket presence ====="
if [ -S /run/dbus/system_bus_socket ] || [ -S /var/run/dbus/system_bus_socket ]; then
    echo "D-Bus socket found"
    ls -l /run/dbus/system_bus_socket 2>/dev/null || ls -l /var/run/dbus/system_bus_socket 2>/dev/null
else
    echo "FATAL: D-Bus socket not found after waiting"
    echo "---- dbus-daemon process check ----"
    ps -ef | grep dbus-daemon | grep -- --system | grep -v grep || echo "No system dbus-daemon running"
    echo "---- directory listing ----"
    ls -ld /run/dbus /var/run/dbus /usr/local/var/run/dbus 2>/dev/null || true
    ls -l  /run/dbus /var/run/dbus /usr/local/var/run/dbus 2>/dev/null || true
    exit 1
fi

echo "===== Create /usr/local compatibility symlink (MUST NOT be a real socket) ====="
mkdir -p /usr/local/var/run/dbus
rm -f /usr/local/var/run/dbus/system_bus_socket 2>/dev/null || true
ln -sf /run/dbus/system_bus_socket /usr/local/var/run/dbus/system_bus_socket
ls -l /usr/local/var/run/dbus/system_bus_socket 2>/dev/null || true

echo "===== Verify DBus is reachable (dbus-send) ====="
if dbus-send --system --print-reply \
    --dest=org.freedesktop.DBus \
    /org/freedesktop/DBus \
    org.freedesktop.DBus.ListNames >/dev/null 2>&1; then
    echo "D-Bus is reachable (dbus-send OK)"
else
    echo "FATAL: D-Bus socket exists but dbus-send failed"
    echo "---- dbus-daemon process check ----"
    ps -ef | grep dbus-daemon | grep -- --system | grep -v grep || true
    exit 1
fi

echo -e "${GREEN}========================================D-Bus Verified Successfully========================================${NC}"
echo -e "${GREEN}========================================Stop all existing services===============================================${NC}"
#Stop flask
fuser -k ${flask_port}/tcp
killall -9 python3
killall -9 IARMDaemonMain
killall -9 CecDaemonMain
killall -9 WPEFramework
killall -9 dsSrvMain
killall -9 pwrSrvMain

pkill python3
pkill IARMDaemonMain
pkill CecDaemonMain
pkill WPEFramework
pkill dsSrvMain
pkill pwrSrvMain

echo -e "${GREEN}========================================flask dependency===============================================${NC}"
if grep -q "HdmiCecSource" <<< "$GivenPlugins"; then
    echo "Found: HdmiCecSource"
    cd $WORKSPACE/deps/rdk/flask/Flask/
    python3 startup.py &
    wait_for_port "$flask_port" "$timeout_duration"
elif grep -q "HdmiCecSink" <<< "$GivenPlugins"; then
    echo "Found: HdmiCecSink"
    cd $WORKSPACE/deps/rdk/flask/Flask/
    python3 startup.py &
    wait_for_port "$flask_port" "$timeout_duration"
fi
echo -e "${GREEN}========================================Continuing with the rest of the script===============================================${NC}"

echo -e "${GREEN}========================================Run iarmbus===============================================${NC}"
cd $WORKSPACE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
deps/rdk/iarmbus/install/bin/IARMDaemonMain &

unset LD_LIBRARY_PATH

echo -e "${GREEN}========================================Run cec deamon===============================================${NC}"
cd $WORKSPACE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/:$WORKSPACE/deps/rdk/hdmicec/install/lib:$WORKSPACE/deps/rdk/hdmicec/ccec/drivers/test:$WORKSPACE/deps/rdk/iarmbus/install/
deps/rdk/hdmicec/install/bin/CecDaemonMain &

echo -e "${GREEN}========================================Run dsSrvMain===============================================${NC}"
cd $WORKSPACE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/:$WORKSPACE/deps/rdk/iarmbus/install:$WORKSPACE/deps/rdk/devicesettings/install/lib
$WORKSPACE/deps/rdk/iarmmgrs/dsmgr/dsSrvMain &

echo -e "${GREEN}========================================Fix HdmiCecSource config===============================================${NC}"
# Fix HdmiCecSource plugin config: remove Platform precondition and set mode to Off (in-process)
cat > $WORKSPACE/install/etc/WPEFramework/plugins/HdmiCecSource.json << 'EOFCONFIG'
{
 "locator":"libWPEFrameworkHdmiCecSource.so",
 "classname":"HdmiCecSource",
 "precondition":[
 ],
 "callsign":"org.rdk.HdmiCecSource",
 "autostart":true,
 "configuration":{
   "root":{
     "mode":"Off",
     "locator":"libWPEFrameworkHdmiCecSourceImplementation.so"
   }
 }
}
EOFCONFIG

# Copy libsafec if not in install path
if [ ! -f "$WORKSPACE/install/usr/lib/libsafec.so.3" ]; then
    cp $WORKSPACE/deps/third-party/safeclib/src/.libs/libsafec.so.3* $WORKSPACE/install/usr/lib/ 2>/dev/null || true
    ln -sf libsafec.so.3.0.9 $WORKSPACE/install/usr/lib/libsafec.so.3 2>/dev/null || true
    ln -sf libsafec.so.3 $WORKSPACE/install/usr/lib/libsafec.so 2>/dev/null || true
fi

echo -e "${GREEN}========================================Run rdkservices===============================================${NC}"
cd $WORKSPACE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/install/usr/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/iarmbus/install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/hdmicec/install/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/hdmicec/ccec/drivers/test
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/hdmicec/rdk-aidl-vcomponent-cec-develop/build/usr/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/devicesettings/install/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKSPACE/deps/rdk/aidl/rdk-halif-aidl/build-tools/linux_binder_idl/local/lib
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
$WORKSPACE/install/usr/bin/WPEFramework -f -c $WORKSPACE/install/etc/WPEFramework/config.json &
