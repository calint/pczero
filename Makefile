# tools used:
#   as       GNU assembler (GNU Binutils for Ubuntu) 2.41
#   g++      (Ubuntu 13.2.0-4ubuntu3) 13.2.0
#   clang++  Ubuntu clang version 16.0.6
#   ld       GNU ld (GNU Binutils for Ubuntu) 2.41

# resulting bootable image
IMAGE=pczero.img

# install on usb device, used in 'install'
INSTALL_TO=/dev/sda

# source files, used in 'print'
SRC_FILES=src/osca.S src/osca.h src/kernel.h src/lib.h src/libge.h src/game.h src/main.cc

# as
AF=-march=i386+387 --32

# g++
#CC=g++ -std=c++2b # c++ 23
#CF=-Os -m32 -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-threadsafe-statics
#CF+=-Wfatal-errors # stop at first error
#CF+=-fanalyzer
#CW+=-Werror # warnings are errors
#CW=-pedantic -pedantic-errors -Wall -Wextra -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
#CW+=-Wpadded # warn when complier pads a data structure
#CW+=-Wfloat-conversion 
#CW+=-Wno-array-bounds # allow pointer shenanigans
#CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
#CW+=-Wno-unused-function # allow for debugging
#CW+=-Wno-unused-variable # allow for debugging
#CW+=-Wno-unused-parameter # allow for debugging

# clang++
CC=clang++ -std=c++20
CF=-Os -m32 -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-threadsafe-statics
CF+=-Wfatal-errors # stop at first error
CW=-Werror # warnings are errors
CW+=-Weverything
CW+=-Wno-c++98-compat # ignore c++98 compatability issues
CW+=-Wno-c++98-c++11-compat-binary-literal # allow 0xb..... literals
CW+=-Wno-global-constructors # global constructors ok here
CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
CW+=-Wno-weak-vtables # allow for source in include files
CW+=-Wno-unused-function # allow for debugging
CW+=-Wno-unused-parameter # allow for debugging
CW+=-Wno-unused-variable # allow for debugging
CW+=-Wno-unused-private-field # allow for padding
CW+=-Wno-unsafe-buffer-usage # allow pointer shenanigans

# ld
LF=-Tlink.ld -melf_i386 -nostdlib

all: clean build print display

build:
	@clear
	@mkdir -p bin/src
	as -c src/osca.S -o bin/src/osca.o $(AF)
	@echo
	$(CC) -c src/main.cc -o bin/src/main.o $(CF) $(CW)
	@echo
	ld -o $(IMAGE) $(LF) && chmod -x $(IMAGE)
	@echo
	
print:
	@echo sizes
	@du -b $(IMAGE) $(SRC_FILES)
	@echo
	@echo wc source
	@wc $(SRC_FILES)
	@echo
	@echo "wc source | gzip"
	@cat $(SRC_FILES) | gzip | wc
	@echo
	@echo -n "assembler calls: " && objdump -d bin/src/main.o | grep call | wc -l
	@echo
	@ls -la $(IMAGE)
	@echo
	@if [ $(shell stat -c "%s" $(IMAGE)) -ge 65536 ]; then echo '!!!'; echo '!!! IMAGE FILE GREATER THAN OSCA LOADS'; echo '!!!'; echo; fi
	
clean:
	@rm -fr bin/

display:
	qemu-system-i386 -display gtk,zoom-to-fit=on -m 1M -drive file=$(IMAGE),format=raw
	@echo

install:
	sudo dd if=/dev/zero of=$(INSTALL_TO) count=1000 && sync
	sudo dd if=$(IMAGE) of=$(INSTALL_TO) && sync

dispusb:
	sudo qemu-system-i386 -display gtk,zoom-to-fit=on -m 1M -drive file=$(INSTALL_TO),format=raw

readusb:
	sudo dd if=$(INSTALL_TO) count=2 | hx | f 00000200
