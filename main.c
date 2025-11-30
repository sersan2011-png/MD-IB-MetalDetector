/* USER CODE BEGIN Header */
/**
  * @brief  Металлоискатель IB
  * @author SSW
  * @date   2025
  */
/* USER CODE END Header */

#include "main.h"
#include "ili9341.h"
#include "fonts.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Внешние дескрипторы (настроены в CubeMX)
extern SPI_HandleTypeDef hspi1;  // TPC5160
extern SPI_HandleTypeDef hspi2;  // ILI9341
extern I2C_HandleTypeDef hi2c1;  // 24C08, PT23119
extern ADC_HandleTypeDef hadc1;  // Батарея

// === Глобальные переменные ===
uint16_t adc_value = 0;
int16_t signal_diff = 0;
int32_t baseline = 0;
uint16_t threshold = 50;
uint32_t carrier_freq = 10000;
uint8_t active_profile_id = 0;
uint8_t in_menu = 0;
uint8_t menu_pos = 0;
uint8_t battery_level = 100;
uint8_t auto_balance = 1;
uint8_t sound_mode = 0;  // 0=4 тона, 1=16, 2=полифония
uint8_t display_brightness = 70;
uint8_t display_contrast = 80;

// === Профили катушек ===
typedef struct {
    uint8_t  id;
    char     name[16];
    uint32_t freq;
    uint8_t  gain;
    uint16_t threshold;
    uint8_t  sound_mode;
    uint8_t  brightness;
    uint8_t  contrast;
} SensorProfile;

SensorProfile sensor_profiles[3] = {
    {0, "Stock Coil", 10000, 50, 50, 0, 70, 80},
    {1, "HighFreq",   40000, 30, 30, 1, 80, 90},
    {2, "Deep",        5000, 70, 80, 2, 60, 70}
};

SensorProfile current_profile;

// === Кнопки ===
#define BTN_MENU   HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)
#define BTN_UP     HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14)
#define BTN_DOWN   HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15)
#define BTN_LEFT   HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)
#define BTN_RIGHT  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)
#define BTN_OK     HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)

// === Функции ===
void SystemClock_Config(void);
void ReadTPC5160(void);
void AutoBalance(void);
void PlayTone(uint16_t freq, uint16_t dur);
void DrawUI(void);
void HandleMenu(void);
void SaveProfileToEEPROM(uint8_t id);
void LoadProfileFromEEPROM(uint8_t id);
void ApplyProfile(uint8_t id);
int16_t CalculateVDI(void);
void DrawSignalGraph(int16_t val);
void DrawHodograph(int16_t amp, int16_t phase);
uint8_t ReadBattery(void);

// === MAIN ===
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();

    LCD_Init();
    LCD_Clear(LCD_BLACK);

    // Загрузить профиль
    LoadProfileFromEEPROM(active_profile_id);
    ApplyProfile(active_profile_id);

    // Калибровка базовой линии
    HAL_Delay(1000);
    baseline = 0;
    for (int i = 0; i < 100; i++) {
        ReadTPC5160();
        baseline += adc_value;
        HAL_Delay(10);
    }
    baseline /= 100;

    // Вывод стартового экрана
    LCD_DrawString(20, 50, "Металлоискатель IB", 1, LCD_GREEN, LCD_BLACK);
    LCD_DrawString(60, 100, "Готов", 1, LCD_WHITE, LCD_BLACK);
    HAL_Delay(1500);
    LCD_Clear(LCD_BLACK);

    while (1) {
        ReadTPC5160();
        signal_diff = (int16_t)(adc_value - baseline);

        if (auto_balance) AutoBalance();
        if (!in_menu) DrawUI();

        // Звук
        if (abs(signal_diff) > threshold) {
            int16_t vdi = CalculateVDI();
            uint16_t freq = 500 + abs(signal_diff) * 2;
            if (vdi < -30) freq += 200;  // Железо — низкий
            else if (vdi > 30) freq += 400;  // Цветной — высокий
            PlayTone(freq, 20);
        }

        // Кнопки
        if (!BTN_MENU) {
            HAL_Delay(50);
            in_menu = !in_menu;
            if (in_menu) HandleMenu();
        }

        HAL_Delay(20);
    }
}

// === Чтение TPC5160 ===
void ReadTPC5160(void) {
    uint8_t tx[3] = {0x00, 0x00, 0x00};
    uint8_t rx[3];
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    adc_value = (rx[0] << 8) | rx[1];
    if (adc_value & 0x8000) adc_value |= 0xFFFF0000;  // 16 бит signed
}

// === Автобаланс ===
void AutoBalance(void) {
    static uint32_t last_update = 0;
    if (HAL_GetTick() - last_update > 500) {
        baseline = 0.98f * baseline + 0.02f * adc_value;
        last_update = HAL_GetTick();
    }
}

// === Звук ===
void PlayTone(uint16_t freq, uint16_t dur) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 500);  // 50% duty
    __HAL_TIM_ENABLE(&htim1);
    HAL_Delay(dur);
    __HAL_TIM_DISABLE(&htim1);
}

// === UI ===
void DrawUI(void) {
    static uint32_t last_draw = 0;
    if (HAL_GetTick() - last_draw < 50) return;
    last_draw = HAL_GetTick();

    LCD_FillRect(0, 0, 240, 30, LCD_BLUE);
    LCD_DrawString(10, 5, "IB Detector", 1, LCD_WHITE, LCD_BLUE);

    // VDI
    int16_t vdi = CalculateVDI();
    char buf[16]; sprintf(buf, "VDI: %d", vdi);
    LCD_DrawString(10, 35, buf, 1, vdi > 0 ? LCD_GREEN : LCD_RED, LCD_BLACK);

    // Батарея
    battery_level = ReadBattery();
    sprintf(buf, "Бат: %d%%", battery_level);
    LCD_DrawString(160, 5, buf, 1, battery_level > 20 ? LCD_GREEN : LCD_RED, LCD_BLUE);

    // Сигнограф
    DrawSignalGraph(signal_diff);

    // Годограф
    int16_t phase = (int16_t)(atan2f(signal_diff, baseline) * 57.3f);
    DrawHodograph(signal_diff, phase);
}

void DrawSignalGraph(int16_t val) {
    static int16_t hist[100] = {0};
    static uint8_t idx = 0;
    hist[idx] = val;
    idx = (idx + 1) % 100;

    LCD_FillRect(10, 60, 220, 60, LCD_BLACK);
    for (int i = 0; i < 99; i++) {
        int x1 = 10 + i;
        int x2 = 10 + i + 1;
        int y1 = 90 - hist[(idx + i) % 100] / 50;
        int y2 = 90 - hist[(idx + i + 1) % 100] / 50;
        LCD_DrawLine(x1, y1, x2, y2, LCD_GREEN);
    }
}

void DrawHodograph(int16_t amp, int16_t phase) {
    LCD_FillRect(10, 130, 100, 100, LCD_BLACK);
    int cx = 60, cy = 180;
    int r = 40;
    LCD_DrawCircle(cx, cy, r, LCD_GRAY);
    int x = cx + (r * amp / 2000);
    int y = cy - (r * phase / 90);
    LCD_DrawFillCircle(x, y, 3, LCD_RED);
}

// === VDI ===
int16_t CalculateVDI(void) {
    int16_t amp = abs(signal_diff);
    if (amp < threshold) return 0;
    int16_t phase_shift = (int16_t)(atan2f(signal_diff, baseline) * 57.3f);
    return CLAMP((phase_shift * 100) / (amp / 10 + 1), -100, 100);
}

#define CLAMP(x, low, high) (((x) < (low)) ? (low) : ((x) > (high)) ? (high) : (x))

// === Меню ===
typedef struct {
    char* name;
    int32_t* value;
    int32_t min;
    int32_t max;
    int32_t step;
} MenuItem;

MenuItem menu[] = {
    {"Частота",    (int32_t*)&carrier_freq,   1000, 50000, 100},
    {"Порог",      (int32_t*)&threshold,         0,  1000,  10},
    {"Звук",       (int32_t*)&sound_mode,       0,     2,   1},
    {"Профиль",    (int32_t*)&active_profile_id, 0,     2,   1},
    {"Сброс",      0,                           0,     0,   0}
};

void HandleMenu(void) {
    LCD_Clear(LCD_BLACK);
    while (in_menu) {
        for (int i = 0; i < 5; i++) {
            char buf[32];
            sprintf(buf, "%s", menu[i].name);
            if (menu[i].value) {
                char val[10];
                if (i == 3) sprintf(val, "%d '%s'", *(menu[i].value), sensor_profiles[*(menu[i].value)].name);
                else sprintf(val, "%ld", *(menu[i].value));
                strcat(buf, ": ");
                strcat(buf, val);
            }
            LCD_DrawString(10, 20 + i * 30, buf, 1, i == menu_pos ? LCD_YELLOW : LCD_WHITE, LCD_BLACK);
        }

        if (!BTN_DOWN)  { menu_pos = (menu_pos + 1) % 5; HAL_Delay(200); }
        if (!BTN_UP)    { menu_pos = (menu_pos + 4) % 5; HAL_Delay(200); }
        if (!BTN_RIGHT && menu[menu_pos].value) { *(menu[menu_pos].value) += menu[menu_pos].step; HAL_Delay(150); }
        if (!BTN_LEFT  && menu[menu_pos].value) { *(menu[menu_pos].value) -= menu[menu_pos].step; HAL_Delay(150); }
        if (!BTN_OK) {
            if (menu_pos == 4) {
                // Сброс
                memcpy(&sensor_profiles, (SensorProfile[]){{0,"Stock Coil",10000,50,50,0,70,80}}, sizeof(sensor_profiles));
                active_profile_id = 0;
                ApplyProfile(0);
                LCD_DrawString(10, 200, "Сброс!", 1, LCD_RED, LCD_BLACK);
                HAL_Delay(1000);
            }
            in_menu = 0;
        }
        if (!BTN_MENU) { in_menu = 0; }

        LCD_Clear(LCD_BLACK);
    }
    ApplyProfile(active_profile_id);
    SaveProfileToEEPROM(active_profile_id);
}

void ApplyProfile(uint8_t id) {
    current_profile = sensor_profiles[id];
    threshold = current_profile.threshold;
    sound_mode = current_profile.sound_mode;
    display_brightness = current_profile.brightness;
    display_contrast = current_profile.contrast;
    carrier_freq = current_profile.freq;
}

// === EEPROM ===
void SaveProfileToEEPROM(uint8_t id) {
    HAL_I2C_Mem_Write(&hi2c1, 0xA0, id * 32, 1, (uint8_t*)&sensor_profiles[id], 32, 100);
}

void LoadProfileFromEEPROM(uint8_t id) {
    if (HAL_I2C_Mem_Read(&hi2c1, 0xA0, id * 32, 1, (uint8_t*)&sensor_profiles[id], 32, 100) != HAL_OK) {
        // Ошибка — использовать дефолт
        sensor_profiles[id] = (SensorProfile){id, "Default", 10000, 50, 50, 0, 70, 80};
    }
}

// === Батарея ===
uint8_t ReadBattery(void) {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t adc = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    float v = adc * 3.3f / 4095.0f * 6.0f;  // Делитель 12к/2к = 6
    return (uint8_t)((v - 3.0f) * 100.0f / 0.6f);
}

// === Обязательные функции из CubeMX ===
void SystemClock_Config(void) { /* Автогенерация CubeMX */ }
static void MX_GPIO_Init(void) { /* Автогенерация */ }
static void MX_SPI1_Init(void) { /* Автогенерация */ }
static void MX_SPI2_Init(void) { /* Автогенерация */ }
static void MX_I2C1_Init(void) { /* Автогенерация */ }
static void MX_ADC1_Init(void) { /* Автогенерация */ }

// Добавьте в stm32f4xx_it.c обработчики прерываний при необходимости

