# built with g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
# make file with linker file

BIN=pczero.img
SRC=../src/boot.cc ../src/tasks.cc
CC=g++ -std=c++2a -nostdlib -O3 -m32 -fno-pie -fno-stack-protector -fno-threadsafe-statics
CW=-Wfatal-errors -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow -Wpadded -Winline\
 -pedantic -pedantic-errors -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization\
 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast\
 -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel\
 -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
CF=-T link.ld -Wl,--oformat,binary -Wl,-Ttext,0x7c00

# usb device
INSTALL_TO=/dev/sda

all:	clean build print display

build:
	@clear
	@$(CC) -c ../src/boot.cc $(CF) $(CW)
	@$(CC) -c ../src/tasks.cc $(CF) $(CW)
	@$(CC) -o $(BIN) tasks.o boot.o $(CF) $(CW)
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
	sudo dd if=$(BIN) of=$(INSTALL_TO)&&sync

readusb:
	sudo dd if=$(INSTALL_TO) count=2|hx|f 00000200
