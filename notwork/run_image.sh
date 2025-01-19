#!/bin/sh -ex

if [ $# -lt 1 ]
then
    echo "Usage: $0 <image name>"
    # 1 mean abnormal end.error
    exit 1
fi

DEVENV_DIR=$(dirname "$0")
DISK_IMG=$1

if [ ! -f $DISK_IMG ]
then
    echo "No such file: $DISK_IMG"
    exit 1
fi

    # start virtual machine by using QEMU
    qemu-system-x86_64 \
    # specify 1G for memmorysize
    -m 1G \
    # set start option about code of UEFI firmware. if means interface. OVMF_CODE.fd is code file of UEFI firmware
    -drive if=pflash,format=raw,readonly,file=$DEVENV_DIR/OVMF_CODE.fd \
    # set start option about strage of UEFI variables. OVMF_VARS.fd is strage of UEFI variables
    -drive if=pflash,format=raw,file=$DEVENV_DIR/OVMF_VARS.fd \
    # connect specified image file to virtual machine 
    -drive if=ide,index=0,media=disk,format=raw,file=$DISK_IMG \
    # add USB controller device
    -device nec-usb-xhci,id=xhci \
    -device usb-mouse \
    -device usb-kbd \
    # connect monitor of QEMU to standard input/output
    -monitor stdio \
    # if additional QEMU options, apply them
    $QEMU_OPTS
