#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>

// 传感器数据结构体
typedef struct {
    float temperature;      // 温度 (°C)
    int humidity;           // 湿度 (%)
    int smoke_value;        // 烟雾值 (ADC值)
    bool alarm_active[3];   // 报警是否激活 [温度, 湿度, 烟雾]
    int alarm_level[3];     // 报警等级 [温度, 湿度, 烟雾] (0:正常, 1:黄色报警, 2:红色报警)
} oled_sensor_data_t;

// OLED初始化函数
int OLED_Init(void);
void OLED_Deinit(void);
void OLED_Clear(void);
void OLED_Refresh(void);
bool OLED_IsReady(void);

// 显示函数
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size);
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowTemperatureHumidity(float temperature, int humidity);
void OLED_ShowSensorData(const oled_sensor_data_t* sensor_data);

#endif // __OLED_H__