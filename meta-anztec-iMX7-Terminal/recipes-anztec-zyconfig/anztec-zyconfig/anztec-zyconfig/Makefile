# makefile to generate the library

INC_DIRS=-I${includedir}/libusb-1.0 -lc

#CC=gcc
#CPP=g++
AR=ar

#CFLAGS=$(INC_DIRS)
#CPP_FLAGS=-std=c++11

OBJ_DIR=./

OBJ1 = usb.o protocol.o services.o services_sc.o debug.o sysdata.o
OBJ2 = logfile.o configfile.o
OBJS = $(patsubst %,$(OBJ_DIR)/%,$(OBJ1)) $(patsubst %,$(OBJ_DIR)/%,$(OBJ2))

# output file needs to start with lib in order to be found by dependant projects
libzylib.a: $(OBJS) Makefile
	$(AR) rs $@ $^

.PHONY: all
all :  toucher libzylib.a

.PHONY: toucher
toucher:
	touch *.c *.cpp

$(OBJ_DIR)/%.o: %.cpp ./*.h Makefile
	$(CPP) -c -o $@ $< $(INC_DIRS) $(CFLAGS)

$(OBJ_DIR)/%.o: %.c ./*.h Makefile
	$(CC) -c -o $@ $< $(INC_DIRS) $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o ./libzylib.a
