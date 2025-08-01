PSP2_EXE_STRIPPED := scummvm_stripped$(EXEEXT)

$(PSP2_EXE_STRIPPED): $(EXECUTABLE)
	$(STRIP) --strip-debug $< -o $@

psp2vpk: $(PSP2_EXE_STRIPPED) $(PLUGINS)
	rm -rf psp2pkg
	rm -f $(EXECUTABLE).vpk
	mkdir -p psp2pkg/sce_sys/livearea/contents
	mkdir -p psp2pkg/data/
	mkdir -p psp2pkg/doc/
	vita-elf-create $(PSP2_EXE_STRIPPED) $(EXECUTABLE).velf
# 	Disable ASLR with -na to support profiling, also it can slow things down
	vita-make-fself -na -s -c $(EXECUTABLE).velf psp2pkg/eboot.bin
ifdef DYNAMIC_MODULES
	# Use psp2rela to convert the main binary to static, this allows plugins to use functions from it without any relocation
	# TODO: Use psp2rela -fetch_base flag instead of using objdump when the change is widespread
	set -e ;\
	textaddr=$$($(OBJDUMP) -p $(EXECUTABLE) | awk '/ LOAD / { vaddr=$$5; getline; if ($$6 == "r-x") print vaddr; }') ;\
	dataaddr=$$($(OBJDUMP) -p $(EXECUTABLE) | awk '/ LOAD / { vaddr=$$5; getline; if ($$6 == "rw-") print vaddr; }') ;\
	psp2rela -src=psp2pkg/eboot.bin -dst=psp2pkg/eboot.bin -static_mode -text_addr=$$textaddr -data_addr=$$dataaddr
endif
	vita-mksfoex -s TITLE_ID=VSCU00001 -d ATTRIBUTE2=12 "$(EXECUTABLE)" psp2pkg/sce_sys/param.sfo
	cp $(srcdir)/dists/psp2/icon0.png psp2pkg/sce_sys/
	cp $(srcdir)/dists/psp2/template.xml psp2pkg/sce_sys/livearea/contents/
	cp $(srcdir)/dists/psp2/bg.png psp2pkg/sce_sys/livearea/contents/
	cp $(srcdir)/dists/psp2/startup.png psp2pkg/sce_sys/livearea/contents/
	cp $(DIST_FILES_THEMES) psp2pkg/data/
ifdef DIST_FILES_ENGINEDATA
	cp $(DIST_FILES_ENGINEDATA) psp2pkg/data/
endif
ifdef DIST_FILES_NETWORKING
	cp $(DIST_FILES_NETWORKING) psp2pkg/data/
endif
ifdef DIST_FILES_VKEYBD
	cp $(DIST_FILES_VKEYBD) psp2pkg/data/
endif
ifdef DYNAMIC_MODULES
	mkdir -p psp2pkg/plugins
	# Each plugin is built as ELF with .suprx extension in main directory because of PLUGIN_SUFFIX variable
	# Then it's stripped and converted here to Vita ELF and SELF inside the package directory
	set -e ;\
	for p in $(PLUGINS); do \
		p=$${p%.suprx} ;\
		$(STRIP) --strip-debug $$p.suprx -o $$p.stripped.elf ;\
		vita-elf-create -n -e $(srcdir)/backends/plugins/psp2/plugin.yml $$p.stripped.elf $$p.velf ;\
		vita-make-fself -s $$p.velf psp2pkg/plugins/$$(basename "$$p").suprx ;\
	done
endif
	cp $(DIST_FILES_DOCS) psp2pkg/doc/
	cp $(srcdir)/dists/psp2/readme-psp2.md psp2pkg/doc/
	cd psp2pkg && zip -r ../$(EXECUTABLE).vpk . && cd ..

.PHONY: psp2vpk
