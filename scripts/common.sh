#!/bin/bash

SDB=$(which sdb)

read_one_of() {
    local __prompt=$1
    local __options=$2
    local __result=$3
    local __tmp=""
    local options_str="[$(echo ${__options[@]} | sed 's/ /\//g')]"
    while [ true ]; do
        read -p "$__prompt $options_str " __tmp
        for opt in $__options; do
            local lower_opt=$(echo "$opt" | awk '{ print(tolower($0)) }')
            local lower_tmp=$(echo "$__tmp" | awk '{ print(tolower($0)) }')
            if [ "$lower_opt" == "$lower_tmp" ]; then
                export $__result="$opt"
                return
            fi
        done
    done
}

read_file() {
    local __prompt=$1
    local __result=$2
    local __tmp=""
    while [ ! -f "$__tmp" ]; do
        read -p "$__prompt" __tmp
        if [ ! -f "$__tmp" ]; then
            echo "Can't find file $__tmp"
        fi
    done
    export $__result="$__tmp"
}

read_dir() {
    local __prompt=$1
    local __result=$2
    local __tmp=""
    while [ ! -d "$__tmp" ]; do
        read -p "$__prompt" __tmp
        if [ ! -d "$__tmp" ]; then
            echo "Can't find directory $__tmp"
        fi
    done
    export $__result="$__tmp"
}

read_consent() {
    local __prompt=$1
    local __result=$2
    local __consent=""
    read_one_of "$__prompt" "Y n" __consent
    if [ "$__consent" == "Y" ]; then
        export $__result=true
    else
        export $__result=false
    fi
}

test_sdb_version() {
    if [ -z "$SDB" ]; then
        read_file "sdb was not found. Enter sdb path [] " SDB
    fi
    ver=( $($SDB version | sed -r 's/.*([0-9]+)\.([0-9]+)\.([0-9]+).*/\1 \2 \3/g') )
    if [[ "${ver[0]}" < "2" ]] || [[ "${ver[1]}" < "3" ]]; then
        echo "Unsupported sdb version:  ${ver[0]}.${ver[1]}.${ver[2]}. Please update sdb to at least 2.3.0"
        exit 1
    fi
    echo "Found sdb version ${ver[0]}.${ver[1]}.${ver[2]}"

    local num_devices=$($SDB devices | tail -n +2 | wc -l)
    if [ "$num_devices" == "0" ]; then
        echo "Error: no devices attached"
        exit 1
    elif [ "$num_devices" != "1" ]; then
        echo "Error: only one device must be attached"
        exit 1
    fi
}
