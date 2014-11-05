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
	     -DUART_BUFFERED \
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
#VPATH += ${ROOT}/third_party/lwip-1.3.2/apps/httpserver_raw
#VPATH += ${ROOT}/utils

VPATH+=${ROOT}/drivers
VPATH+=${ROOT}/third_party/uip-1.0/apps/dhcpc
VPATH+=${ROOT}/third_party/uip-1.0/uip
VPATH+=${ROOT}/utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=${ROOT}
IPATH+=${ROOT}/third_party/uip-1.0/apps
IPATH+=${ROOT}/third_party/uip-1.0/uip
IPATH+=${ROOT}/third_party/uip-1.0

#IPATH+=.
#IPATH+=${ROOT}/third_party/lwip-1.3.2/apps
#IPATH+=${ROOT}/third_party/lwip-1.3.2/ports/stellaris/include
#IPATH+=${ROOT}/third_party/lwip-1.3.2/src/include
#IPATH+=${ROOT}/third_party/lwip-1.3.2/src/include/ipv4
#IPATH+=${ROOT}/third_party

AFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}
CFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}

TARGET=rollo

LDFILE=${TARGET}.ld

debug: gcc/${TARGET}.axf
	arm-none-eabi-gdbtui -x init.gdb gcc/${TARGET}.axf

debugc: gcc/${TARGET}.axf
	arm-none-eabi-gdb --command=init.gdb gcc/${TARGET}.axf	

serial:
	picocom -b 115200 -f n -d 8 -p n /dev/cu.usbserial-0B010169B

io_fsdata.h: ${wildcard fs/*}
	${ROOT}/tools/makefsfile/makefsfile.exe -i fs -o io_fsdata.h -r -h

all: gcc
all: gcc/${TARGET}.axf
all: gcc/${TARGET}.lst

clean:
	@rm -rf gcc ${wildcard *~}

gcc:
	@mkdir gcc

OBJ  = startup_gcc.o uartstdio.o uip.o uip_arp.o psock.o uip_timer.o dhcpc.o httpd-uri-cmd.o
OBJ += ustdlib.o enet_uip.o httpd.o httpd-fs.o http-strings.o httpd-cgi.o rollo.o
#OBJ  = startup_gcc.o main.o rollo.o lmi_fs.o uartstdio.o
#OBJ += httpd.o  locator.o lwiplib.o ustdlib.o
GOBJ = $(patsubst %.o,gcc/%.o,$(OBJ))

gcc/${TARGET}.axf: $(GOBJ)
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
