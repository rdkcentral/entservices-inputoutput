#!/bin/bash

# Define ANSI color codes
GREEN='\033[0;32m'     # Green text
YELLOW='\033[1;33m'    # Yellow text
RED='\033[0;31m'       # Red text
NC='\033[0m'           # No color (reset)

# Error checking function
check_error() {
    local exit_code=$?
    local step_name="$1"
    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}❌ ERROR: $step_name failed with exit code $exit_code${NC}"
        echo -e "${RED}Build stopped. Please fix the error and try again.${NC}"
        return $exit_code
    fi
    return 0
}

# Set up directories
SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=$(dirname "$SCRIPT")
RDK_DIR="$SCRIPTS_DIR/../"
WORKSPACE="$SCRIPTS_DIR/workspace"
INCLUDE_DIR="/usr/include"

# Function to build Thunder Tools
build_thunder_tools() {
    echo -e "${GREEN}========================================Build Thunder Tools===============================================${NC}"
    cd $SCRIPTS_DIR
    
    if [ ! -d "ThunderTools" ]; then
        echo -e "${YELLOW}Cloning ThunderTools...${NC}"
        BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-tools_4.4.bb"
        SRCREV=$(grep "^SRCREV" "$BB_FILE" | cut -d'"' -f2 || true)
        
        if [ -n "$SRCREV" ]; then
            git clone git@github.com:rdkcentral/ThunderTools.git && cd ThunderTools && git checkout R4_4 && git checkout "$SRCREV" || { echo "❌ Git clone failed."; exit 1; }
            cd $SCRIPTS_DIR
        else
            echo -e "${RED}SRCREV not found${NC}"
            exit 1
        fi
    fi
    
    # Apply patches
    # input_file="$BB_FILE"
    # patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-tools/"
    # patches=$(awk '
    #     BEGIN { in_src_uri = 0 }
    #     /SRC_URI/ { in_src_uri = 1 }
    #     in_src_uri {
    #         for (i = 1; i <= NF; i++) {
    #             if ($i ~ /file:\/\/.*\.patch/) {
    #                 match($i, /file:\/\/[^ ]*\.patch/)
    #                 print substr($i, RSTART, RLENGTH)
    #             }
    #         }
    #         if ($0 !~ /\\$/) {
    #             in_src_uri = 0
    #         }
    #     }
    # ' "$input_file")
    
    cd "$SCRIPTS_DIR/ThunderTools"
    echo "$patches" | while read -r patch_uri; do
        patch_file="${patch_uri#file://}"
        full_path="$patch_base_dir/$patch_file"
        if [ -f "$full_path" ]; then
            git apply "$full_path" 2>&1 || true
        fi
    done
    
    cd $SCRIPTS_DIR/ThunderTools/cmake
    mv FindProxyStubGenerator.cmake.in. FindProxyStubGenerator.cmake.in 2>/dev/null || true
    
    if [ ! -d "ThunderTools/build" ]; then
        cmake -G Ninja -S ThunderTools -B ThunderTools/build -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr"
        check_error "CMake configure ThunderTools"
        sudo ninja -C ThunderTools/build install
        check_error "Build ThunderTools"
        sudo chown -R $USER:$USER "$WORKSPACE/install" 2>/dev/null || true
    else
        echo -e "${GREEN}✓ ThunderTools already built${NC}"
    fi
    
    echo -e "${GREEN}✓ ThunderTools built successfully${NC}"
}

# Function to build Thunder
build_thunder() {
    echo -e "${GREEN}========================================Build Thunder===============================================${NC}"
    set -x
    cd $SCRIPTS_DIR
    
    if [ ! -d "Thunder" ]; then
        echo -e "${YELLOW}Cloning Thunder...${NC}"
        BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework_4.4.bb"
        SRCREV_thunder=$(grep "^SRCREV_thunder" "$BB_FILE" | cut -d'"' -f2)
        
        if [ -n "$SRCREV_thunder" ]; then
            git clone git@github.com:rdkcentral/Thunder.git
            cd Thunder && git checkout R4_4 && git checkout "$SRCREV_thunder"
            cd "$SCRIPTS_DIR"
        else
            echo -e "${RED}SRCREV_thunder not found${NC}"
            exit 1
        fi
    fi
    
    # Apply patches
    input_file="$BB_FILE"
    patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework"
    
    cd $SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework/r4.4/
    mv 1004-Add-support-for-project-dir.patch 1004-Add-support-for-project-dir._patch 2>/dev/null || true
    
    # patches=$(awk '
    #     BEGIN { in_src_uri = 0 }
    #     /SRC_URI/ { in_src_uri = 1 }
    #     in_src_uri {
    #         for (i = 1; i <= NF; i++) {
    #             if ($i ~ /file:\/\/.*\.patch/) {
    #                 match($i, /file:\/\/[^ ]*\.patch/)
    #                 print substr($i, RSTART, RLENGTH)
    #             }
    #         }
    #         if ($0 !~ /\\$/) {
    #             in_src_uri = 0
    #         }
    #     }
    # ' "$input_file")
    
    cd "$SCRIPTS_DIR/Thunder"
    echo "$patches" | while read -r patch_uri; do
        patch_file="${patch_uri#file://}"
        full_path="$patch_base_dir/$patch_file"
        if [ -f "$full_path" ]; then
            git apply "$full_path" 2>&1 || true
        fi
    done
    
    if [ ! -d "build" ]; then
        MODULE_NAME="L2HalMock"
        cmake -G Ninja -S . -B build \
            -DMODULE_NAME=ON \
            -DCMAKE_CXX_FLAGS="-DMODULE_NAME=\"${MODULE_NAME}\"" \
            -DBINDING="127.0.0.1" \
            -DCMAKE_BUILD_TYPE="Debug" \
            -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
            -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/" \
            -DPORT="55555" \
            -DTOOLS_SYSROOT="${PWD}" \
            -DCMAKE_CURRENT_SOURCE_DIR="${SCRIPTS_DIR}" \
            -DINITV_SCRIPT=OFF
        check_error "CMake configure Thunder"
        sudo ninja -C build install
        check_error "Build Thunder"
        sudo chown -Rf 777 "$WORKSPACE/install" 2>/dev/null || true
    else
        echo -e "${GREEN}✓ Thunder already built${NC}"
    fi
    
    echo -e "${GREEN}✓ Thunder built successfully${NC}"
    set +x
}

# Function to build ThunderClientLibraries
build_thunder_client_libraries() {
    echo -e "${GREEN}========================================Build ThunderClientLibraries===============================================${NC}"
    cd $SCRIPTS_DIR
    
    if [ ! -d "ThunderClientLibraries" ]; then
        echo -e "${YELLOW}Cloning ThunderClientLibraries...${NC}"
        BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-clientlibraries_4.4.bb"
        SRCREV_wpeframework=$(grep "^SRCREV_wpeframework-clientlibraries" "$BB_FILE" | cut -d'"' -f2)
        
        if [ -n "$SRCREV_wpeframework" ]; then
            git clone git@github.com:rdkcentral/ThunderClientLibraries.git
            cd ThunderClientLibraries && git checkout R4_4 && git checkout "$SRCREV_wpeframework"
            cd "$SCRIPTS_DIR"
        else
            echo -e "${RED}SRCREV_wpeframework not found${NC}"
            exit 1
        fi
    fi
    
    # Apply patches
    input_file="$BB_FILE"
    patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-clientlibraries"
    
    patches=$(awk '
        BEGIN { in_src_uri = 0 }
        /SRC_URI/ { in_src_uri = 1 }
        in_src_uri {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /file:\/\/.*\.patch/) {
                    match($i, /file:\/\/[^ ]*\.patch/)
                    print substr($i, RSTART, RLENGTH)
                }
            }
            if ($0 !~ /\\$/) {
                in_src_uri = 0
            }
        }
    ' "$input_file")
    
    cd "$SCRIPTS_DIR/ThunderClientLibraries"
    echo "$patches" | while read -r patch_uri; do
        patch_file="${patch_uri#file://}"
        full_path="$patch_base_dir/$patch_file"
        if [ -f "$full_path" ]; then
            git apply "$full_path" 2>&1 || true
        fi
    done
    
    mkdir -p $INCLUDE_DIR/interfaces
    cp $SCRIPTS_DIR/entservices-apis/apis/PowerManager/IPowerManager.h $INCLUDE_DIR/interfaces 2>/dev/null || true
    cp $SCRIPTS_DIR/entservices-apis/apis/Ids.h $INCLUDE_DIR/interfaces 2>/dev/null || true
    cp $SCRIPTS_DIR/entservices-apis/apis/Module.h $INCLUDE_DIR/interfaces 2>/dev/null || true
    cp $SCRIPTS_DIR/entservices-apis/apis/entservices_errorcodes.h $INCLUDE_DIR/interfaces 2>/dev/null || true
    
    if [ ! -d "build/ThunderClientLibraries" ]; then
        sudo cmake -G Ninja -S ThunderClientLibraries -B build/ThunderClientLibraries \
            -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
            -DCMAKE_PREFIX_PATH="${WORKSPACE}/install/usr" \
            -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" \
            -DTOOLS_SYSROOT="${PWD}" \
            -DBLUETOOTHAUDIOSINK=OFF \
            -DDEVICEINFO=OFF \
            -DDISPLAYINFO=OFF \
            -DSECURITYAGENT=OFF \
            -DPLAYERINFO=OFF \
            -DPROTOCOLS=OFF \
            -DVIRTUALINPUT=OFF \
            -DCOMPOSITORCLIENT=OFF \
            -DPOWERCONTROLLER=ON 
        check_error "CMake configure ThunderClientLibraries"
        sudo cmake --build build/ThunderClientLibraries --target install
        check_error "Build ThunderClientLibraries"
        sudo chown -R $USER:$USER "$WORKSPACE/install" 2>/dev/null || true
    else
        echo -e "${GREEN}✓ ThunderClientLibraries already built${NC}"
    fi
    
    echo -e "${GREEN}✓ ThunderClientLibraries built successfully${NC}"
}

# Function to build entservices-apis
build_entservices_apis() {
    echo -e "${GREEN}========================================Build entservices-apis===============================================${NC}"
    cd $SCRIPTS_DIR
    
    if [ ! -d "entservices-apis" ]; then
        echo -e "${YELLOW}Cloning entservices-apis...${NC}"
        BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/entservices-apis.bb"
        SRCREV_entservicesapis=$(grep "^SRCREV_entservices-apis" "$BB_FILE" | cut -d'"' -f2)
        
        if [ -n "$SRCREV_entservicesapis" ]; then
            git clone git@github.com:rdkcentral/entservices-apis.git
            cd entservices-apis && git checkout main && git checkout "$SRCREV_entservicesapis"
            cd "$SCRIPTS_DIR"
        else
            echo -e "${RED}SRCREV_entservicesapis not found${NC}"
            exit 1
        fi
    fi
    
    # Apply patches
    input_file="$BB_FILE"
    patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/entservices-apis"
    
    patches=$(awk '
        BEGIN { in_src_uri = 0 }
        /SRC_URI/ { in_src_uri = 1 }
        in_src_uri {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /file:\/\/.*\.patch/) {
                    match($i, /file:\/\/[^ ]*\.patch/)
                    print substr($i, RSTART, RLENGTH)
                }
            }
            if ($0 !~ /\\$/) {
                in_src_uri = 0
            }
        }
    ' "$input_file")
    
    cd "$SCRIPTS_DIR/entservices-apis"
    echo "$patches" | while read -r patch_uri; do
        patch_file="${patch_uri#file://}"
        full_path="$patch_base_dir/$patch_file"
        if [ -f "$full_path" ]; then
            git apply "$full_path" 2>&1 || true
        fi
    done
    
    if [ ! -d "entservices-apis/build" ]; then
        cmake -G Ninja -S entservices-apis -B entservices-apis/build \
            -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
            -DCMAKE_BUILD_TYPE="Debug" \
            -DTOOLS_SYSROOT="${PWD}"
        check_error "CMake configure entservices-apis"
        sudo ninja -C entservices-apis/build install
        check_error "Build entservices-apis"
        sudo chown -R $USER:$USER "$WORKSPACE/install" 2>/dev/null || true
    else
        echo -e "${GREEN}✓ entservices-apis already built${NC}"
    fi
    
    echo -e "${GREEN}✓ entservices-apis built successfully${NC}"
}

# Function to build third-party dependencies
build_third_party() {
    echo -e "${GREEN}========================================Build Third-Party Dependencies===============================================${NC}"
    cd $WORKSPACE
    
    echo -e "${YELLOW}Building flux...${NC}"
    (cd $WORKSPACE/deps/third-party/flux && autoreconf; autoreconf -f -i && ./configure --prefix="${WORKSPACE}/install/usr" && make && make install && cd -)
    check_error "Build flux"
    
    echo -e "${YELLOW}Building directfb...${NC}"
    (cd $WORKSPACE/deps/third-party/DirectFB && autoreconf; autoreconf -f -i && ./configure --prefix="${WORKSPACE}/install/usr" && make && make install && cd -)
    check_error "Build directfb"
    
    echo -e "${YELLOW}Building log4c...${NC}"
    mv $WORKSPACE/deps/third-party/log4c-1.2.4 $WORKSPACE/deps/third-party/log4c 2>/dev/null || true
    (cd $WORKSPACE/deps/third-party/log4c && autoreconf; autoreconf -f -i && ./configure --prefix="${WORKSPACE}/install/usr" && make && make install && cd -)
    check_error "Build log4c"
    
    echo -e "${YELLOW}Building safeclib...${NC}"
    (cd $WORKSPACE/deps/third-party/safeclib && autoreconf; autoreconf -f -i && ./configure --prefix="${WORKSPACE}/install/usr" && make && make install && cd -)
    check_error "Build safeclib"
    
    echo -e "${YELLOW}Building dbus...${NC}"
    mv $WORKSPACE/deps/third-party/dbus-1.6.8 $WORKSPACE/deps/third-party/dbus 2>/dev/null || true
    (cd $WORKSPACE/deps/third-party/dbus && autoreconf; autoreconf -f -i && ./configure --prefix="${WORKSPACE}/install/usr" && make && make install && cd -)
    check_error "Build dbus"
    
    echo -e "${YELLOW}Building glib...${NC}"
    mv $WORKSPACE/deps/third-party/glib-2.72.3 $WORKSPACE/deps/third-party/glib 2>/dev/null || true
    (cd $WORKSPACE/deps/third-party/glib && meson _build && ninja -C _build )
    check_error "Build glib"
    
    echo -e "${GREEN}✓ Third-party dependencies built successfully${NC}"
}

# Function to build IARM
build_iarm() {
    echo -e "${GREEN}========================================Build IARM===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "iarmbus" ]; then
        echo -e "${YELLOW}Cloning iarmbus...${NC}"
        INC_FILE=$SCRIPTS_DIR/meta-rdk-video/recipes-extended/iarmbus/iarmbus_git.bb
        SRCREV_iarmbus=$(grep "^SRCREV_iarmbus" "$INC_FILE" | cut -d'"' -f2 || true)
        
        if [ -n "$SRCREV_iarmbus" ]; then
            git clone git@github.com:rdkcentral/iarmbus.git && cd iarmbus && git checkout "$SRCREV_iarmbus" || { echo "❌ Git clone failed."; exit 1; }
            cd $WORKSPACE/deps/rdk/
        else
            echo -e "${RED}SRCREV_iarmbus not found${NC}"
            exit 1
        fi
    fi
    
    cp $SCRIPTS_DIR/patches/rdkservices/iarmbus/build.sh $WORKSPACE/deps/rdk/iarmbus/
    sed -i "s|source \$PWD/../env.sh|source $SCRIPTS_DIR/env.sh|g" $WORKSPACE/deps/rdk/iarmbus/build.sh
    
    export GLIB_LIBRARY_PATH=/usr/local/lib
    export GLIB_CONFIG_INCLUDE_PATH=/usr/local/lib/glib-2.0
    export GLIBS='-lglib-2.0 -lz'
    
    (cd $WORKSPACE/deps/rdk/iarmbus/ && sudo -E ./build.sh)
    check_error "Build IARM"
    echo -e "${GREEN}✓ IARM built successfully${NC}"
}

# Function to build DeviceSettings
build_devicesettings() {
    echo -e "${GREEN}========================================Build DeviceSettings===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "devicesettings" ]; then
        echo -e "${YELLOW}Cloning DeviceSettings...${NC}"
        INC_FILE=$SCRIPTS_DIR/meta-rdk-video/recipes-extended/devicesettings/devicesettings_git.bb
        SRCREV_devicesettings=$(grep "^SRCREV_devicesettings" "$INC_FILE" | cut -d'"' -f2 || true)
        
        if [ -n "$SRCREV_devicesettings" ]; then
            git clone git@github.com:rdkcentral/devicesettings.git && cd devicesettings && git checkout "$SRCREV_devicesettings" || { echo "❌ Git clone failed."; exit 1; }
            cd $WORKSPACE/deps/rdk/
        fi
    fi
    
    # Clone rdk-halif-device_settings
    if [ ! -d "rdk-halif-device_settings" ]; then
        git clone git@github.com:rdkcentral/rdk-halif-device_settings.git
        cd $WORKSPACE/deps/rdk/rdk-halif-device_settings
        git apply $SCRIPTS_DIR/patches/rdkservices/halif/intErr.patch 2>&1 || true
        sudo cp $WORKSPACE/deps/rdk/rdk-halif-device_settings/include/*.h $INCLUDE_DIR
        cd $WORKSPACE/deps/rdk/
    fi
    
    # Clone DeviceSettings HAL emulator
    cd $WORKSPACE/deps/rdk/devicesettings
    if [ ! -d "hal/src" ]; then
        rm -rf hal
        mkdir -p hal 
        cd hal
        eval "$(ssh-agent -s)" 2>/dev/null || true
        ssh-add ~/.ssh/id_ed25519_rdkcentral 2>/dev/null || true
        ssh-add ~/.ssh/id_ed25519_rdke 2>/dev/null || true
        git clone git@github.com:rdk-e/devicesettings-emulator.git
        mv devicesettings-emulator src
        cd $WORKSPACE/deps/rdk/devicesettings/hal/src
        git apply $SCRIPTS_DIR/patches/rdkservices/devicesettings/dsVideoHal.patch 2>&1 || true
        ( cd $WORKSPACE/deps/rdk/devicesettings/hal/src && sudo make )
    fi
    
    # Build RFC first (dependency)
    if [ ! -d "$WORKSPACE/deps/rdk/rfc" ]; then
        echo -e "${YELLOW}Building RFC (dependency for DeviceSettings)...${NC}"
        build_rfc
    fi
    
    sudo cp $WORKSPACE/deps/rdk/iarmbus/core/include/*.h $INCLUDE_DIR 2>/dev/null || true
    sudo cp $WORKSPACE/deps/rdk/devicesettings/stubs/*.h $INCLUDE_DIR 2>/dev/null || true
    sudo cp $WORKSPACE/deps/rdk/rfc/rfcapi/rfcapi.h $INCLUDE_DIR 2>/dev/null || true
    
    cd $WORKSPACE/deps/rdk/devicesettings/
    git apply $SCRIPTS_DIR/patches/rdkservices/devicesettings/NewDS_Err.patch 2>&1 || true
    
    cp $SCRIPTS_DIR/patches/rdkservices/devicesettings/build.sh $WORKSPACE/deps/rdk/devicesettings/
    chmod +x $WORKSPACE/deps/rdk/devicesettings/build.sh
    
    set +e
    (cd $WORKSPACE/deps/rdk/devicesettings/ && ./build.sh)
    set -e
    
    # Rebuild dshalcli (RPC client library) and libds.so
    echo -e "${YELLOW}Rebuilding dshalcli and libds.so...${NC}"
    
    # Copy header files to ds directory so they can be found during build
    cp $WORKSPACE/deps/rdk/devicesettings/ds/include/*.hpp $WORKSPACE/deps/rdk/devicesettings/ds/ 2>/dev/null || true
    cp $WORKSPACE/deps/rdk/devicesettings/ds/include/*.h $WORKSPACE/deps/rdk/devicesettings/ds/ 2>/dev/null || true
    cp $WORKSPACE/deps/rdk/devicesettings/rpc/include/*.h $WORKSPACE/deps/rdk/devicesettings/ds/ 2>/dev/null || true
    cp $WORKSPACE/deps/rdk/devicesettings/hal/include/*.h $WORKSPACE/deps/rdk/devicesettings/ds/ 2>/dev/null || true
    
    cd $WORKSPACE/deps/rdk/devicesettings/rpc/cli
    make clean && make
    
    # Copy dshalcli to ds/install/lib so libds.so can link against it
    mkdir -p $WORKSPACE/deps/rdk/devicesettings/ds/install/lib
    cp $WORKSPACE/deps/rdk/devicesettings/rpc/cli/install/lib/libdshalcli.so $WORKSPACE/deps/rdk/devicesettings/ds/install/lib/
    
    cd $WORKSPACE/deps/rdk/devicesettings/ds
    make clean && make
    cd $WORKSPACE/deps/rdk/devicesettings
    
    # Copy rebuilt libraries to install directory
    cp $WORKSPACE/deps/rdk/devicesettings/ds/libds.so $WORKSPACE/install/usr/lib/
    cp $WORKSPACE/deps/rdk/devicesettings/rpc/cli/install/lib/libdshalcli.so $WORKSPACE/install/usr/lib/
    
    echo -e "${GREEN}✓ DeviceSettings build completed${NC}"
}

# Function to build RFC
build_rfc() {
    echo -e "${GREEN}========================================Build RFC===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "rfc" ]; then
        INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"
        SRCREV_rfc=$(grep -oP '^SRCREV:pn-rfc\s*=\s*"\K[0-9a-f]+' "$INC_FILE" || true)
        
        if [ -n "$SRCREV_rfc" ]; then
            git clone git@github.com:rdkcentral/rfc.git && cd rfc && git checkout "$SRCREV_rfc" || { echo "❌ Git clone failed."; exit 1; }
        else
            git clone git@github.com:rdkcentral/rfc.git && cd rfc && git checkout "b6da366e704a006394b33759f144c0a4256fd335" || { echo "❌ Git clone failed."; exit 1; }
        fi
        cd $WORKSPACE/deps/rdk/
    fi
    
    cd /usr/include/
    sudo mkdir -p wdmp-c
    sudo cp $WORKSPACE/deps/rdk/devicesettings/stubs/wdmp-c.h $INCLUDE_DIR/wdmp-c 2>/dev/null || true
    sudo cp $WORKSPACE/deps/rdk/rfc/rfcMgr/gtest/mocks/*.h $INCLUDE_DIR 2>/dev/null || true
    
    cd $WORKSPACE/deps/rdk/rfc
    git apply $SCRIPTS_DIR/patches/rdkservices/rfc/rfc.patch 2>&1 || true
    
    set +e
    (cd $WORKSPACE/deps/rdk/rfc && \
        automake --add-missing 2>/dev/null || true; \
        autoreconf -f -i && \
        ./configure && \
        if [ -d "rfcapi" ]; then
            make -C rfcapi || true
            sudo make -C rfcapi install || true
        fi && \
        make || true && \
        sudo make install || true)
    set -e
    
    echo -e "${GREEN}✓ RFC build completed${NC}"
}

# Function to build iarmmgr
build_iarmmgr() {
    echo -e "${GREEN}========================================Build iarmmgr===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "iarmmgrs" ]; then
        INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"
        SRCREV_iarmmgrs=$(grep -oP '^SRCREV:pn-iarmmgrs\s*=\s*"\K[0-9a-f]+' "$INC_FILE" || true)
        
        if [ -n "$SRCREV_iarmmgrs" ]; then
            git clone git@github.com:rdkcentral/iarmmgrs.git && cd iarmmgrs && git checkout "$SRCREV_iarmmgrs" || { echo "❌ Git clone failed."; exit 1; }
        else
            git clone git@github.com:rdkcentral/iarmmgrs.git && cd iarmmgrs && git checkout "a5781a35ac728b4d4b57a8a82e07e9b3b63c65f9" || { echo "❌ Git clone failed."; exit 1; }
        fi
        cd $WORKSPACE/deps/rdk/
    fi
    
    sudo cp $WORKSPACE/deps/rdk/iarmmgrs/mfr/include/*.h $INCLUDE_DIR 2>/dev/null || true
    cp $WORKSPACE/deps/rdk/devicesettings/hal/src/libds-hal.so $WORKSPACE/deps/rdk/devicesettings/install/lib 2>/dev/null || true
    
    cd $WORKSPACE/deps/rdk/iarmmgrs
    git apply $SCRIPTS_DIR/patches/rdkservices/iarmmgrs/Newiarm.patch 2>&1 || true
    cp $SCRIPTS_DIR/patches/rdkservices/iarmmgrs/build.sh $WORKSPACE/deps/rdk/iarmmgrs
    
    set +e
    (cd $WORKSPACE/deps/rdk/iarmmgrs && ./build.sh)
    set -e
    
    echo -e "${GREEN}✓ iarmmgr build completed${NC}"
}

# Function to build power-manager
build_power_manager() {
    echo -e "${GREEN}========================================Build power-manager===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "power-manager" ]; then
        git clone git@github.com:rdkcentral/power-manager.git
    fi
    
    echo -e "${GREEN}✓ power-manager cloned${NC}"
}

# Function to build hdmicec
build_hdmicec() {
    echo -e "${GREEN}========================================Build hdmicec===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "hdmicec" ]; then
        rm -rf act-hdmicec
        git clone git@github.com:rdkcentral/hdmicec.git
    fi
    
    cd $WORKSPACE/deps/rdk/hdmicec
    if [ ! -d "soc/L2HalMock/common" ]; then
        mkdir -p soc/L2HalMock
        cd $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/
        if [ ! -d "hdmicec-hal-emulator" ]; then
            git clone git@github.com:rdk-e/hdmicec-hal-emulator.git && cd hdmicec-hal-emulator && git checkout "2e459b5cf75eed032b9957d5c8ffab1da6d6cc3b"
            cd $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/
        fi
        if [ ! -d "common" ]; then
            mv hdmicec-hal-emulator common 2>/dev/null || true
        fi
    fi
    
    # Apply patches to fix HdmiCecGetLogicalAddress function signature
    cd $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common
    if [ -f "$SCRIPTS_DIR/patches/rdkservices/hdmicec/test_main_fix.patch" ]; then
        echo -e "${YELLOW}Applying patch to test/main.cpp...${NC}"
        if git apply "$SCRIPTS_DIR/patches/rdkservices/hdmicec/test_main_fix.patch" 2>&1; then
            echo -e "${GREEN}✓ test/main.cpp patch applied successfully${NC}"
        else
            echo -e "${YELLOW}⚠️ test/main.cpp patch may have already been applied, continuing...${NC}"
        fi
    fi
    if [ -f "$SCRIPTS_DIR/patches/rdkservices/hdmicec/hdmi_cec_driver_fix.patch" ]; then
        echo -e "${YELLOW}Applying patch to hdmi_cec_driver.c...${NC}"
        if git apply "$SCRIPTS_DIR/patches/rdkservices/hdmicec/hdmi_cec_driver_fix.patch" 2>&1; then
            echo -e "${GREEN}✓ hdmi_cec_driver.c patch applied successfully${NC}"
        else
            echo -e "${YELLOW}⚠️ hdmi_cec_driver.c patch may have already been applied, continuing...${NC}"
        fi
    fi
    cd $WORKSPACE/deps/rdk/hdmicec
    
    # Copy drivers folder from patches to workspace
    echo -e "${YELLOW}Copying drivers folder from patches to workspace...${NC}"
    # Copy to ccec/include/ccec/driver (for backward compatibility)
    mkdir -p $WORKSPACE/deps/rdk/hdmicec/ccec/include/ccec/driver
    # Copy to ccec/drivers/include/ccec/drivers (for the actual include path used by Makefile)
    mkdir -p $WORKSPACE/deps/rdk/hdmicec/ccec/drivers/include/ccec/drivers
    if [ -d "$SCRIPTS_DIR/patches/rdkservices/hdmicec/drivers/include/ccec/drivers" ]; then
        cp -r $SCRIPTS_DIR/patches/rdkservices/hdmicec/drivers/include/ccec/drivers/* $WORKSPACE/deps/rdk/hdmicec/ccec/include/ccec/driver/ 2>/dev/null || true
        cp -r $SCRIPTS_DIR/patches/rdkservices/hdmicec/drivers/include/ccec/drivers/* $WORKSPACE/deps/rdk/hdmicec/ccec/drivers/include/ccec/drivers/ 2>/dev/null || true
        echo -e "${GREEN}✓ Drivers folder copied successfully${NC}"
    else
        echo -e "${YELLOW}⚠️ Drivers folder not found in patches, skipping copy${NC}"
    fi
    
    # Copy telemetry header stub to include path
    echo -e "${YELLOW}Copying telemetry header stub...${NC}"
    mkdir -p $WORKSPACE/deps/rdk/hdmicec/ccec/include
    if [ -f "$SCRIPTS_DIR/patches/rdkservices/hdmicec/telemetry_busmessage_sender.h" ]; then
        cp $SCRIPTS_DIR/patches/rdkservices/hdmicec/telemetry_busmessage_sender.h $WORKSPACE/deps/rdk/hdmicec/ccec/include/ 2>/dev/null || true
        echo -e "${GREEN}✓ Telemetry header stub copied successfully${NC}"
    else
        echo -e "${YELLOW}⚠️ Telemetry header stub not found in patches, skipping copy${NC}"
    fi
    
    # Copy host directory if it doesn't exist (needed for some includes)
    if [ ! -d "$WORKSPACE/deps/rdk/hdmicec/host" ]; then
        echo -e "${YELLOW}Copying host directory from LATEST_INPUT_OUTPU...${NC}"
        if [ -d "/home/kishore/PROJECT/LATEST_INPUT_OUTPU/entservices-inputoutput/L2HalMock/workspace/deps/rdk/hdmicec/host" ]; then
            cp -r /home/kishore/PROJECT/LATEST_INPUT_OUTPU/entservices-inputoutput/L2HalMock/workspace/deps/rdk/hdmicec/host $WORKSPACE/deps/rdk/hdmicec/ 2>/dev/null || true
            echo -e "${GREEN}✓ Host directory copied successfully${NC}"
        else
            echo -e "${YELLOW}⚠️ Host directory not found in LATEST_INPUT_OUTPU, skipping copy${NC}"
        fi
    fi
    
    sudo cp $WORKSPACE/deps/rdk/power-manager/source/include/pwrMgr.h $INCLUDE_DIR 2>/dev/null || true
    sudo cp $WORKSPACE/deps/rdk/hdmicec/ccec/include/ccec/driver/hdmi_cec_driver.h $INCLUDE_DIR 2>/dev/null || true
    sudo cp $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common/*.hpp $INCLUDE_DIR 2>/dev/null || true
    
    cd $INCLUDE_DIR
    sudo mkdir -p json
    sudo cp $INCLUDE_DIR/jsoncpp/json/*.h $INCLUDE_DIR/json/ 2>/dev/null || true
    
    cp $SCRIPTS_DIR/patches/rdkservices/hdmicec/build.sh $WORKSPACE/deps/rdk/hdmicec
    (cd $WORKSPACE/deps/rdk/hdmicec && ./build.sh)
    check_error "Build hdmicec"
    echo -e "${GREEN}✓ hdmicec built successfully${NC}"
}

# Function to build AIDL interfaces
build_aidl() {
    echo -e "${GREEN}========================================Build AIDL Interfaces (rdk-halif-aidl)===============================================${NC}"
    mkdir -p "$WORKSPACE/deps/rdk/aidl"
    if [ -f "$SCRIPTS_DIR/patches/rdkservices/aidl/build.sh" ]; then
        echo -e "${YELLOW}Copying AIDL build script to aidl...${NC}"
        cp "$SCRIPTS_DIR/patches/rdkservices/aidl/build.sh" "$WORKSPACE/deps/rdk/aidl/rdk-halif-aidl/build.sh"
        chmod +x "$WORKSPACE/deps/rdk/aidl/rdk-halif-aidl/build.sh"
        echo -e "${YELLOW}Running AIDL build script from aidl...${NC}"
        (cd "$WORKSPACE/deps/rdk/aidl/rdk-halif-aidl" && ./build.sh)
        check_error "Build AIDL interfaces"
        echo -e "${GREEN}✓ AIDL interfaces built successfully${NC}"
    else
        echo -e "${YELLOW}⚠️ AIDL build script not found, skipping AIDL build${NC}"
    fi
}

# Function to build iarm_event_sender
build_iarm_event_sender() {
    echo -e "${GREEN}========================================Build iarm_event_sender===============================================${NC}"
    cd $WORKSPACE/deps/rdk/
    
    if [ ! -d "sender" ]; then
        git clone git@github.com:rdk-e/iarm_event_sender.git sender && cd sender && git checkout L2Halmock
        cd $WORKSPACE/deps/rdk/
    fi
    
    (cd $WORKSPACE/deps/rdk/sender/ && ./build.sh)
    check_error "Build iarm_event_sender"
    echo -e "${GREEN}✓ iarm_event_sender built successfully${NC}"
}

# Function to build HdmiCecSource (rdkservices)
build_hdmicecsource() {
    echo -e "${GREEN}========================================Build HdmiCecSource===============================================${NC}"
    
    if grep -q "HdmiCecSource" <<< "$SelectedPlugins"; then
        sudo cp $SCRIPTS_DIR/patches/rdkservices/properties/HdmiCecSource/device.properties /etc/ 2>/dev/null || true
    fi
    
    if grep -q "HdmiCecSink" <<< "$SelectedPlugins"; then
        sudo cp $SCRIPTS_DIR/patches/rdkservices/properties/HdmiCecSink/device.properties /etc/ 2>/dev/null || true
    fi
    
    sudo cp ${WORKSPACE}/install/usr/lib/cmake/WPEFramework/common/CmakeHelperFunctions.cmake $WORKSPACE/install/usr/include/WPEFramework/Modules 2>/dev/null || true
    
    # Copy runtime libraries
    if [ -f "$WORKSPACE/deps/rdk/iarmbus/install/libIARMBus.so" ]; then
        cp "$WORKSPACE/deps/rdk/iarmbus/install/libIARMBus.so" "$WORKSPACE/install/usr/lib/" 2>/dev/null || true
    fi
    if [ -f "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCEC.so" ]; then
        cp "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCEC.so" "$WORKSPACE/install/usr/lib/" 2>/dev/null || true
    fi
    if [ -f "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCECHal.so" ]; then
        cp "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCECHal.so" "$WORKSPACE/install/usr/lib/" 2>/dev/null || true
    fi
    if [ -f "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCECOSHal.so" ]; then
        cp "$WORKSPACE/deps/rdk/hdmicec/install/lib/libRCECOSHal.so" "$WORKSPACE/install/usr/lib/" 2>/dev/null || true
    fi
    
    cp -r $RDK_DIR/Tests/L2HALMockTests/. $WORKSPACE/deps/rdk/flask 2>/dev/null || true
    
    # Copy rfcapi.h
    if [ -f "$RDK_DIR/L2HalMock/patches/rdkservices/files/rfcapi.h" ]; then
        mkdir -p "$RDK_DIR/helpers"
        cp "$RDK_DIR/L2HalMock/patches/rdkservices/files/rfcapi.h" "$RDK_DIR/helpers/rfcapi.h" 2>/dev/null || true
    fi
    
    cd $INCLUDE_DIR
    mkdir -p interfaces
    sudo cp $SCRIPTS_DIR/entservices-apis/apis/HdmiCecSource/IHdmiCecSource.h $INCLUDE_DIR/interfaces 2>/dev/null || true
    
    cd $RDK_DIR;
    
    # Set up cmake paths for Thunder components
    THUNDER_BUILD="workspace/entservices-inputoutput/L2HalMock/Thunder/build"
    THUNDER_CMAKE="workspace/entservices-inputoutput/L2HalMock/Thunder/cmake"
    
    # Create WPEFrameworkConfig.cmake if it doesn't exist
    if [ ! -f "$THUNDER_BUILD/WPEFrameworkConfig.cmake" ]; then
        cat > /tmp/WPEFrameworkConfig.cmake << 'EOFCMAKE'
# WPEFramework config file - minimal version for build compatibility
if(NOT TARGET WPEFramework::WPEFramework)
    get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
    if(EXISTS "${_DIR}/Source/core/WPEFrameworkCoreTargets.cmake")
        include("${_DIR}/Source/core/WPEFrameworkCoreTargets.cmake")
    endif()
    if(TARGET WPEFrameworkCore)
        add_library(WPEFramework::WPEFramework ALIAS WPEFrameworkCore)
    endif()
endif()
set(WPEFramework_FOUND TRUE)
EOFCMAKE
        sudo cp /tmp/WPEFrameworkConfig.cmake "$THUNDER_BUILD/WPEFrameworkConfig.cmake"
    fi
    
    # Copy Thunder cmake targets to expected locations
    echo -e "${YELLOW}Copying Thunder cmake targets...${NC}"
    sudo cp "$THUNDER_BUILD/Source/core/CMakeFiles/Export/lib/cmake/WPEFrameworkCore/WPEFrameworkCoreTargets.cmake" "$THUNDER_BUILD/Source/core/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/plugins/CMakeFiles/Export/lib/cmake/WPEFrameworkPlugins/WPEFrameworkPluginsTargets.cmake" "$THUNDER_BUILD/Source/plugins/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/com/CMakeFiles/Export/lib/cmake/WPEFrameworkCOM/WPEFrameworkCOMTargets.cmake" "$THUNDER_BUILD/Source/com/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/messaging/CMakeFiles/Export/lib/cmake/WPEFrameworkMessaging/WPEFrameworkMessagingTargets.cmake" "$THUNDER_BUILD/Source/messaging/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/websocket/CMakeFiles/Export/lib/cmake/WPEFrameworkWebSocket/WPEFrameworkWebSocketTargets.cmake" "$THUNDER_BUILD/Source/websocket/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/cryptalgo/CMakeFiles/Export/lib/cmake/WPEFrameworkCryptalgo/WPEFrameworkCryptalgoTargets.cmake" "$THUNDER_BUILD/Source/cryptalgo/" 2>/dev/null || true
    sudo cp "$THUNDER_BUILD/Source/plugins/CMakeFiles/Export/lib/cmake/ConfigGenerator/ConfigGeneratorTargets.cmake" "$THUNDER_BUILD/Source/plugins/" 2>/dev/null || true
    
    WORKSPACE="/workspace/entservices-inputoutput/L2HalMock/workspace"
    SCRIPTS_DIR="/workspace/entservices-inputoutput/L2HalMock"
    
    cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="/workspace/entservices-inputoutput/L2HalMock/Thunder/cmake/modules" \
    -DWPEFrameworkDefinitions_DIR="$SCRIPTS_DIR/install/lib/cmake/WPEFrameworkDefinitions" \
    -DCompileSettingsDebug_DIR="$SCRIPTS_DIR/install/lib/cmake/CompileSettingsDebug" \
    -DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
    -DRDK_SERVICE_L2HALMOCK=ON \
    -DUSE_THUNDER_R4=ON \
    -DPLUGIN_HDMICECSOURCE=ON \
    -DPLUGIN_HDMICECSINK=OFF \
    -DCOMCAST_CONFIG=OFF \
    -DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
    -DOSAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/osal/include" \
    -DCEC_HOST_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/host/include" \
    -DDS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libds.so" \
    -DDS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/ds/include" \
    -DDSHAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/hal/include" \
    -DDSRPC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/rpc/include" \
    -DIARMBUS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/core/include" \
    -DIARMRECEIVER_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/receiver/include" \
    -DIARMPWR_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/hal/include" \
    -DCEC_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCEC.so" \
    -DIARMBUS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/install/libIARMBus.so" \
    -DDSHAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libdshalcli.so" \
    -DCEC_HAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECHal.so" \
    -DOSAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECOSHal.so" \
    -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -Wall -Werror -Wno-error=format=-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog"
    
    (cd $RDK_DIR && cmake --build build --target install)
    check_error "Build HdmiCecSource"
    
    # Enable autostart
    cd $WORKSPACE/install/etc/WPEFramework/plugins/ 2>/dev/null || true
    HdmiCecSourceJsonFile=$(find . -maxdepth 1 -name "*HdmiCecSource*.json" -o -name "*CecSource*.json" 2>/dev/null | head -n 1)
    if [ -n "$HdmiCecSourceJsonFile" ] && [ -f "$HdmiCecSourceJsonFile" ]; then
        sed -i 's/"autostart":false/"autostart":true/' "$HdmiCecSourceJsonFile"
        echo "✅ HdmiCecSource autostart enabled"
    fi
    
    echo -e "${GREEN}✓ HdmiCecSource built successfully${NC}"
}

# Function to build HdmiCecSource (rdkservices)
build_deviceanddisplay() {
    echo -e "${GREEN}========================================Build DeviceAndDisplay (rdkservices)===============================================${NC}"
    cd $RDK_DIR/entservices-deviceanddisplay;
        cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" \
    -DCMAKE_PREFIX_PATH="${WORKSPACE}/install/usr;${THUNDER_BUILD};${THUNDER_BUILD}/Source/core;${THUNDER_BUILD}/Source/plugins;${THUNDER_BUILD}/Source/com;${THUNDER_BUILD}/Source/messaging;${THUNDER_BUILD}/Source/websocket;${THUNDER_BUILD}/Source/cryptalgo" \
    -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules;${THUNDER_CMAKE}/common;${THUNDER_CMAKE}/modules" \
    -DWPEFramework_DIR="${THUNDER_BUILD}/Source/core" \
    -DWPEFrameworkCore_DIR="${THUNDER_BUILD}/Source/core" \
    -DWPEFrameworkPlugins_DIR="${THUNDER_BUILD}/Source/plugins" \
    -DWPEFrameworkCOM_DIR="${THUNDER_BUILD}/Source/com" \
    -DWPEFrameworkMessaging_DIR="${THUNDER_BUILD}/Source/messaging" \
    -DWPEFrameworkWebSocket_DIR="${THUNDER_BUILD}/Source/websocket" \
    -DWPEFrameworkCryptalgo_DIR="${THUNDER_BUILD}/Source/cryptalgo" \
    -DConfigGenerator_DIR="${THUNDER_BUILD}/Source/plugins" \
    -DWPEFrameworkDefinitions_DIR="$SCRIPTS_DIR/install/lib/cmake/WPEFrameworkDefinitions" \
    -DCompileSettingsDebug_DIR="$SCRIPTS_DIR/install/lib/cmake/CompileSettingsDebug" \
    -DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
    -DRDK_SERVICE_L2HALMOCK=ON \
    -DUSE_THUNDER_R4=ON \
    -DPLUGIN_POWERMANAGER=ON \
    -DCOMCAST_CONFIG=OFF \
    -DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
    -DOSAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/osal/include" \
    -DCEC_HOST_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/host/include" \
    -DDS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libds.so" \
    -DDS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/ds/include" \
    -DDSHAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/hal/include" \
    -DDSRPC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/rpc/include" \
    -DIARMBUS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/core/include" \
    -DIARMRECEIVER_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/receiver/include" \
    -DIARMPWR_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/hal/include" \
    -DCEC_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCEC.so" \
    -DIARMBUS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/install/libIARMBus.so" \
    -DDSHAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libdshalcli.so" \
    -DCEC_HAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECHal.so" \
    -DOSAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECOSHal.so" \
    -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -Wall -Werror -Wno-error=format=-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog"
}
# Main menu
PS3='Please enter your choice: '
options=(
    "Build Third-Party (flux, directfb, log4c, safeclib, dbus, glib)"
    "Build Thunder Tools"
    "Build Thunder"
    "Build ThunderClientLibraries"
    "Build entservices-apis"
    "Build IARM"
    "Build DeviceSettings"
    "Build RFC"
    "Build iarmmgr"
    "Build power-manager"
    "Build hdmicec"
    "Build AIDL Interfaces"
    "Build iarm_event_sender"
    "Build HdmiCecSource (rdkservices)"
    "Build DeviceAndDisplay (rdkservices)"
    "Build All"
    "Quit"
)

select opt in "${options[@]}"
do
    case $opt in
        "Build Third-Party (flux, directfb, log4c, safeclib, dbus, glib)")
            echo "You chose: $opt"
            build_third_party
            ;;
        "Build Thunder Tools")
            echo "You chose: $opt"
            build_thunder_tools
            ;;
        "Build Thunder")
            echo "You chose: $opt"
            build_thunder
            ;;
        "Build ThunderClientLibraries")
            echo "You chose: $opt"
            build_thunder_client_libraries
            ;;
        "Build entservices-apis")
            echo "You chose: $opt"
            build_entservices_apis
            ;;
        "Build IARM")
            echo "You chose: $opt"
            build_iarm
            ;;
        "Build DeviceSettings")
            echo "You chose: $opt"
            build_devicesettings
            ;;
        "Build RFC")
            echo "You chose: $opt"
            build_rfc
            ;;
        "Build iarmmgr")
            echo "You chose: $opt"
            build_iarmmgr
            ;;
        "Build power-manager")
            echo "You chose: $opt"
            build_power_manager
            ;;
        "Build hdmicec")
            echo "You chose: $opt"
            build_hdmicec
            ;;
        "Build AIDL Interfaces")
            echo "You chose: $opt"
            build_aidl
            ;;
        "Build iarm_event_sender")
            echo "You chose: $opt"
            build_iarm_event_sender
            ;;
        "Build HdmiCecSource (rdkservices)")
            echo "You chose: $opt"
            build_hdmicecsource
            ;;
        "Build DeviceAndDisplay (rdkservices)")
            echo "You chose: $opt"
            build_deviceanddisplay
            ;;
        "Build All")
            echo "You chose: $opt"
            echo -e "${GREEN}========================================Building All Components===============================================${NC}"
            build_third_party
            build_thunder_tools
            build_thunder
            build_entservices_apis
            build_thunder_client_libraries
            build_iarm
            build_devicesettings
            build_rfc
            build_iarmmgr
            build_power_manager
            build_hdmicec
            build_aidl
            build_iarm_event_sender
            build_hdmicecsource
            echo -e "${GREEN}========================================All builds completed!===============================================${NC}"
            ;;
        "Quit")
            break
            ;;
        *) 
            echo "Invalid option $REPLY"
            ;;
    esac
    echo ""
    echo -e "${YELLOW}1) Third-Party 2) Thunder Tools 3) Thunder 4) ThunderClientLibraries 5) entservices-apis 6) IARM 7) DeviceSettings 8) RFC 9) iarmmgr 10) power-manager 11) hdmicec 12) AIDL 13) iarm_event_sender 14) HdmiCecSource 15) Build All 16) Quit${NC}"
done
