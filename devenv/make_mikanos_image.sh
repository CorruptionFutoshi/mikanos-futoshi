# -x means display executed commands for debug
#!/bin/sh -ex

# dirname means extract directory name. $0 means name or path of this executing script 
DEVENV_DIR=$(dirname "$0")
MOUNT_POINT=./mnt

if [ "$DISK_IMG" = "" ]
then
	DISK_IMG=./mikanos.img
fi

if [ "$MIKANOS_DIR" = "" ]
then
	# $# means number of parameters. -lt means less than
	if [ $# -lt 1 ]
	then
		# <day> means day. Probably execute from other than build.sh, need day.
		echo "Usage: $0 <day>"
		exit 1
	fi
	MIKANOS_DIR="$HOME/mikanos-futoshi/$1"
fi

LOADER_EFI="$HOME/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/BOOTX64.efi"
KERNEL_ELF="$MIKANOS_DIR/kernel/kernel.elf"

$DEVENV_DIR/make_image.sh $DISK_IMG $MOUNT_POINT $LOADER_EFI $KERNEL_ELF
$DEVENV_DIR/mount_image.sh $DISK_IMG $MOUNT_POINT

if [ "$APPS_DIR" != "" ]
then
	sudo mkdir $MOUNT_POINT/$APPS_DIR
fi

for APP in $(ls "$MIKANOS_DIR/apps")
do
	# -f means file or not
	if [ -f $MIKANOS_DIR/apps/$APP/$APP ]
	then
		sudo cp "$MIKANOS_DIR/apps/$APP/$APP" $MOUNT_POINT/$APPS_DIR
	fi
done

if [ "$RESOURCE_DIR" != "" ]
then
	sudo cp $MIKANOS_DIR/$RESOURCE_DIR/* $MOUNT_POINT/
fi

# wait 0.5 second for execution should be complete
sleep 0.5
# unmount means that disconnect file system from this script. But file system  is continuously usable from caller script 
sudo umount $MOUNT_POINT
