#!/bin/sh -ex

if [ $# -lt 3 ]
then
        echo "Usge: $0 <image name> <mount point> <.efi file> {another file}"
        # 1 means abnormal end. error
        exit 1
fi

DEVENV_DIR=$(dirname "$0")
DISK_IMG=$1
MOUNT_POINT=$2
EFI_FILE=$3
ANOTHER_FILE=$4

if [ ! -f $EFI_FILE ]
then
	echo "No such file: $EFI_FILE"
	exit 1
fi

rm -f $DISK_IMG
# -f means format
qemu-img create -f raw $DISK_IMG 200M
# mkfs.fat means that create FAT filesystem. -n means name of volume label.
# -s means number of cluster per sector. -f means FAT version. -R means number of reserved sector. -F means format version of FAT.
mkfs.fat -n 'MIKAN OS-fu' -s 2 -f 2 -R 32 -F 32 $DISK_IMG

$DEVENV_DIR/mount_image.sh $DISK_IMG $MOUNT_POINT
sudo mkdir -p $MOUNT_POINT/EFI/BOOT
sudo cp $EFI_FILE $MOUNT_POINT/EFI/BOOT/BOOTX64.EFI

if [ "$ANOTHER_FILE" != "" ]
then
	sudo cp $ANOTHER_FILE $MOUNT_POINT/
fi

sleep 0.5
sudo umount $MOUNT_POINT
