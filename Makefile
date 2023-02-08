# GNU assembler (GNU Binutils for Ubuntu) 2.39
# g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
# GNU ld (GNU Binutils for Ubuntu) 2.39

# resulting bootable image
IMAGE=pczero.img
# source files to be compiled
SRC=src/osca.S src/main.cc
# all files with source, used in 'print'
FILES=$(SRC)
FILES+=src/osca.h src/lib.h src/lib2d.h src/libge.h

# GNU assembler (GNU Binutils for Ubuntu) 2.39
AF=--march=i386 --32

# g++ (Ubuntu 12.2.0-3ubuntu1) 12.2.0
#CC=g++ -std=c++2b # c++ 23
#CW=-pedantic -pedantic-errors -Wall -Wextra -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Weffc++ -Wfloat-equal
#CW+=-Werror # warnings are errors
#CW+=-Wpadded # warn when complier pads a data structure
##CW+=-Winline # don't warn about non-inlined functions
#CW+=-Wno-analyzer-malloc-leak
#CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
#CW+=-Wno-unused-function # allow for debugging
#CW+=-Wno-unused-variable # allow for debugging
#CW+=-Wno-unused-parameter # allow for debugging
#CF=-O3 -m32 -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
#CF+=-Wfatal-errors # stop at first error
#CF+=-fconserve-stack # try to inhibit excessive use of stack by optimizer
#CF+=-fanalyzer
#CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.

#Ubuntu clang version 15.0.6
#Target: x86_64-pc-linux-gnu
CC=clang++ -std=c++2b # c++ 23
CW=-Weverything
CW+=-Werror # warnings are errors
CW+=-Wno-c++98-compat
CW+=-Wno-c++98-c++11-compat-binary-literal
CW+=-Wno-c++98-compat-bind-to-temporary-copy
CW+=-Wno-global-constructors # global constructors ok here
CW+=-Wno-float-equal # allow float comparison since it is bitwise relevant
CW+=-Wno-unused-parameter # allow for debugging
CW+=-Wno-unused-function # allow for debugging
CW+=-Wno-weak-vtables # allow for source in include files
CF=-Os -m32 -nostdlib -fno-builtin -fno-pie -fno-rtti -fno-exceptions -fno-rtti -fno-threadsafe-statics
CF+=-Wfatal-errors # stop at first error
CF+=-fno-stack-protector # disable error: undefined reference to '__stack_chk_fail'.

# GNU ld (GNU Binutils for Ubuntu) 2.39
LF=-Tlink.ld -melf_i386 -nostdlib

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
#	clang-tidy src/main.cc
#	@echo
	ld -o $(IMAGE) $(LF) && chmod -x $(IMAGE)
	@echo
	
print:
	@echo sizes
	@du -b $(IMAGE) $(FILES)
	@echo
	@echo wc source
	@wc $(FILES)
	@echo
	@echo "wc source | gzip"
	@cat $(FILES)|gzip|wc
	@echo
	@echo -n "calls: " && objdump -d bin/src/main.o | grep call | wc -l
	@echo
	@if [ $(shell stat -c "%s" $(IMAGE)) -ge 66048 ]; then echo '!!!';echo '!!! IMAGE FILE GREATER THAN OSCA LOADS';echo '!!!';echo; fi
	
clean:
	@rm -fr bin/*

display:
	qemu-system-i386 -m 2M -drive file=$(IMAGE),format=raw
	@echo

install:
	sudo dd if=/dev/zero of=$(INSTALL_TO) count=1000&&sync
	sudo dd if=$(IMAGE) of=$(INSTALL_TO)&&sync

dispusb:
	sudo qemu-system-i386 -m 2M -drive file=$(INSTALL_TO),format=raw

readusb:
	sudo dd if=$(INSTALL_TO) count=2|hx|f 00000200
