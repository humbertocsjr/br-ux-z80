include ../build/conf.mk

IMG = 720.img

all: $(IMG)
	@echo >> /dev/null

clean:
	@rm -f $(IMG) >> /dev/null

720.img: $(wildcard ../output/*) Makefile
	@$(ECHOIMG)
	@dd if=/dev/zero of=$@ bs=1024 count=$(patsubst %.img,%,$@) status=none
	@$(GENFS) $@ ../output/floppy.boot $(patsubst %.img,%,$@) ../output/floppy.boot /boot/bootsect