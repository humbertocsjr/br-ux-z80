include ../../build/conf.mk

BIN = ../../bin/gasm80

all: $(BIN)
	@echo >> /dev/null

clean:
	@rm -f $(BIN) >> /dev/null

../../bin/%: %.c
	@$(ECHOCC)
	@$(CC) $(CCFLAGS) -o $@ $^
	@chmod +x $@