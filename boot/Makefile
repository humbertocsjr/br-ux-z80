include ../build/conf.mk

BIN = ../output/floppy.boot


all: $(BIN)
	@echo >> /dev/null

clean:
	@rm -f $(BIN) >> /dev/null

../output/%.bin: %.s
	@$(ECHOAS)
	@$(ASZ80) -o $@ $^

../output/floppy.boot: ../output/bootflp.bin ../output/loaderflp.bin
	@$(ECHOAS)
	@dd if=../output/bootflp.bin of=$@ bs=512 count=1 status=none
	@dd if=../output/loaderflp.bin of=$@ bs=512 seek=1 count=5 status=none conv=notrunc