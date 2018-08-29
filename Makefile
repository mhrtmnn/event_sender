# Platform specific
BIN_DIR := ~/Documents/Projects/BBB/buildroot/output/host/bin
IP_ADDR := 10.10.0.40
COMPILER := arm-buildroot-linux-uclibcgnueabihf-gcc-7.3.0

# Project specific
PROG := example
LIB_LIST := libevdev


all:
	$(BIN_DIR)/$(COMPILER) $(PROG).c -o $(PROG) `$(BIN_DIR)/pkg-config --cflags --libs $(LIB_LIST)`

clean:
	rm -rf $(PROG)

deploy: all
	scp $(PROG) root@$(IP_ADDR):/root/
