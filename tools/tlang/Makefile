include ../../build/conf.mk

BIN = ../../bin/tlang

all: $(BIN)
	@echo >> /dev/null

clean:
	@rm -f $(BIN) >> /dev/null

../../bin/%: %.c
	@$(ECHOCC)
	@$(CC) $(CCFLAGS) -g -o $@ $^
	@chmod +x $@

test: all
	../../bin/tlang test.t test.tok