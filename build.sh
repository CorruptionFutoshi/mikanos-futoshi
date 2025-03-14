# this is called shebang. it means this script is executed by /bin/sh.
# -e means if error has occur, script is end. -u means if use undefined variable, script is end. 
#!/bin/sh -eu

# ${MAKE_OPTS: -} means if MAKE_OPTS environmental variable is defined, its value, else empty
# -C means cd
make ${MAKE_OPTS: -} -C kernel kernel.elf

# loop and execute make command about makefile in apps directory 
for MK in $(ls apps/*/Makefile)
do
	APP_DIR=$(dirname $MK)
	# basename means extract last word of path
	APP=$(basename $APP_DIR)
	make ${MAKE_OPTS:-} -C $APP_DIR $APP
done

# if there some declaration and command in same row, command can use declared variables. So dont have to set DISK_IMG and MIKANOS_DIR as parameter. $PWD means current directory
# DISK_IMG=./disk.img MIKANOS_DIR=$PWD $HOME/mikanos-futoshi/devenv/make_mikanos_image.sh

# ${1:-} means if first parameter is confirmed, its value, else empty
if [ "${1:-}" = "run" ]
then
	# execute run_image.sh with parameter ./disk.img
	# $HOME/mikanos-futoshi/devenv/run_image.sh ./disk.img
	
	MIKANOS_DIR=$PWD $HOME/mikanos-futoshi/devenv/run_mikanos.sh
fi
