PART   = LM3S9B96
ROOT   = ../../StellarisWare
PREFIX = arm-none-eabi
OPT    = -O2 -g

CC = ${PREFIX}-gcc

AFLAGS = -mthumb              \
         -mcpu=cortex-m3      \
         -MD                  \
         -I$(ROOT)

CFLAGS = -mthumb             \
         -mcpu=cortex-m3     \
         $(OPT)              \
         -ffunction-sections \
         -fdata-sections     \
         -MD                 \
         -std=c99            \
         -Wall               \
         -pedantic           \
         -DPART_${PART}      \
	     -I$(ROOT)           \
	     -DTARGET_IS_TEMPEST_RB1 \
         -c

AR=${PREFIX}-ar
LD=${PREFIX}-ld
OBJDUMP=${PREFIX}-objdump
OBJCOPY=${PREFIX}-objcopy
LDFLAGS=--gc-sections 
LIBGCC=${shell ${CC} ${CFLAGS} -print-file-name=libgcc.a}
LIBC=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}
LIBM=${shell ${CC} ${CFLAGS} -print-file-name=libm.a}
LIBI=${shell ${CC} ${CFLAGS} -print-file-name=libiberty.a}

Debug: all
cleanDebug: clean

Release: all
cleanRelease: clean

#
# Where to find source files that do not live in this directory.
#
VPATH=${ROOT}/utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=${ROOT}

TARGET=rollo

LDFILE=${TARGET}.ld

debug: gcc/${TARGET}.axf
	arm-none-eabi-gdbtui -x init.gdb gcc/${TARGET}.axf

debugc: gcc/${TARGET}.axf
	arm-none-eabi-gdb --command=init.gdb gcc/${TARGET}.axf	

serial:
	picocom -b 115200 -f n -d 8 -p n /dev/cu.usbserial-0B010169B

all: gcc
all: gcc/${TARGET}.axf
all: gcc/${TARGET}.lst

clean:
	@rm -rf gcc ${wildcard *~}

gcc:
	@mkdir gcc
		
ANIM_OBJ  = startup_gcc.o rollo.o uartstdio.o flash_pb.c 
OBJ = $(patsubst %.o,gcc/%.o,$(ANIM_OBJ))

gcc/${TARGET}.axf: $(OBJ)
gcc/${TARGET}.axf: ${ROOT}/driverlib/gcc/libdriver.a
gcc/${TARGET}.axf: ${TARGET}.ld

LDFILE=${TARGET}.ld
ENTRY=ResetISR

DEBUG=1

ifdef DEBUG
CFLAGS+=-g -DDEBUG
endif

gcc/%.o: %.c
	@echo "  compile ${<}"
	@${CC} ${CFLAGS} -Dgcc -o ${@} ${<}

gcc/%.o: %.S
	@echo "  assemble ${<}"
	@${CC} ${AFLAGS} -Dgcc -o ${@} -c ${<}

gcc/%.axf:
	@echo "  link ${@}"
	@${LD} --entry ${ENTRY} -T${LDFILE}                  \
		  ${LDFLAGS} -o ${@} $(filter %.o %.a, ${^})    \
		  '${LIBC}' '${LIBGCC}'
	@echo "  binary ${@:.axf=.bin}"
	@${OBJCOPY} -O binary ${@} ${@:.axf=.bin}

gcc/${TARGET}.lst: gcc/${TARGET}.axf
	@echo "  listing $@"
	@$(OBJDUMP) -h -S $< > $@


ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif