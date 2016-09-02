#!/bin/zsh
setopt SH_WORD_SPLIT

SCRIPT="$(readlink -f $0)"
SCRIPTPATH="$(dirname $SCRIPT)"
cd $SCRIPTPATH

module purge
module load intel/16.0.3 
module load fca/2.5.2393
module load mxm/3.3.3002
module load bullxmpi_mlx_mt/bullxmpi_mlx_mt-1.2.8.3
#module load gcc/5.1.0
#module load openmpi/1.8.4-gcc51
#module load gcc/4.9.2
#module load openmpi/1.8.4-gcc49
module load betke/hdf5/1.8.17
module load betke/netcdf/4.4.0

module list

CC="$(which mpicc)"
CFLAGS="-g3 -Wall -O0 -std=gnu11 $(pkg-config --cflags netcdf)"
LDFLAGS="$(pkg-config --libs netcdf) -lrt"

SRC="../src"

declare -A TYPES
TYPES[char]="NC_BYTE"
TYPES[short]="NC_SHORT"
TYPES[int]="NC_INT"
TYPES[double]="NC_DOUBLE"

echo "${(@k)TYPES}"
set -x
for K in ${(@k)TYPES}; do
	CPPFLAGS="-DDEBUG -DDATATYPE=$K -DNC_DATATYPE=${TYPES[$K]}"
	${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -o "../benchtool-$K" \
		${SRC}/types.c \
		${SRC}/report.c \
		${SRC}/timer.c \
		${SRC}/benchmark.c \
		${SRC}/main.c \
		${SRC}/options.c
done
set +x


exit 0


