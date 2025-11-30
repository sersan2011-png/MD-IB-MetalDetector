/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

#define BTN_MENU   HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)
#define BTN_UP     HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14)
#define BTN_DOWN   HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15)
#define BTN_LEFT   HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)
#define BTN_RIGHT  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)
#define BTN_OK     HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
