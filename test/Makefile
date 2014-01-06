# Makefile fuer Komponententest by Martin Ongsiek

# Namen der Unterordner
BIN_FOLDER = bin/Debug
OBJ_FOLDER = obj/Debug
DEP_FOLDER = dep

# Name of the programm. Dieser wird aus dem Ordnernamen erzeugt.
PRG        = $(BIN_FOLDER)/$(notdir $(shell pwd))

# OBJ = $(patsubst %.o, %.c,$(wildcard *.c))
OBJ = main.o

# Put the OBJ in .obj folder
OBJ_2    = $(patsubst %.o,$(OBJ_FOLDER)/%.o,$(OBJ))

CFLAGS   = -g -std=c99 -Wextra -fprofile-arcs -ftest-coverage -I.
LDFLAGS  = -fprofile-arcs -ftest-coverage -Wl,-Map,$(PRG).map

# Zus�tzliche Bibliotheken
LIBS     = -lm
CC       = gcc

all: $(OBJ_FOLDER) $(DEP_FOLDER) $(BIN_FOLDER) $(PRG).exe toolchain.txt 
#toolchain.txt
#gcov -f -u -o obj\Debug main.c
$(PRG).exe: $(OBJ_2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Es gibt leider sehr merkwürdige Probleme.
toolchain.txt:
	echo "Gnu C Compiler:" > $@
	echo "---------------" >> $@
	which $(CC) >> $@
	$(CC) --version >> $@
	echo "Gnu Debugger:" >> $@
	echo "-------------" >> $@
	which gdb.exe >> $@
	gdb.exe --version >> $@
	echo "Coverage Analyse:" >> $@
	echo "-----------------" >> $@
	which gcov.exe >> $@
	gcov.exe --version >> $@
	echo make $(MAKE_VERSION) >> $@

clean:
	rm -rf $(DEP_FOLDER) $(OBJ_FOLDER) toolchain.txt

default: all
Debug: all
Release: all
cleandefault: clean
cleanDebug: clean
cleanRelease: clean

# Make folders
$(OBJ_FOLDER):
	mkdir -p $(OBJ_FOLDER)
$(DEP_FOLDER):
	mkdir -p $(DEP_FOLDER)
$(BIN_FOLDER):
	mkdir -p $(BIN_FOLDER)

#compile
$(OBJ_FOLDER)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_FOLDER)/$<.d -c $< -o $@

DEPS := $(wildcard $(DEP_FOLDER)/*.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
