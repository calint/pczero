# GNU assembler (GNU Binutils for Ubuntu) 2.39
# g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
# GNU ld (GNU Binutils for Ubuntu) 2.39

# resulting bootable image
IMAGE=pczero.img
# source files to be compiled
SRC=src/osca.S src/main.cc
# all files with source, used in 'print'
FILES=$(SRC)
FILES+=src/lib.h src/lib2d.h

# GNU assembler (GNU Binutils for Ubuntu) 2.39
AF=--march=i386 --32

# g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
CC=g++ -std=c++2a
CW=-pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
CW+=-Wfatal-errors
CW+=-Wpadded
#CW+=-Winline
CF=-O3 -m32 -fno-builtin -nostdlib -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
CF+=-fconserve-stack # try to inhibit excessive use of stack by optimizer
CF+=-fanalyzer
CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.

#Ubuntu clang version 15.0.6
#Target: x86_64-pc-linux-gnu
#CC=clang++ -Wfatal-errors
#CW=-pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
#CW+=-Wpadded
##CW+=-Winline
#CW+=-Wno-unused-private-field # disable warning regarding padding
#CF=-O3 -fno-builtin -nostdlib -m32 -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
#CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.
#CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.

# GNU ld (GNU Binutils for Ubuntu) 2.39
LF=-T ../link.ld -melf_i386 -nostdlib

# usb device
INSTALL_TO=/dev/sda

all:	clean build print display

build:
	@clear
	@mkdir -p bin/src
	as -c src/osca.S -o bin/src/osca.o $(AF)
	@echo
	$(CC) -c src/main.cc -o bin/src/main.o $(CF) $(CW)
	@echo
	@echo
	cd bin && ld -o $(IMAGE) $(LF) && chmod -x $(IMAGE)
	@echo
	
print:
	@echo sizes
	@du -b bin/$(IMAGE) $(FILES)
	@echo
	@echo wc source
	@wc $(FILES)
	@echo
	@echo "wc source | gzip"
	@cat $(FILES)|gzip|wc
	@echo
	
clean:
	@rm -fr bin/*

display:
	qemu-system-i386 -m 2M -drive file=bin/$(IMAGE),format=raw
	 
install:
	sudo dd if=/dev/zero of=$(INSTALL_TO) count=1000&&sync
	sudo dd if=bin/$(IMAGE) of=$(INSTALL_TO)&&sync

dispusb:
	sudo qemu-system-i386 -m 2M -drive file=$(INSTALL_TO),format=raw

readusb:
	sudo dd if=$(INSTALL_TO) count=2|hx|f 00000200
