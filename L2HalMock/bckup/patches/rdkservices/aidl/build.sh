#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BINDER_TOOLS_DIR="${SCRIPT_DIR}/build-tools/linux_binder_idl"
AIDL_BIN="${BINDER_TOOLS_DIR}/local/bin/aidl"

# Default values
AIDL_TARGET="${1:-hdmicec}"
AIDL_SRC_VERSION="${2:-current}"

usage() {
    echo "Usage: $0 [AIDL_TARGET] [AIDL_SRC_VERSION]"
    echo ""
    echo "Arguments:"
    echo "  AIDL_TARGET       Module to build (default: hdmicec)"
    echo "                    Options: audiodecoder, audiosink, avbuffer, avclock, boot,"
    echo "                             broadcast, cdm, deepsleep, deviceinfo, flash,"
    echo "                             hdmicec, hdmiinput, hdmioutput, indicator, panel,"
    echo "                             planecontrol, sensor, videodecoder, videosink"
    echo "  AIDL_SRC_VERSION  Version to build (default: current)"
    echo ""
    echo "Example:"
    echo "  $0 hdmicec current"
    exit 1
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
fi

echo "=== Building RDK HAL AIDL Interface ==="
echo "Target: ${AIDL_TARGET}"
echo "Version: ${AIDL_SRC_VERSION}"

# Step 1: Install binder tools if not present
if [[ ! -f "${AIDL_BIN}" ]]; then
    echo ""
    echo "=== Installing Binder Tools ==="
    
    if [[ ! -d "${BINDER_TOOLS_DIR}" ]]; then
        mkdir -p "${SCRIPT_DIR}/build-tools"
        git clone https://github.com/rdkcentral/linux_binder_idl "${BINDER_TOOLS_DIR}"
    fi
    
    echo "Building AIDL generator tool..."
    cd "${BINDER_TOOLS_DIR}"
    ./build-aidl-generator-tool.sh
    cd "${SCRIPT_DIR}"
fi

if [[ ! -f "${AIDL_BIN}" ]]; then
    echo "ERROR: AIDL binary not found at ${AIDL_BIN}"
    exit 1
fi

echo ""
echo "=== Configuring CMake ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
    -DAIDL_TARGET="${AIDL_TARGET}" \
    -DAIDL_SRC_VERSION="${AIDL_SRC_VERSION}" \
    -DAIDL_BIN="${AIDL_BIN}"

echo ""
echo "=== Building ==="
cmake --build .

echo ""
echo "=== Build Complete ==="
echo "Generated files: ${SCRIPT_DIR}/gen/${AIDL_TARGET}/${AIDL_SRC_VERSION}/"
