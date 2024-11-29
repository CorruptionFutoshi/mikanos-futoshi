BASEDIR="$HOME/mikanos-futoshi/devenv/x86_64-elf"
EDK2DIR="$HOME/edk2"

# !($BASEDIR is directory?) -d option means check target is directory or not
if [! -d $BASEDIR]
then
	echo "$BASEDIR is not exist"
	echo "download following files manually, and extract them to $(dirname $BASEDIR)"
	echo "https://github.com/uchan-nos/mikanos-build/releases/download/v2.0/x86_64-elf.tar.gz"	
else
	#-I means add filepath to find include files
	export CPPFLAGS="\
	-I$BASEDIR/include/c++/v1 \
	-I$BASEDIR/include \
	-I$BASEDIR/include/freetype2 \	
       	-I$EDK2DIR/MdePkg/Include \
	-I$EDK2DIR/MdePkg/include/X64 \
	# -nostdlibinc means not include include files in standard library
	-nostdlibinc \
	# -D means definition
	# __ELF__ means file is ELF format 
	-D__ELF__ \
	# _LDBL_EQ_DBL means size of longdouble equalls size of double 
	-D_LDBL_EQ_DBL \
	# _GNU_SOURCE means enable GNU extensions
	-D_GNU_SOURCE \
	# _POSIX_TIMES means Posix timer method is usable 
	-D_POSIX_TIMES \
	# set attribute of EFIAPI. ms_abi is one of the ABI in Windows platform
	-DEFIAPI='__attribute__((ms_abi))'"
	# -L means add filepath to find library files
	export LDFLAGS="-L$BASEDIR/lib"
fi	
