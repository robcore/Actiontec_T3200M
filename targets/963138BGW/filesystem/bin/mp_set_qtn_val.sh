#! /bin/sh

if [ "$#" != "2" ]; then
    echo "---HELP---"
    echo "$0 console_access on/off"
    echo "$0 manuMode 1/0"
    echo "---END---"
    exit 1
fi

name=${1}
val=${2}

if [ ${name} = "console_access" ]; then
    if [[ ${val} != "on" && ${val} != "off" ]]; then
        echo "invalid val"
	exit 1
    fi
    console_access=$(qcsapi_sockrpc get_bootcfg_param console_access)
    if [ ${console_access} != ${val} ]; then
        qcsapi_sockrpc update_bootcfg_param console_access ${val}
        qcsapi_sockrpc commit_bootcfg
	if [ ${val} != $(qcsapi_sockrpc get_bootcfg_param console_access) ]; then
	    echo "fail"
	    exit 1
        fi
	echo "success"
	exit 0
    else
	echo "success"
	exit 0
    fi
fi

if [ ${name} = "manuMode" ]; then
    if [[ ${val} != "1" && ${val} != "0" ]]; then
        echo "invalid val"
	exit 1
    fi
    manuMode=$(qcsapi_sockrpc get_bootcfg_param manuMode)
    if [ ${manuMode} != ${val} ]; then
        qcsapi_sockrpc update_bootcfg_param manuMode ${val}
        qcsapi_sockrpc commit_bootcfg
        if [ ${val} != $(qcsapi_sockrpc get_bootcfg_param manuMode) ]; then
	    echo "fail"
	    exit 1
	fi
	echo "success"
	exit 0
    else
	echo "success"
	exit 0
    fi
fi
