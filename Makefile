BIN_DIR := ~/Documents/Projects/BBB/buildroot/output/host/bin
IP_ADDR := 10.10.0.40
COMPILER := arm-buildroot-linux-uclibcgnueabihf-gcc-7.3.0


all:
	$(BIN_DIR)/$(COMPILER) ex.c -o ex `$(BIN_DIR)/pkg-config --cflags --libs libevdev`

deploy:
	scp ex root@$(IP_ADDR):/root/
