# tools used:
#   as       2.45.0
#   g++      15.2.1
#   clang++  20.1.8
#   ld       2.45.0
#   strip    2.45.0
#
# emulated with:
#   qemu-system-i386  10.1.0
#   VirtualBox        7.0
#
# tested on hardware:
#   asus zenbook UX305CA
#   dell inspiron 1545
#

# resulting bootable image
IMAGE=pczero.img

# install on usb device, used in 'install'
INSTALL_TO=/dev/sda

# source files, used in 'print'
SRC_FILES=src/osca.S src/osca.hpp src/lib.hpp src/kernel.hpp src/libge.hpp src/game.hpp src/main.cpp

# as
AF=-march=i386+387 --32 -W -fatal-warnings

# g++
#CC=g++ -std=c++23
#CF=-g -Os -m32 -march=i386 -ffreestanding -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-stack-protector
#CF+=-Wfatal-errors # stop at first error
#CF+=-fanalyzer
#CF+=-Werror # warnings are errors
#CW=-pedantic -pedantic-errors -Wall -Wextra -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
#CW+=-Wpadded # warn when complier pads a data structure
#CW+=-Wno-effc++ # allow pointer data members
#CW+=-Wno-array-bounds # allow pointer shenanigans
#CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
#CW+=-Wno-cast-qual # allow cast from const void* to void*
#CW+=-Wno-unused-function # allow for debugging
#CW+=-Wno-unused-variable # allow for debugging
#CW+=-Wno-unused-parameter # allow for debugging

# clang++
CC=clang++ -std=c++23
CF=-g -Os -m32 -march=i386 -ffreestanding -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-stack-protector
CF+=-Wfatal-errors # stop at first error
CF+=-Werror # warnings are errors
CW=-Weverything # all warnings
CW+=-Wno-c++98-compat # ignore c++98 compatability issues
CW+=-Wno-c++98-compat-pedantic # alow 'long long'
CW+=-Wno-c++98-c++11-compat-binary-literal # allow 0xb..... literals
CW+=-Wno-c++11-narrowing # allow
CW+=-Wno-global-constructors # global variables allowed (initiated at init)
CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
CW+=-Wno-weak-vtables # allow for source in include files
CW+=-Wno-unsafe-buffer-usage # allow pointer shenanigans
CW+=-Wno-unused # allow for debugging

# ld
LF=-Tlink.ld -melf_i386 -nostdlib

all: clean build print display

build:
	@clear
	@mkdir -p bin/src
	as -c src/osca.S -o bin/src/osca.o $(AF)
	@echo
	$(CC) -c src/main.cpp -o bin/src/main.o $(CF) $(CW)
	@echo
	cp bin/src/main.o bin/src/main_debug.o
	strip --strip-debug bin/src/main.o
	objcopy --remove-section=.comment bin/src/main.o bin/src/main.o
	@echo
	ld -o $(IMAGE) $(LF) -Map bin/pczero.map && chmod -x $(IMAGE)
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
	@if [ $(shell stat -c "%s" $(IMAGE)) -ge 32768 ]; then echo -e "\e[31m!!!"; echo "!!! IMAGE FILE GREATER THAN OSCA LOADS"; echo -e "!!!\e[0m"; echo; fi
	
clean:
	@rm -fr bin/

display:
	qemu-system-i386 -display gtk,zoom-to-fit=on -m 1M -drive file=$(IMAGE),format=raw
	@echo

install:
	sudo dd if=/dev/zero of=$(INSTALL_TO) bs=512 count=2880 && sync
	sudo dd if=pczero.img of=$(INSTALL_TO) && sync

dispusb:
	sudo qemu-system-i386 -display gtk,zoom-to-fit=on -m 1M -drive file=$(INSTALL_TO),format=raw

readusb:
	sudo dd if=$(INSTALL_TO) count=2 | hx | f 00000200
