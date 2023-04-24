#!/bin/bash

function mostrarInfoPc {
    modelName=$(cat /proc/cpuinfo | grep -m 1 "model name" | awk '{printf"%s %s %s %s %s %s", $4, $5, $6, $7, $8, $9}')
    kernelVersion=$(cat /proc/version | awk '{print $3}')
    sizeMemory=$(cat /proc/meminfo | grep "MemTotal" | awk '{print $2, $3}')

    echo "ModelName: $modelName"
    echo "KernelVersion: $kernelVersion"
    echo "Memory (kb): $sizeMemory"
    echo 
}

function infoProcesos {

    echo "    UID         PID    PPID      STATUS    CMD"

    for proceso in /proc/[0-9]*/;
    do
        uid=$(cat $proceso/status | grep "Uid" | awk '{printf"%s", $2}')
        pid=$(cat $proceso/status | grep "Pid" | awk '{printf"%s", $2}')
        ppid=$(cat $proceso/status | grep "PPid" | awk '{printf"%s", $2}')
        status=$(cat $proceso/status | grep "State" | awk '{printf"%s", $3}')
        cmd=$(cat $proceso/cmdline | tr '\0' ' ')

        printf "%6s %12s %7s %12s   %s\n" "$uid" "$pid" "$ppid" "$status" "$cmd"
    done

    echo
}

function infoMemoriaRam {
    memoriaTotal=$(cat /proc/meminfo | grep "MemTotal" | awk '{print $2}')
    memoriaLibre=$(cat /proc/meminfo | grep "MemAvailable" | awk '{print $2}')

    echo "Total  Available"
    echo "$memoriaTotal $memoriaLibre 1000000" | awk '{printf"%3.1f %8.1f\n", $1 / $3, $2 / $3}'
    echo
}

function conexionesTCP {
    let i=0

    echo "Source:Port             Destination:Port        Status"
    while true;
    do
        sourcePort=$(cat /proc/net/tcp | grep "$i: " | awk '{printf"%s", $2}')
        destinationPort=$(cat /proc/net/tcp | grep "$i: " | awk '{printf"%s", $3}')
        state=$(cat /proc/net/tcp | grep "$i: " | awk '{printf"%s", $4}')

        if [ -z $sourcePort ]; then
            break
        fi
        
        if [ $state = "01" ]; then
            stateAux="ESTABLISHED"
        elif [ $state = "02" ]; then 
            stateAux="SYN_SENT"
        elif [ $state = "03" ]; then
            stateAux="SYN_RECV"
        elif [ $state = "04" ]; then
            stateAux="FIN_WAIT1"
        elif [ $state = "05" ]; then
            stateAux="FIN_WAIT2"
        elif [ $state = "06" ]; then
            stateAux="TIME_WAIT"
        elif [ $state = "07" ]; then
            stateAux="CLOSE"
        elif [ $state = "08" ]; then
            stateAux="CLOSE_WAIT"
        elif [ $state = "09" ]; then
            stateAux="LAST_ACK"
        elif [ $state = "0A" ]; then
            stateAux="LISTEN"
        elif [ $state = "0B" ]; then
            stateAux="CLOSING"
        fi

        ipSourse=$(echo $sourcePort | cut -d ":" -f 1)
        portSourse=$(echo $sourcePort | cut -d ":" -f 2)
        ipDestination=$(echo $destinationPort | cut -d ":" -f 1)
        portDestination=$(echo $destinationPort | cut -d ":" -f 2)

        ipSource_dec=$(printf "%d.%d.%d.%d\n" 0x${ipSourse:6:2} 0x${ipSourse:4:2} 0x${ipSourse:2:2} 0x${ipSourse:0:2})
        portSource_dec=$((0x${portSourse}))
        ipDestination_dec=$(printf "%d.%d.%d.%d\n" 0x${ipDestination:6:2} 0x${ipDestination:4:2} 0x${ipDestination:2:2} 0x${ipDestination:0:2})
        portDestination_dec=$((0x${portDestination}))

        echo "$ipSource_dec $portSource_dec $ipDestination_dec $portDestination_dec $stateAux" | awk '{printf"%s:%s %20s:%s %17s\n", $1, $2, $3, $4, $5}'
        let i=$i+1
    done

    echo
}

function help { 
    echo
}

mostrarInfoPc

while true;
do
    read opcion

    if [ $opcion = "-ps" ]; then
        infoProcesos
    elif [ $opcion = "-m" ]; then
        infoMemoriaRam
    elif [ $opcion = "-tcp" ]; then
        conexionesTCP
    elif [ $opcion = "-help" ]; then
        help
    elif [ $opcion = "-exit" ]; then
        exit
    fi
done