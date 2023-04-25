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
        cmd=$(cat $proceso/cmdline | tr '\0' ' ' | awk '{printf"%s", $1}')

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
    let i=2

    echo "Source:Port             Destination:Port        Status"
    while true;
    do
        if [ $1 = "1" ]; then
            sourcePort=$(cat /proc/net/tcp | tail -n +$i | sed -n 2p | awk '{printf"%s", $2}')
            destinationPort=$(cat /proc/net/tcp | tail -n +$i | sed -n 2p | awk '{printf"%s", $3}')
            status=$(cat /proc/net/tcp | tail -n +$i | sed -n 2p | awk '{printf"%s", $4}')
        elif [ $1 = "2" ]; then
            sourcePort=$(cat /proc/net/tcp | sort -k4  | tail -n +$i | sed -n 2p | awk '{printf"%s", $2}')
            destinationPort=$(cat /proc/net/tcp | sort -k4 | tail -n +$i | sed -n 2p | awk '{printf"%s", $3}')
            status=$(cat /proc/net/tcp | sort -k4 | tail -n +$i | sed -n 2p | awk '{printf"%s", $4}')
        fi

        if [ -z $sourcePort ]; then
            break
        fi
        
        if [ $status = "01" ]; then
            statusAux="ESTABLISHED"
        elif [ $status = "02" ]; then 
            statusAux="SYN_SENT"
        elif [ $status = "03" ]; then
            statusAux="SYN_RECV"
        elif [ $status = "04" ]; then
            statusAux="FIN_WAIT1"
        elif [ $status = "05" ]; then
            statusAux="FIN_WAIT2"
        elif [ $status = "06" ]; then
            statusAux="TIME_WAIT"
        elif [ $status = "07" ]; then
            statusAux="CLOSE"
        elif [ $status = "08" ]; then
            statusAux="CLOSE_WAIT"
        elif [ $status = "09" ]; then
            statusAux="LAST_ACK"
        elif [ $status = "0A" ]; then
            statusAux="LISTEN"
        elif [ $status = "0B" ]; then
            statusAux="CLOSING"
        fi

        ipSourse=$(echo $sourcePort | cut -d ":" -f 1)
        portSourse=$(echo $sourcePort | cut -d ":" -f 2)
        ipDestination=$(echo $destinationPort | cut -d ":" -f 1)
        portDestination=$(echo $destinationPort | cut -d ":" -f 2)

        ipSource_dec=$(printf "%d.%d.%d.%d\n" 0x${ipSourse:6:2} 0x${ipSourse:4:2} 0x${ipSourse:2:2} 0x${ipSourse:0:2})
        portSource_dec=$((0x${portSourse}))
        ipDestination_dec=$(printf "%d.%d.%d.%d\n" 0x${ipDestination:6:2} 0x${ipDestination:4:2} 0x${ipDestination:2:2} 0x${ipDestination:0:2})
        portDestination_dec=$((0x${portDestination}))
   
        echo "$ipSource_dec $portSource_dec $ipDestination_dec $portDestination_dec $statusAux" | awk '{printf"%s:%s %20s:%s %17s\n", $1, $2, $3, $4, $5}'
        let i=$i+1
    done

    echo
}

function help { 
    echo "###############################################"
    echo "#               Lista de comandos             #"
    echo "#                                             #"
    echo "# 1) -ps: Muestra la información de todos los #"
    echo "#         procesos que tiene el sistema.      #"
    echo "#                                             #"
    echo "# 2) -m: Muestra la cantidad de memoria ram   #"
    echo "#        total y la cantidad de memoria       #"
    echo "#        disponible.                          #"
    echo "#                                             #"
    echo "# 3) -tcp: Muestra las conexiones TCP.        #"
    echo "#                                             #"
    echo "# 4) -tcpStatus: Muestra las conexiones TCP   #"
    echo "#                ordenadas por status.        #"
    echo "###############################################"          

    echo
}

mostrarInfoPc

while true;
do
    read opcion
    echo

    if [ $opcion = "-ps" ]; then
        infoProcesos
    elif [ $opcion = "-m" ]; then
        infoMemoriaRam
    elif [ $opcion = "-tcp" ]; then
        conexionesTCP "1"
    elif [ $opcion = "-tcpStatus" ]; then
        conexionesTCP "2"
    elif [ $opcion = "-help" ]; then
        help
    elif [ $opcion = "-exit" ]; then
        exit
    else
        echo "error: $opcion no existe, escriba -help para ver la lista de comandos"
        echo
    fi
done