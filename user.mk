#
#
#================================================================
#   
#   
#   文件名称：user.mk
#   创 建 者：肖飞
#   创建日期：2019年10月25日 星期五 13时04分38秒
#   修改日期：2022年03月28日 星期一 16时32分03秒
#   描    述：
#
#================================================================
include config.mk

USER_C_INCLUDES += -Iapps
USER_C_INCLUDES += -Iapps/modules
USER_C_INCLUDES += -Iapps/modules/os
USER_C_INCLUDES += -Iapps/modules/drivers
USER_C_INCLUDES += -Iapps/modules/hardware
USER_C_INCLUDES += -Iapps/modules/app
USER_C_INCLUDES += -Iapps/modules/app/vfs_disk


USER_C_SOURCES += apps/os_memory.c
USER_C_SOURCES += apps/os_random.c
USER_C_SOURCES += apps/app.c
USER_C_SOURCES += apps/uart_debug_handler.c
USER_C_SOURCES += apps/can_config.c

USER_C_SOURCES += apps/modules/app/uart_debug.c
USER_C_SOURCES += apps/modules/app/file_log.c
USER_C_SOURCES += apps/modules/app/vfs_disk/vfs.c
USER_C_SOURCES += apps/modules/app/mt_file.c
USER_C_SOURCES += apps/modules/app/can_data_task.c
USER_C_SOURCES += apps/modules/app/uart_data_task.c
USER_C_SOURCES += apps/modules/app/duty_cycle_pattern.c
USER_C_SOURCES += apps/modules/app/usbh_user_callback.c
USER_C_SOURCES += apps/modules/app/early_sys_callback.c
USER_C_SOURCES += apps/modules/app/usb_upgrade.c
USER_C_SOURCES += apps/modules/app/firmware_upgrade.c
USER_C_SOURCES += apps/modules/hardware/flash.c
USER_C_SOURCES += apps/modules/hardware/modbus_slave_txrx.c
USER_C_SOURCES += apps/modules/hardware/modbus_spec.c
USER_C_SOURCES += apps/modules/hardware/storage.c
ifneq ($(call ifdef_any_of,STORAGE_OPS_25LC1024),)
USER_C_SOURCES += apps/modules/hardware/storage_25lc1024.c
endif
ifneq ($(call ifdef_any_of,STORAGE_OPS_24LC128),)
USER_C_SOURCES += apps/modules/hardware/storage_24lc128.c
endif
ifneq ($(call ifdef_any_of,STORAGE_OPS_W25Q256),)
USER_C_SOURCES += apps/modules/hardware/storage_w25q256.c
endif
USER_C_SOURCES += apps/modules/drivers/spi_txrx.c
USER_C_SOURCES += apps/modules/drivers/can_txrx.c
USER_C_SOURCES += apps/modules/drivers/can_ops_hal.c
USER_C_SOURCES += apps/modules/drivers/usart_txrx.c
USER_C_SOURCES += apps/modules/os/event_helper.c
USER_C_SOURCES += apps/modules/os/callback_chain.c
USER_C_SOURCES += apps/modules/os/bitmap_ops.c
USER_C_SOURCES += apps/modules/os/iap.c
USER_C_SOURCES += apps/modules/os/os_utils.c
USER_C_SOURCES += apps/modules/os/cpu_utils.c
USER_C_SOURCES += apps/modules/os/log.c
USER_C_SOURCES += apps/modules/os/object_class.c
USER_C_SOURCES += apps/modules/os/retarget.c
USER_C_SOURCES += apps/modules/os/syscalls.c


USER_CFLAGS += -DtraceTASK_SWITCHED_IN=StartIdleMonitor -DtraceTASK_SWITCHED_OUT=EndIdleMonitor
USER_CFLAGS += -DLOG_CONFIG_FILE=\"log_config.h\"

#USER_CFLAGS += -DLOG_DISABLE
#USER_CFLAGS += -DALLOC_TRACE_DISABLE

CFLAGS += $(USER_CFLAGS) $(CONFIG_CFLAGS)

#LDFLAGS += -u _printf_float -Wl,--wrap=srand -Wl,--wrap=rand
LDFLAGS += -u _printf_float

default: all

IAP_FILE := apps/modules/os/iap.h

#define update-iap-include
#	if [ -f $(IAP_FILE) ]; then
#		touch $(IAP_FILE);
#	fi
#endef

ifneq ($(call ifdef_any_of,USER_APP),)
build-type := .app.stamps
build-type-invalid := .bootloader.stamps
CFLAGS += -DUSER_APP
LDSCRIPT = STM32F207VETx_FLASH_APP.ld
#$(info $(shell $(update-iap-include)))
$(info "build app!")
else
build-type := .bootloader.stamps
build-type-invalid := .app.stamps
LDSCRIPT = STM32F207VETx_FLASH.ld
#$(info $(shell $(update-iap-include)))
$(info "build bootloader!")
endif

$(build-type) :
#	$(shell $(update-iap-include))
	-rm $(build-type-invalid)
	touch $@


PHONY += all
PHONY += default

USER_DEPS := config.mk $(build-type) $(LDSCRIPT)

cscope: all
	rm cscope e_cs -rf
	mkdir -p cscope
	#$(silent)tags.sh prepare;
	$(silent)touch dep_files;
	$(silent)touch raw_dep_files;
	$(silent)for f in $$(find . -type f -name "*.d" 2>/dev/null); do \
		cat "$$f" >> raw_dep_files; \
	done;
	for i in $$(cat "raw_dep_files" | sed 's/^.*://g' | sed 's/[\\ ]/\n/g' | sort -h | uniq); do \
		if test "$${i:0:1}" = "/";then \
			echo "$$i" >> dep_files; \
		else \
			readlink -f "$$i" >> dep_files; \
		fi; \
	done; \
	$(silent)rm raw_dep_files
	$(silent)cat dep_files | sort | uniq | sed 's/^\(.*\)$$/\"\1\"/g' >> cscope/cscope.files;
	$(silent)cat dep_files | sort | uniq >> cscope/ctags.files;
	$(silent)rm dep_files
	$(silent)tags.sh cscope;
	$(silent)tags.sh tags;
	$(silent)tags.sh env;

clean: clean-cscope
clean-cscope:
	rm cscope e_cs -rf

firmware:
	python apps/modules/fw.py -f build/eva_boot.bin
