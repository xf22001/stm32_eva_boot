

/*================================================================
 *
 *
 *   文件名称：app.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月11日 星期五 16时54分03秒
 *   修改日期：2021年09月03日 星期五 20时40分16秒
 *   描    述：
 *
 *================================================================*/
#include "app.h"

#include <string.h>

#include "iwdg.h"

#include "os_utils.h"

#include "usart_txrx.h"
#include "file_log.h"
#include "uart_debug.h"
#include "usb_upgrade.h"

#include "duty_cycle_pattern.h"
#include "iap.h"

#include "log.h"

extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;

static app_info_t *app_info = NULL;
static os_signal_t app_event = NULL;

app_info_t *get_app_info(void)
{
	return app_info;
}

void app_init(void)
{
	if(app_event != NULL) {
		return;
	}

	app_event = signal_create(1);
}

void send_app_event(app_event_t event)
{
	signal_send(app_event, event, 0);
}

void app(void const *argument)
{

	app_info = (app_info_t *)os_calloc(1, sizeof(app_info_t));

	OS_ASSERT(app_info != NULL);

	//get_or_alloc_uart_debug_info(&huart1);
	//add_log_handler((log_fn_t)log_uart_data);


	debug("===========================================start app============================================");

	try_to_firmware_upgrade();

	while(1) {
		uint32_t event;
		int ret = signal_wait(app_event, &event, 100);

		if(ret == 0) {
			switch(event) {
				case APP_EVENT_USB: {
					start_usb_upgrade();
				}
				break;

				default: {
				}
				break;
			}
		}

		handle_usb_upgrade();
	}
}

static pattern_state_t work_pattern_state = {
	.type = PWM_COMPARE_COUNT_UP,
	.duty_cycle = 0,
};

static void update_work_led(void)
{
	//计数值小于duty_cycle,输出1;大于duty_cycle输出0
	uint16_t duty_cycle = get_duty_cycle_pattern(&work_pattern_state, 1000, 0, 20);
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty_cycle);
}

void idle(void const *argument)
{
	MX_IWDG_Init();
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

	while(1) {
		HAL_IWDG_Refresh(&hiwdg);
		update_work_led();
		osDelay(10);
	}
}

int force_bootloader(void)
{
	int ret = -1;
	u_uint8_bits_t u_uint8_bits;
	u_uint8_bits.v = 0;

	HAL_Init();
	MX_GPIO_Init();

	u_uint8_bits.s.bit0 = (HAL_GPIO_ReadPin(d1_GPIO_Port, d1_Pin) == GPIO_PIN_SET) ? 1 : 0;
	u_uint8_bits.s.bit1 = (HAL_GPIO_ReadPin(d2_GPIO_Port, d2_Pin) == GPIO_PIN_SET) ? 1 : 0;
	u_uint8_bits.s.bit2 = (HAL_GPIO_ReadPin(d3_GPIO_Port, d3_Pin) == GPIO_PIN_SET) ? 1 : 0;

	if(u_uint8_bits.v == 0x07) {
		ret = 0;
	}

	HAL_DeInit();

	return ret;
}
