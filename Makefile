include build/conf.mk

all clean: build/conf.mk bin include output
	@$(MAKE) pre_$@
	@$(MAKE) -C tools $@
	@$(MAKE) -C boot $@
	@$(MAKE) -C distro $@
	@$(MAKE) post_$@


pre_all:
	@echo "\033[0;32mBR\033[0;33m-\033[0;34mUX \033[0;31m${VERSION}${EDITION}\033[0m v$(VERSION).${SUB_VERSION} (B$(SUB_VERSION)${REVISION}) Build System"

pre_clean:
	@echo >> /dev/null

post_all:
	@echo Build completed

post_clean:
	@rm -Rf output/*

bin include output:
	@mkdir -p $@

build/conf.mk: configure build/base.mk build/version build/sub_version build/edition build/revision
	@echo "Run ./configure"
	@exit 1

run_sharp: all
	@$(ECHORUN) Sharp HB-8000 with DDX 3.0 FDC
	@openmsx -machine Sharp_HB-8000_1.1 -ext megaram -ext DDX_3.0 -diska distro/720.img

run_expert: all
	@$(ECHORUN) Gradiente Expert GPC-1 with Microsol CDX-2 FDC
	@openmsx -machine Gradiente_Expert_GPC-1 -ext megaram -ext Microsol_CDX-2 -diska distro/720.img

run_ddplus: all
	@$(ECHORUN) Gradiente Expert DD Plus
	@openmsx -machine Gradiente_Expert_DD_Plus -ext megaram -diska distro/720.img

run_minimal: all
	@$(ECHORUN) Mitsubishi ML-F110 with Mitsubishi ML-30FD
	@openmsx -machine Mitsubishi_ML-F110 -ext megaram -ext Mitsubishi_ML-30DC_ML-30FD -diska distro/720.img