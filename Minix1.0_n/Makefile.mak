
##################################
# Makefile for Minix             #
# make -f Makefile.mak           #
##################################

# Entry point of Minix
# It must be as same as 'KernelEntryPointPhyAddr' !!!
ENTRYPOINT	= 0x30400

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I ./BOOT/INCLUDE 
ASMLFLAGS	= -I  ./LOADER/INCLUDE
ASMKFLAGS	= -I ./KERNEL -f elf
CFLAGS		= -I ./INCLUDE -c -fno-builtin
LDFLAGS		= -s -Ttext $(ENTRYPOINT)

# This Program
MinixBOOT	= ./BUILD/BOOT.bin ./BUILD/LOADER.bin
MinixKERNEL	= ./BUILD/KERNEL.bin
OBJS		=  ./BUILD/KERNEL.o ./BUILD/main.o ./BUILD/klibx86.o ./BUILD/table.o \
  ./BUILD/protect.o ./BUILD/i8259.o ./BUILD/start.o ./BUILD/proc.o ./BUILD/clock.o \
  ./BUILD/sendrec.o ./BUILD/system.o  ./BUILD/tty.o  ./BUILD/printk.o  ./BUILD/abs.o \
  ./BUILD/atoi.o ./BUILD/ctype.o ./BUILD/atol.o ./BUILD/bcopy.o ./BUILD/itoa.o \
  ./BUILD/strcat.o ./BUILD/strcmp.o ./BUILD/strcpy.o ./BUILD/strlen.o ./BUILD/strncat.o \
  ./BUILD/strncmp.o ./BUILD/strncpy.o ./BUILD/memory.o  

# All Phony Targets
.PHONY : everything clean

# Default starting position
everything :cleanAll $(MinixBOOT) $(MinixKERNEL) clean

cleanAll :
	rm -f $(MinixBOOT)
	rm -f $(MinixKERNEL)
	rm -f $(OBJS)

clean:
	rm -f $(OBJS)

./BUILD/BOOT.bin : ./BOOT/BOOT.asm
	$(ASM) $(ASMBFLAGS) -o $@ $<

./BUILD/LOADER.bin : ./LOADER/LOADER.asm
	$(ASM) $(ASMLFLAGS) -o $@ $<

./BUILD/KERNEL.o : ./KERNEL/x86.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

./BUILD/klibx86.o : ./KERNEL/klibx86.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

./BUILD/sendrec.o : ./LIB/sendrec.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

./BUILD/main.o : ./KERNEL/main.c ./H/const.h ./H/type.h ./KERNEL/const.h ./KERNEL/type.h \
  ./KERNEL/glo.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/table.o : ./KERNEL/table.c ./H/const.h ./H/type.h ./KERNEL/const.h ./KERNEL/type.h \
  ./KERNEL/glo.h ./KERNEL/proc.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/protect.o: ./KERNEL/protect.c ./H/const.h ./H/type.h ./KERNEL/const.h \
  ./KERNEL/type.h ./KERNEL/glo.h ./KERNEL/proc.h ./KERNEL/protect.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/i8259.o: ./KERNEL/i8259.c ./KERNEL/const.h ./KERNEL/type.h ./KERNEL/glo.h \
  ./KERNEL/protect.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/start.o: ./KERNEL/start.c ./H/const.h ./H/type.h ./KERNEL/const.h ./KERNEL/type.h \
  ./KERNEL/glo.h ./KERNEL/proc.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/proc.o: ./KERNEL/proc.c ./H/const.h ./H/type.h ./KERNEL/const.h ./KERNEL/type.h \
  ./KERNEL/protect.h ./KERNEL/proc.h ./KERNEL/glo.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/clock.o: ./KERNEL/clock.c ./H/const.h ./H/type.h ./H/com.h ./KERNEL/const.h \
  ./KERNEL/type.h ./KERNEL/protect.h ./KERNEL/proc.h ./KERNEL/glo.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/system.o: ./KERNEL/system.c ./H/const.h ./H/type.h ./H/com.h ./KERNEL/const.h \
  ./KERNEL/type.h ./KERNEL/protect.h ./KERNEL/proc.h ./KERNEL/glo.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/memory.o: ./KERNEL/memory.c ./H/const.h ./H/type.h ./H/callnr.h ./H/com.h ./H/error.h \
  ./KERNEL/const.h ./KERNEL/type.h ./KERNEL/proc.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/printk.o: ./LIB/printk.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/abs.o: ./LIB/abs.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/atoi.o: ./LIB/atoi.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/ctype.o: ./LIB/ctype.c ./INCLUDE/ctype.h
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/atol.o: ./LIB/atol.c ./INCLUDE/ctype.h
	$(CC) $(CFLAGS) -o $@ $< 

./BUILD/bcopy.o: ./LIB/bcopy.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/itoa.o: ./LIB/itoa.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strcat.o: ./LIB/strcat.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strcmp.o: ./LIB/strcmp.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strcpy.o: ./LIB/strcpy.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strlen.o: ./LIB/strlen.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strncat.o: ./LIB/strncat.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strncmp.o: ./LIB/strncmp.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/strncpy.o: ./LIB/strncpy.c
	$(CC) $(CFLAGS) -o $@ $<

./BUILD/tty.o: ./KERNEL/tty.c ./H/const.h ./H/type.h ./H/com.h ./H/signal.h ./H/error.h \
  ./H/callnr.h ./KERNEL/keymap.h ./KERNEL/keyboard.h ./KERNEL/const.h \
  ./KERNEL/type.h ./KERNEL/protect.h ./KERNEL/proc.h ./KERNEL/glo.h
	$(CC) $(CFLAGS) -o $@ $<

$(MinixKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(MinixKERNEL) $(OBJS)
