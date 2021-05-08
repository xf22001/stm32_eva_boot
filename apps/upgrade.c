

/*================================================================
 *
 *
 *   文件名称：upgrade.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月08日 星期六 13时42分36秒
 *   修改日期：2021年05月08日 星期六 17时12分24秒
 *   描    述：
 *
 *================================================================*/
#include "upgrade.h"
#include "mt_file.h"
#include "flash.h"
#include "iap.h"

#include "log.h"

static int check_firmware(void)
{
	int ret = -1;
	FIL file;
	FRESULT r;
	uint32_t fw_crc;
	uint32_t crc = 0;
	UINT read;
	uint8_t buffer[16];
	FILINFO file_info;
	uint32_t count = 0;

	r = mt_f_open(&file, "/fw.crc", FA_READ);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_read(&file, &fw_crc, sizeof(fw_crc), &read);

	if(r != FR_OK) {
		debug("");
		goto close;
	}

	if(read != sizeof(fw_crc)) {
		debug("");
		goto close;
	}

	r = mt_f_close(&file);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_stat("/fw.bin", &file_info)					/* Get file status */;

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_open(&file, "/fw.bin", FA_READ);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	while(count < file_info.fsize) {
		r = mt_f_read(&file, buffer, 16, &read);

		if(r != FR_OK) {
			goto close;
		}

		count += read;
		crc += sum_crc32(buffer, read);
	}

	if(crc == fw_crc) {
		debug("ok!");
		ret = 0;
	}

close:
	mt_f_close(&file);
exit:
	return ret;
}

static int flush_firmware(void)
{
	int ret = -1;
	FIL file;
	FRESULT r;
	UINT read;
	uint32_t fw_crc = 0;
	uint32_t crc = 0;
	uint8_t buffer[16];
	FILINFO file_info;
	uint32_t count = 0;

	//擦除第6和7扇
	if(flash_erase_sector(FLASH_SECTOR_6, 2) != 0) {
		debug("");
		goto exit;
	}

	r = mt_f_stat("/fw.bin", &file_info)					/* Get file status */;

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_open(&file, "/fw.bin", FA_READ);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	while(count < file_info.fsize) {
		r = mt_f_read(&file, buffer, 16, &read);

		if(r != FR_OK) {
			debug("");
			goto close;
		}

		if(flash_write(USER_FLASH_FIRST_PAGE_ADDRESS + count, buffer, read) != 0) {
			debug("");
			goto close;
		}

		fw_crc += sum_crc32(buffer, read);
		count += read;
	}

	crc = sum_crc32((void *)USER_FLASH_FIRST_PAGE_ADDRESS, count);

	if(crc == fw_crc) {
		uint8_t flag = 0x01;

		if(flash_write(APP_CONFIG_ADDRESS, &flag, 1) != 0) {
			debug("");
			goto close;
		}

		debug("ok!");
		ret = 0;
	}

close:
	mt_f_close(&file);
exit:
	return ret;
}

static os_signal_t usb_fs_signal = NULL;

void upgrade_init(void)
{
	usb_fs_signal = signal_create(1);
}

void start_upgrade(void)
{
	signal_send(usb_fs_signal, 0, 0);
}

void task_usb_upgrade(void const *argument)
{
	upgrade_state_t state = UPGRADE_STATE_SYNC;
	int ret;

	while(1) {
		switch(state) {
			case UPGRADE_STATE_SYNC: {
				ret = signal_wait(usb_fs_signal, NULL, 30 * 1000);

				if(ret == 0) {
					state = UPGRADE_STATE_CHECK_FIRMWARE;
				} else {
					HAL_NVIC_SystemReset();
				}
			}
			break;

			case UPGRADE_STATE_CHECK_FIRMWARE: {
				ret = check_firmware();

				if(ret == 0) {
					state = UPGRADE_STATE_FLUSH_FIRMWARE;
				} else {
					HAL_NVIC_SystemReset();
				}
			}
			break;

			case UPGRADE_STATE_FLUSH_FIRMWARE: {
				ret = flush_firmware();

				if(ret == 0) {
				} else {
				}

				HAL_NVIC_SystemReset();
			}
			break;

			default: {
			}
			break;
		}
	}
}

void start_usb_upgrade(void)
{
	osThreadDef(usb_upgrade, task_usb_upgrade, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(usb_upgrade), NULL);
}
