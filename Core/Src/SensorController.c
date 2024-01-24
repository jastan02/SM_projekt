#include "SensorController.h"

void SensorController_Init(SensorController *sc, TIM_HandleTypeDef *htim4, TIM_HandleTypeDef *htim14, UART_HandleTypeDef *huart) {
    memset(sc->readings, 0, sizeof(sc->readings));
    sc->total = 0;
    sc->average = 0;
    sc->readIndex = 0;
    sc->Kp = 38;
    sc->Ki = 0.01;
    sc->Kd = 1.6;
    sc->Setpoint = 25;
    sc->error = sc->prevError = sc->integral = sc->derivative = 0;
    sc->lastTime = HAL_GetTick();
    sc->htim4 = htim4;
    sc->htim14 = htim14;
    sc->huart = huart;
}

void SensorController_Update(SensorController *sc) {
    sc->value = SensorController_ReadSensor(sc);
}

float SensorController_ReadSensor(SensorController *sc) {
    uint32_t start = HAL_TIM_ReadCapturedValue(sc->htim4, TIM_CHANNEL_1);
    uint32_t stop = HAL_TIM_ReadCapturedValue(sc->htim4, TIM_CHANNEL_2);
    uint32_t newValue = (stop - start) / 58.0f;

    sc->total = sc->total - sc->readings[sc->readIndex] + newValue;
    sc->readings[sc->readIndex] = newValue;
    sc->readIndex = (sc->readIndex + 1) % NUM_READINGS;
    sc->average = sc->total / NUM_READINGS;

    return sc->average;
}

void SensorController_ProcessPID(SensorController *sc) {
    float czas = HAL_GetTick();
    sc->error = sc->Setpoint - sc->value;
    sc->integral += sc->error * (czas - sc->lastTime);
    sc->derivative = (sc->error - sc->prevError) / (czas - sc->lastTime);

    float output = (sc->Kp * sc->error) + (sc->Ki * sc->integral) + (sc->Kd * sc->derivative);
    sc->prevError = sc->error;
    sc->lastTime = HAL_GetTick();

    SensorController_SetServoPWMFromPID(sc, output);
}

void SensorController_SetServoPWMFromPID(SensorController *sc, float pidOutput) {
    float scaledOutput = 1200 - pidOutput;
    if (scaledOutput < 600) {
        scaledOutput = 600;
    } else if (scaledOutput > 1800) {
        scaledOutput = 1800;
    }

    __HAL_TIM_SET_COMPARE(sc->htim14, TIM_CHANNEL_1, (uint16_t)scaledOutput);
}

void SensorController_SendDataOverUART(SensorController *sc) {
    char buffer[100];
    sprintf(buffer, "Sensor: %.2f, Kp: %.2f, Ki: %.2f, Kd: %.2f\r\n", sc->value, sc->Kp, sc->Ki, sc->Kd);
    HAL_UART_Transmit(sc->huart, (uint8_t*)buffer, strlen(buffer), 100);
}

void SensorController_ProcessReceivedData(SensorController *sc, const char* data) {
    // Tymczasowe zmienne do przechowywania odczytanych wartości
    float tempKp, tempKi, tempKd, tempSetpoint;

    if (sscanf(data, "Set Kp:%f Ki:%f Kd:%f Setpoint:%f", &tempKp, &tempKi, &tempKd, &tempSetpoint) == 4) {
        sc->Kp = tempKp;
        sc->Ki = tempKi;
        sc->Kd = tempKd;
        sc->Setpoint = tempSetpoint;

        // Dodatkowe logi, jeśli są potrzebne
    }
}

void SensorController_HAL_UART_RxCpltCallback(SensorController *sc, UART_HandleTypeDef *huart) {
    if (huart == sc->huart) {
        char receivedChar = (char)(huart->Instance->RDR & 0xFF);

        if (receivedChar == '\n' || sc->rxIndex >= RX_BUFFER_SIZE - 1) {
            sc->rxBuffer[sc->rxIndex] = '\0';
            SensorController_ProcessReceivedData(sc, sc->rxBuffer);

            memset(sc->rxBuffer, 0, RX_BUFFER_SIZE);
            sc->rxIndex = 0;
        } else {
            sc->rxBuffer[sc->rxIndex++] = receivedChar;
        }

        HAL_UART_Receive_IT(huart, (uint8_t *)&sc->rxBuffer[sc->rxIndex], 1);
    }
}
