

/*================================================================
 *
 *
 *   文件名称：probe_tool_handler.c
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 12时48分07秒
 *   修改日期：2021年05月08日 星期六 11时45分30秒
 *   描    述：
 *
 *================================================================*/
#include "probe_tool_handler.h"

#include <string.h>

#include "lwip/netdb.h"
#include "lwip/inet.h"

#include "net_client.h"
#include "flash.h"
#include "iap.h"
#include "app.h"
#include "ftp_client.h"

#define LOG_UDP
#include "log.h"

extern struct netif gnetif;

static void fn1(request_t *request)
{
	probe_server_chunk_sendto(request->payload.fn, (void *)0x8000000, 512);
}

static void fn2(request_t *request)
{
	probe_server_chunk_sendto(request->payload.fn, (void *)0x8000000, 512);
}

static void fn3(request_t *request)
{
	static uint32_t file_crc32 = 0;

	uint32_t data_size = request->header.data_size;
	uint32_t data_offset = request->header.data_offset;
	uint32_t total_size = request->header.total_size;
	uint32_t stage = request->payload.stage;
	uint8_t *data = (uint8_t *)(request + 1);
	uint8_t is_app = 0;
	uint8_t start_app = 0;

#if defined(USER_APP)
	is_app = 1;
#endif

	if(is_app == 1) {
		uint8_t flag = 0x00;
		flash_write(APP_CONFIG_ADDRESS, &flag, 1);
		_printf("in app, reset for upgrade!\n");
		HAL_NVIC_SystemReset();
		return;
	}

	if(stage == 0) {
		flash_erase_sector(FLASH_SECTOR_6, 2);//擦除第6和7扇区
	} else if(stage == 1) {
		if(data_size == 4) {
			uint32_t *p = (uint32_t *)data;
			file_crc32 = *p;
		}
	} else if(stage == 2) {
		flash_write(USER_FLASH_FIRST_PAGE_ADDRESS + data_offset, data, data_size);

		if(data_offset + data_size == total_size) {
			uint32_t read_offset = 0;
			uint32_t crc32 = 0;

			while(read_offset < total_size) {
				uint32_t i;
				uint32_t left = total_size - read_offset;
				uint32_t read_size = (left > 32) ? 32 : left;
				uint8_t *read_buffer = (uint8_t *)(USER_FLASH_FIRST_PAGE_ADDRESS + read_offset);

				for(i = 0; i < read_size; i++) {
					crc32 += read_buffer[i];
				}

				read_offset += read_size;
			}

			_printf("crc32:%x, file_crc32:%x\n", crc32, file_crc32);

			if(crc32 == file_crc32) {
				start_app = 1;
			}
		}
	}

	loopback(request);

	if(start_app) {
		uint8_t flag = 0x01;

		_printf("start app!\n");
		flash_write(APP_CONFIG_ADDRESS, &flag, 1);
		HAL_NVIC_SystemReset();
	}
}

static int p_host(struct hostent *ent)
{
	int ret = 0;
	char **cp;

	if(ent == NULL) {
		ret = -1;
		return ret;
	}

	_printf("\n");

	_printf("h_name:%s\n", ent->h_name);
	_printf("h_aliases:\n");
	cp = ent->h_aliases;

	while(*cp != NULL) {
		_printf("%s\n", *cp);
		cp += 1;

		if(*cp != NULL) {
			//_printf(", ");
		}
	}

	_printf("h_addrtype:%d\n", ent->h_addrtype);

	_printf("h_length:%d\n", ent->h_length);

	_printf("h_addr_list:\n");
	cp = ent->h_addr_list;

	while(*cp != NULL) {
		_printf("%s\n", inet_ntoa(**cp));
		cp += 1;

		if(*cp != NULL) {
			//_printf(", ");
		}
	}

	return ret;
}

static void get_host_by_name(char *content, uint32_t size)
{
	struct hostent *ent;
	char *hostname = (char *)os_alloc(RECV_BUFFER_SIZE);
	int ret;
	int fn;
	int catched;

	//_hexdump("content", (const char *)content, size);

	if(hostname == NULL) {
		return;
	}

	hostname[0] = 0;

	ret = sscanf(content, "%d %s%n", &fn, hostname, &catched);

	if(ret == 2) {
		_printf("hostname:%s!\n", hostname);
		ent = gethostbyname(hostname);
		p_host(ent);
	} else {
		_printf("no hostname!\n");
	}

	os_free(hostname);
}

static void fn4(request_t *request)
{
	_printf("local host ip:%s\n", inet_ntoa(gnetif.ip_addr));

	get_host_by_name((char *)(request + 1), request->header.data_size);
	memset(request, 0, RECV_BUFFER_SIZE);
}

uint16_t osGetCPUUsage(void);
static void fn5(request_t *request)
{
	int size = xPortGetFreeHeapSize();
	uint8_t *os_thread_info;
	uint8_t is_app = 0;
	uint32_t ticks = osKernelSysTick();
	uint16_t cpu_usage = osGetCPUUsage();
	size_t total_heap_size = get_total_heap_size();
	size_t heap_size;
	size_t heap_count;
	size_t heap_max_size;

#if defined(USER_APP)
	is_app = 1;
#endif
	get_mem_info(&heap_size, &heap_count,  &heap_max_size);

	_printf("cpu usage:%d\n", cpu_usage);
	_printf("free os heap size:%d\n", size);
	_printf("total heap size:%d, free heap size:%d, used:%d, heap count:%d, max heap size:%d\n",
			total_heap_size,
			total_heap_size - heap_size,
			heap_size,
			heap_count,
			heap_max_size);
	_printf("current ticks:%lu\n", ticks);
	_printf("%lu day %lu hour %lu min %lu sec\n",
	        ticks / (1000 * 60 * 60 * 24),//day
	        (ticks % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60),//hour
	        (ticks % (1000 * 60 * 60)) / (1000 * 60),//min
	        (ticks % (1000 * 60)) / (1000)//sec
	       );

	if(size < 4 * 1024) {
		return;
	}

	size = 1024;

	os_thread_info = (uint8_t *)os_alloc(size);

	if(os_thread_info == NULL) {
		return;
	}

	osThreadList(os_thread_info);

	_puts((const char *)os_thread_info);

	os_free(os_thread_info);

	if(is_app) {
		_printf("in app!\n");
	} else {
		_printf("in bootloader!\n");
	}
}

static void fn6(request_t *request)
{
}

#include "eeprom.h"
#include "main.h"
extern SPI_HandleTypeDef hspi3;
static void fn7(request_t *request)
{
	uint8_t id;
	//char *buffer = (char *)os_alloc(1024);
	//int ret;
	//bms_data_settings_t *settings = (bms_data_settings_t *)0;

	//if(buffer == NULL) {
	//	return;
	//}

	eeprom_info_t *eeprom_info = get_or_alloc_eeprom_info(get_or_alloc_spi_info(&hspi3), spi3_cs_GPIO_Port, spi3_cs_Pin, spi3_wp_GPIO_Port, spi3_wp_Pin);

	if(eeprom_info == NULL) {
		return;
	}

	id = eeprom_id(eeprom_info);
	_printf("eeprom id:0x%x\n", id);
	//_printf("test ...\n");

	//memset(buffer, 0, 1024);

	//eeprom_write(eeprom_info, 5 * 1024 + 1, (uint8_t *)0x8000000, 1024);
	//eeprom_read(eeprom_info, 5 * 1024 + 1, (uint8_t *)buffer, 1024);

	//ret = memcmp(buffer, (const void *)0x8000000, 1024);

	//if(ret == 0) {
	//	_printf("read write successful!\n");
	//} else {
	//	_printf("read write failed!\n");
	//}

	//os_free(buffer);
	//_printf("bms_data_settings_t size:%d\n", sizeof(bms_data_settings_t));

	//_printf("settings->cem_data.u1 offset:%d\n", (void *)&settings->cem_data.u1 - (void *)settings);
}

static void fn8(request_t *request)
{
}

static void fn9(request_t *request)
{
}

static void fn10(request_t *request)
{
}

static void fn11(request_t *request)
{
}

static void fn12(request_t *request)
{
}

static server_item_t server_map[] = {
	{1, fn1},
	{2, fn2},
	{3, fn3},
	{4, fn4},
	{5, fn5},
	{6, fn6},
	{7, fn7},
	{8, fn8},
	{9, fn9},
	{10, fn10},
	{11, fn11},
	{12, fn12},
};

server_map_info_t server_map_info = {
	.server_map = server_map,
	.server_map_size = sizeof(server_map) / sizeof(server_item_t),
};