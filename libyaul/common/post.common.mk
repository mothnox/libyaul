ifeq ($(strip $(OBJECTS)),)
  $(error Empty OBJECTS (object list))
endif

DEPS:= ${OBJECTS:.o=.d}

IMAGE_DIRECTORY?= cd
IMAGE_1ST_READ_BIN?= A.BIN

all: $(PROJECT).iso

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@
	@sh -c "du -h -s $@ | cut -d '	' -f 1"

$(PROJECT).elf: $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) $(foreach lib,$(LIBRARIES),-l$(lib)) -o $@
	$(NM) $(PROJECT).elf > $(PROJECT).sym
	$(OBJDUMP) -S $(PROJECT).elf > $(PROJECT).asm

%.romdisk: $(shell find ./romdisk -type f 2> /dev/null) $(ROMDISK_DEPS)
	genromfs -a 16 -v -V "ROOT" -d ./romdisk/ -f $@

%.romdisk.o: %.romdisk
	fsck.genromfs ./romdisk/
	bin2o $< `echo "$<" | sed -E 's/[\. ]/_/g'` $@

%.o: %.c
	$(CC) $(CFLAGS) -Wp,-MMD,$*.d -c -o $@ $<

%.o: %.S
	$(AS) $(AFLAGS) -o $@ $<

$(PROJECT).iso: $(PROJECT).bin IP.BIN $(shell find $(IMAGE_DIRECTORY)/ -type f)
	mkdir -p $(IMAGE_DIRECTORY)
	cp $(PROJECT).bin $(IMAGE_DIRECTORY)/$(IMAGE_1ST_READ_BIN)
	for txt in "ABS.TXT" "BIB.TXT" "CPY.TXT"; do \
	    if ! [ -s $(IMAGE_DIRECTORY)/$$txt ]; then \
	        printf -- "empty\n" > $(IMAGE_DIRECTORY)/$$txt; \
	    fi \
	done
	make-iso $(IMAGE_DIRECTORY) $(PROJECT)

IP.BIN: $(INSTALL_ROOT)/share/yaul/bootstrap/ip.S
	$(eval $@_TMP_FILE:= $(shell mktemp))
	cat $< | awk ' \
	/\.ascii \"\$$VERSION\"/ { sub(/\$$VERSION/, "$(IP_VERSION)"); } \
	/\.ascii \"\$$RELEASE_DATE\"/ { sub(/\$$RELEASE_DATE/, "$(IP_RELEASE_DATE)"); } \
	/\.ascii \"\$$AREAS\"/ { printf ".ascii \"%-10.10s\"\n", "$(IP_AREAS)"; next; } \
	/\.ascii \"\$$PERIPHERALS\"/ { printf ".ascii \"%-16.16s\"\n", "$(IP_PERIPHERALS)"; next; } \
	/\.ascii \"\$$TITLE\"/ { \
	    # TODO: Allow for titles to be greater than 16 characters \
	    printf ".ascii \"%-16s\"\n", "$(IP_TITLE)"; \
	    next; \
	} \
	/\.long \$$MASTER_STACK_ADDR/ { sub(/\$$MASTER_STACK_ADDR/, "$(IP_MASTER_STACK_ADDR)"); } \
	/\.long \$$SLAVE_STACK_ADDR/ { sub(/\$$SLAVE_STACK_ADDR/, "$(IP_SLAVE_STACK_ADDR)"); } \
	/\.long \$$1ST_READ_ADDR/ { sub(/\$$1ST_READ_ADDR/, "$(IP_1ST_READ_ADDR)"); } \
	{ print; } \
        ' | $(AS) $(AFLAGS) \
		-I$(INSTALL_ROOT)/share/yaul/bootstrap -o $($@_TMP_FILE) -
	$(CC) -Wl,-Map,$@.map -nostdlib -m2 -mb -nostartfiles \
	-specs=ip.specs $($@_TMP_FILE) -o $@
	$(RM) $($@_TMP_FILE)

clean:
	-rm -f $(OBJECTS) \
		$(DEPS) \
		$(PROJECT).asm \
		$(PROJECT).bin \
		$(PROJECT).elf \
		$(PROJECT).map \
		IP.BIN \
		IP.BIN.map \
		$(PROJECT).sym

-include $(DEPS)
