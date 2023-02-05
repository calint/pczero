# built with g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0

BIN=bin/pczero.img
# source files to be compiled
SRC=src/_osca.S src/main.cc
# all files with source, used in 'print'
FILES=$(SRC)
FILES+=src/lib.h src/lib2d.h

CC=g++ -std=c++2a -Wfatal-errors
CW=-pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
CW+=-Wpadded
#CW+=-Winline
CF=-O3 -m32 -fno-builtin -nostdlib -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
CF+=-fconserve-stack # try to inhibit excessive use of stack by optimizer
CF+=-fanalyzer
CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.
LF=-Wl,--oformat=binary,-Ttext=0x7c00

#CC=clang++ -Wfatal-errors
#CW=-pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
#CW+=-Wpadded
#CW+=-Winline
#CW+=-Wno-unused-private-field # disable warning regarding padding
#CF=-O3 -fno-builtin -nostdlib -m32 -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
#LF=-Wl,--oformat=binary,-Ttext=0x7c00

# usb device
INSTALL_TO=/dev/sda

all:	clean build print display

build:
	@clear
	@$(CC) -o $(BIN) $(SRC) $(CF) $(LF) $(CW)
	@chmod -x $(BIN)
	
print:
	@pwd
	@du -b $(BIN) $(SRC)
	@echo
	@echo wc source
	@wc $(FILES)
	@echo
	@echo "wc source | gzip"
	@cat $(FILES)|gzip|wc
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
