IMG=install.img
BIN=pc.img
SRC=pc.cc std.cc main.cc
CC=g++ -m32 -O0 -fno-pie
CW=-Wfatal-errors -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow -Wpadded -Winline
CF=-nostdlib -Wl,--oformat,binary -Wl,-Ttext,0x7c00 -fno-stack-protector

# usb device
INSTALL_TO=/dev/sda

all:	clean build print display

build:
	@clear
	@$(CC) -o $(BIN) $(SRC) $(CF) $(CW)
	@chmod -x $(BIN)
	@cp $(BIN) $(IMG)
	@date>>$(IMG)
	@ls -l Makefile>>$(IMG)
	@ls -l $(SRC)>>$(IMG)
	@cat Makefile>>$(IMG)
	@cat $(SRC)>>$(IMG)
	
print:
	@pwd
	@du -bh $(IMG) $(BIN) $(SRC)
	@echo
	@echo wc source
	@wc $(SRC)
	@echo
	@echo "wc source | gzip"
	@cat $(SRC)|gzip|wc
	@echo
	
clean:
	@rm -f $(IMG) $(BIN)

display:
	@qemu-system-i386 -m 1 -drive file=$(IMG),format=raw
	 
install:
	sudo dd if=$(IMG) of=$(INSTALL_TO)&&sync

readusb:
	sudo dd if=$(INSTALL_TO) count=2|hx|f 00000200

