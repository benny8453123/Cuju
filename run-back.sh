#!/bin/bash

sudo ./x86_64-softmmu/qemu-system-x86_64 -drive if=none,id=drive0,cache=none,format=raw,file=/home/cuju/data/Ubuntu20G-1604.img -device virtio-blk,drive=drive0,scsi=off -m 1G -enable-kvm -net tap,ifname=tap1 -net nic,model=virtio,vlan=0,macaddr=ae:ae:00:00:00:25 -vga std -chardev socket,id=mon,path=/home/cuju/vm1r.monitor,server,nowait -mon chardev=mon,id=monitor,mode=readline -incoming tcp:127.0.0.1:4441,ft_mode \
-blk-server 127.0.0.1:5001 \
-ft-join-port 4449
