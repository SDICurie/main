
emulator:
	$(AT)if [ -e $(T)/packages/intel/emulator ];then \
		EMULATOR=$(T)/packages/intel/emulator; \
	fi; \
	if [ -e $(T)/tools/emulator ];then \
		EMULATOR=$(T)/tools/emulator; \
	fi; \
	if [ -e $(T)/external/qemu ] && [ ! -z $$EMULATOR ];then \
		echo ""; \
		echo $(ANSI_CYAN)"Building Emulator"$(ANSI_OFF); \
		mkdir -p $(OUT)/qemu; \
		(cd $(OUT)/qemu; \
			if [ ! -e config-host.h ];then \
				$(T)/external/qemu/configure \
					--target-list="i386-softmmu,arc-softmmu" \
					--enable-skinning \
					--enable-debug \
					--prefix=/usr \
					--interp-prefix=/etc/qemu-binfmt/%M \
					--disable-blobs \
					--disable-strip \
					--sysconfdir=/etc \
					--enable-rbd \
					--audio-drv-list="pa,alsa,sdl,oss" --enable-vnc-sasl --enable-docs; \
			fi; \
			$(MAKE)); \
		mkdir -p $(OUT)/emulator; \
		mkdir -p $(OUT)/emulator/qemu_skin; \
		cp -L $(T)/external/qemu/skin/hvga/* $(OUT)/emulator/qemu_skin; \
		cp -L $(OUT)/qemu/i386-softmmu/qemu-system-i386 $(OUT)/emulator/emulator_x86; \
		cp -L $(OUT)/qemu/arc-softmmu/qemu-system-arc $(OUT)/emulator/emulator_arc; \
		cp -L $(T)/external/qemu/scripts/emulator_ctb.sh $(OUT)/emulator/; \
		cp -L $(T)/external/qemu/scripts/emulator_crb.sh $(OUT)/emulator/; \
		cp -L $(T)/external/qemu/scripts/emulator_x86.sh $(OUT)/emulator/; \
		cp -L $(T)/external/qemu/scripts/emulator_arc.sh $(OUT)/emulator/; \
		cp -L $(T)/external/qemu/docs/emulator_curie.txt $(OUT)/emulator/; \
		cp -Lr $(T)/external/qemu/factory $(OUT)/emulator/; \
		cp -L $(T)/external/toolchain/tools/debugger/gdb-arc/bin/gdb-arc $(OUT)/emulator/arc-elf32-gdb; \
		touch $(OUT)/emulator/emulator_data.bin; \
		touch $(OUT)/emulator/emulator_flash.bin; \
		if [ -e $$EMULATOR/SensorDataStreamer/prebuilt/SensorDataStreamerWear.apk ];then \
			cp -L $$EMULATOR/SensorDataStreamer/prebuilt/SensorDataStreamerWear.apk $(OUT)/emulator/; \
		fi; \
		if [ -e $$EMULATOR/SensorDataStreamer/prebuilt/SensorDataStreamerMobile.apk ];then \
			cp -L $$EMULATOR/SensorDataStreamer/prebuilt/SensorDataStreamerMobile.apk $(OUT)/emulator/; \
		fi; \
		if [ -e $$EMULATOR/SensorDataCollector/prebuilt/SensorDataCollectorWear.apk ];then \
			cp -L $$EMULATOR/SensorDataCollector/prebuilt/SensorDataCollectorWear.apk $(OUT)/emulator/; \
		fi; \
		if [ -e $$EMULATOR/SensorDataCollector/prebuilt/SensorDataCollectorMobile.apk ];then \
			cp -L $$EMULATOR/SensorDataCollector/prebuilt/SensorDataCollectorMobile.apk $(OUT)/emulator/; \
		fi; \
		if [ -e $$EMULATOR/SensorDataBase ];then \
			cp -L $$EMULATOR/SensorDataBase/*.trace $(OUT)/emulator/; \
		fi; \
		if [ -e $(T)/external/go ] && [ -e $$EMULATOR/SensorProxy ] ;then \
			/bin/bash -c "export GOROOT=$(T)/external/go; \
			export GOPATH=$(T)/external/go-packages; \
			export OUT=$(OUT); \
			$(MAKE) -C $$EMULATOR/SensorProxy/"; \
		fi; \
		if [ -e $$EMULATOR/pattern_matching ];then \
			cp -L $$EMULATOR/pattern_matching/emulator_pvp.so $(OUT)/emulator/; \
		fi; \
		echo $(ANSI_CYAN)"Done Emulator"$(ANSI_OFF); \
	fi

