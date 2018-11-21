#!/bin/bash

#no vhost gdb
#sudo gdb --tui -ex 'set confirm off' --args ./x86_64-softmmu/qemu-system-x86_64 -drive if=none,id=drive0,cache=none,format=raw,file=/home/ben/Ubuntu20G-1604.img -device virtio-blk,drive=drive0,scsi=off -m 1G -enable-kvm -net tap,ifname=tap0 -net nic,model=virtio,vlan=0,macaddr=ae:ae:00:00:00:25 -vga std -chardev socket,id=mon,path=/home/ben/vm1.monitor,server,nowait -mon chardev=mon,id=monitor,mode=readline

#no vhost blk-server-pri
#sudo gdb --tui -ex 'set confirm off' --args ./x86_64-softmmu/qemu-system-x86_64 -drive if=none,id=drive0,cache=none,format=raw,file=/home/ben/data/Ubuntu20G-1604.img -device virtio-blk,drive=drive0,scsi=off -m 1G -enable-kvm -net tap,ifname=tap2 -net nic,model=virtio,vlan=0,macaddr=ae:ae:00:00:00:25 -vga std -chardev socket,id=mon,path=/home/ben/vm1.monitor,server,nowait -mon chardev=mon,id=monitor,mode=readline \
sudo gdb --tui -ex 'set confirm off' --args ./x86_64-softmmu/qemu-system-x86_64 -drive if=none,id=drive0,cache=none,format=raw,file=/home/ben/nfs_folder/Ubuntu20G-1604.img -device virtio-blk,drive=drive0,scsi=off -m 1G -enable-kvm -net tap,ifname=tap2 -net nic,model=virtio,vlan=0,macaddr=ae:ae:00:00:00:25 -vga std -chardev socket,id=mon,path=/home/ben/vm1.monitor,server,nowait -mon chardev=mon,id=monitor,mode=readline \
-blk-server-listen :5001 \
-ft-join-port 4447


#vhost
#sudo ./x86_64-softmmu/qemu-system-x86_64 -device vhost-scsi-pci,id=vhost-scsi0,wwpn=naa.6001405af7cd6025 \
#-device virtio-scsi-pci,id=scsi0 \
#-m 1G -enable-kvm \
#-net nic,model=virtio,vlan=0,macaddr=ae:ae:00:00:00:25,netdev=net0 \
#-netdev tap,id=net0,ifname=tap0,vhost=on \
#-vga std -chardev socket,id=mon,path=/home/ben/vm1.monitor,server,nowait -mon chardev=mon,id=monitor,mode=readline

