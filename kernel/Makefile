include ../build/conf.mk

BIN = ../output/br-ux


all: $(BIN)
	@echo >> /dev/null

clean:
	@rm -f $(BIN) >> /dev/null

../output/%: %.s $(wildcard modules/*.s) $(wildcard ../include/*.s)
	@$(ECHOAS)
	@$(ASZ80) -o $@ $<