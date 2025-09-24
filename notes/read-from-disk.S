cld # clear direction (forward)
# read 127 sectors * 512B = 65024 bytes
#  assumes load from USB. floppy does not support that many
#   sectors.
# command 2 (%ah), number of sectors to read (%al)
mov $0x027f,%ax
# from cylinder 0 (%ch), sector 2 (%cl)
#  sector starts from 1, not 0
mov $0x0002,%cx
xor %dh,%dh # head 0
# %dl, drive number, is unchanged
mov $0x07e0,%bx # to es:bx=7e00:0000
mov %bx,%es     #
xor %bx,%bx     #
int $0x13       # read disk
