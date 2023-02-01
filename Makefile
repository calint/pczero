# built with g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0

BIN=bin/pczero.img
SRC=src/boot.cc src/tasks.cc
CC=g++ -std=c++2a -O3 -fno-rtti -fno-exceptions
# -Werror
CW=-Wfatal-errors -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wpadded -Winline\
 -pedantic -pedantic-errors -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization\
 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast\
 -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel\
 -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
CF=-nostdlib -m32 -fno-pie -Wl,--oformat,binary -Wl,-Ttext,0x7c00 -fno-stack-protector -fno-threadsafe-statics

# usb device
INSTALL_TO=/dev/sda

all:	clean build print display

build:
	@clear
	@$(CC) -o $(BIN) $(SRC) $(CF) $(CW)
	@chmod -x $(BIN)
	
print:
	@pwd
	@du -bh $(BIN) $(SRC)
	@echo
	@echo wc source
	@wc $(SRC)
	@echo
	@echo "wc source | gzip"
	@cat $(SRC)|gzip|wc
	@echo
	
clean:
	@rm -f $(BIN)

display:
	qemu-system-i386 -m 2M -drive file=$(BIN),format=raw
	 
install:
	sudo dd if=/dev/zero of=$(INSTALL_TO) count=1000&&sync
	sudo dd if=$(BIN) of=$(INSTALL_TO)&&sync

dispusb:
	sudo qemu-system-i386 -m 2M -drive file=$(INSTALL_TO),format=raw

readusb:
	sudo dd if=$(INSTALL_TO) count=2|hx|f 00000200
