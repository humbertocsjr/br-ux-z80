include ../build/conf.mk

IMG = 720.img

all: $(IMG)
	@echo >> /dev/null

clean:
	@rm -f $(IMG) >> /dev/null

720.img: ../bin/genfs $(wildcard ../output/*) Makefile
	@$(ECHOIMG)
	@dd if=/dev/zero of=$@ bs=1024 count=$(patsubst %.img,%,$@) status=none
	@../bin/genfs -i $@ -s $(patsubst %.img,%,$@) -b ../output/floppy.boot -a ../output/floppy.boot /boot/boot