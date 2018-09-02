# Platform specific
BIN_DIR := ~/Documents/Projects/BBB/buildroot/output/host/bin
IP_ADDR := 10.10.0.40
COMPILER := arm-buildroot-linux-uclibcgnueabihf-gcc-7.3.0

# Project specific
PROG := event_sender
SRC_LIST := $(PROG).c protobuf_handling.c network_handling.c
PROTO_NAME := nunchuk_update
LIB_LIST := libevdev libprotobuf-c
CFLAGS := -Wall -g


all: proto
	$(BIN_DIR)/$(COMPILER) $(SRC_LIST) $(PROTO_NAME).pb-c.c $(CFLAGS) -o $(PROG) `$(BIN_DIR)/pkg-config --cflags --libs $(LIB_LIST)`

proto:
	# The --c_out flag instructs the protoc compiler to use the protobuf-c plugin (https://github.com/protobuf-c/protobuf-c)
	$(BIN_DIR)/protoc --c_out=. $(PROTO_NAME).proto

clean:
	rm -rf $(PROG) $(PROTO_NAME).pb-c.c $(PROTO_NAME).pb-c.h

deploy: all
	scp $(PROG) root@$(IP_ADDR):/root/
