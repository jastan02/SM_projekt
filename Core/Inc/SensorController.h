#ifndef SENSORCONTROLLER_H
#define SENSORCONTROLLER_H
#define RX_BUFFER_SIZE 50

#include "main.h"

#define NUM_READINGS 10
#define MIN_VALID_VALUE 2
#define MAX_VALID_VALUE 60

typedef struct {
    uint32_t readings[NUM_READINGS];
    uint32_t total;
    uint32_t average;
    int readIndex;
    float value;
    float Kp, Ki, Kd;
    float Setpoint;
    float error, prevError, integral, derivative;
    float lastTime;
    TIM_HandleTypeDef *htim4;
    TIM_HandleTypeDef *htim14;
    UART_HandleTypeDef *huart;
    char rxBuffer[RX_BUFFER_SIZE];
    uint32_t rxIndex;
} SensorController;

void SensorController_Init(SensorController *sc, TIM_HandleTypeDef *htim4, TIM_HandleTypeDef *htim14, UART_HandleTypeDef *huart);
void SensorController_Update(SensorController *sc);
void SensorController_ProcessPID(SensorController *sc);
void SensorController_SendDataOverUART(SensorController *sc);
float SensorController_ReadSensor(SensorController *sc);
void SensorController_SetServoPWMFromPID(SensorController *sc, float pidOutput);

#endif // SENSORCONTROLLER_H
