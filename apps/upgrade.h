

/*================================================================
 *   
 *   
 *   文件名称：upgrade.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月08日 星期六 13时42分40秒
 *   修改日期：2021年05月08日 星期六 16时02分43秒
 *   描    述：
 *
 *================================================================*/
#ifndef _UPGRADE_H
#define _UPGRADE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	UPGRADE_STATE_SYNC = 0,
	UPGRADE_STATE_CHECK_FIRMWARE,
	UPGRADE_STATE_FLUSH_FIRMWARE,
} upgrade_state_t;

void upgrade_init(void);
void start_upgrade(void);
void start_usb_upgrade(void);
#endif //_UPGRADE_H
