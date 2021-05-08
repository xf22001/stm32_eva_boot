

/*================================================================
 *   
 *   
 *   文件名称：early_sys_callback.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月25日 星期四 15时11分14秒
 *   修改日期：2021年05月08日 星期六 16时03分15秒
 *   描    述：
 *
 *================================================================*/
#include "early_sys_callback.h"
#include "mt_file.h"
#include "os_utils.h"
#include "upgrade.h"

void early_sys_callback(void)
{
	init_mem_info();
	mt_file_environment_init();
	upgrade_init();
}
