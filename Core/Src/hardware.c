/*
 * Файл с функциями непосредственной проверки контрольных точек и ключевых элементов
 */
 #include "hardware.h"
#include <stdint.h>
#include <string.h>

extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;

uint16_t vol_raw;
uint32_t channel, vol_average, tok;
                                               // функции подсчета переменных

void test_voltage(uint8_t* buf, uint32_t channel){
	vol_average = 0;
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
	    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	    {
	    	buf[0] = STATUS_EXEC_ERROR;
	    	        return;
	    }

	HAL_ADC_Start(&hadc1);
	for (int i = 0; i < SAMPLES; i++) {
		 while (!LL_ADC_IsActiveFlag_EOCS(ADC1)) {}
		 	 	 LL_ADC_ClearFlag_EOCS(ADC1);
		         vol_raw = HAL_ADC_GetValue(&hadc1);
		         vol_average += vol_raw;
		}
	HAL_ADC_Stop(&hadc1);

	vol_average = vol_average * REFERENCE_VOLTAGE / (ADC_BIT_RATE * SAMPLES);

	buf[1] = (uint8_t)(vol_average & 0xFF);
	buf[2] = (uint8_t)((vol_average >> 8) & 0xFF);
    buf[3] = (uint8_t)((vol_average >> 16) & 0xFF);
	buf[4] = (uint8_t)((vol_average >> 24) & 0xFF);
	buf[0] = STATUS_OK;
    return;
}

void apply_relay(GPIO_TypeDef *PORT, uint32_t PIN){
	            return(SET_BIT(PORT->BSRR, PIN));
}

void uart_tx_rx(UART_HandleTypeDef* uart, uint8_t* buf, uint8_t* tx, uint8_t* rx, size_t size){
	if (HAL_UART_Transmit(uart, tx, size, TIMEOUT_RX) != HAL_OK) {
		        buf[0] = STATUS_TIMED_OUT;
		        return;
		    }
		 if (uart->RxState == HAL_UART_STATE_READY) {
			 if (HAL_UART_Receive(uart, rx, size, TIMEOUT_RX) != HAL_OK) {
			             buf[0] = STATUS_TIMED_OUT;
			             return;
			         }
			     } else {
			         HAL_UART_AbortReceive(uart);
			         if (HAL_UART_Receive(uart, rx, size, TIMEOUT_RX) != HAL_OK) {
			             buf[0] = STATUS_TIMED_OUT;
			             return;
			         }
			     }
}

int compare_arrays(uint8_t arr1[], uint8_t arr2[], size_t size){
	for(int i = 0; i < size; i++){
		if (arr1[i] != arr2[i]){
			return 1;
		}
	}
	return 0;
}


                                                 // функции управления



void apply_voltage_relay_1(uint8_t* buf) // Подставить нужный пин, сейчас PB9
{
	switch (buf[1]) {
		case CLOSE_RELAY:
			apply_relay(RELAY_PORT, RELAY_1_PIN_0);
			if (READ_BIT(RELAY_PORT->IDR, RELAY_1_PIN_1) != 0){
								buf[0] = STATUS_EXEC_ERROR;
							} else {
							    buf[0] = STATUS_OK;
							}
			return;
		case OPEN_RELAY:
			apply_relay(RELAY_PORT, RELAY_1_PIN_1);
			if (READ_BIT(RELAY_PORT->IDR, RELAY_1_PIN_1) != 0){
								buf[0] = STATUS_OK;
							} else {
							    buf[0] = STATUS_EXEC_ERROR;
							}
			return;
		default:
			buf[0] = STATUS_INVALID_CMD;
			return;
	}
}

void test_voltage_4_point(uint8_t* buf)
{
	switch (buf[1])
	{
	case CHECKPOINT_6V_NEG:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_3_3V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_6V:
		channel = ADC_MUX;
		break;
	default:
		buf[0] = STATUS_INVALID_CMD;
		return;
	}
	test_voltage(buf, channel);
}

void test_voltage_current(uint8_t* buf)
{
	switch (buf[1])
	{
	case SYPPLY_VOLTAGE:
		channel = ADC_SYPPLY_VOLTAGE;
		test_voltage(buf, channel);
				return;

	case SUPPLY_CURRENT:
		    vol_average = 0;
		    uint32_t res_shunt = RES_SHUNT; // шунтирующий резистор для измерения тока 100 mOm
			ADC_ChannelConfTypeDef sConfig = {0};
			sConfig.Channel = ADC_SUPPLY_CURRENT;
			sConfig.Rank = 1;
			sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
			    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			    {
			    	buf[0] = STATUS_EXEC_ERROR;
			    	        return;
			    }

			HAL_ADC_Start(&hadc1);
			for (int i = 0; i < SAMPLES; i++) {
				 while (!LL_ADC_IsActiveFlag_EOCS(ADC1)) {}
				 	 	 LL_ADC_ClearFlag_EOCS(ADC1);
				         vol_raw = HAL_ADC_GetValue(&hadc1);
				         vol_average += vol_raw;
				}

			HAL_ADC_Stop(&hadc1);
			vol_average = vol_average * REFERENCE_VOLTAGE / (ADC_BIT_RATE * SAMPLES);

			tok = vol_average / res_shunt;
			buf[1] = (uint8_t)(tok & 0xFF);
			buf[2] = (uint8_t)(tok >> 8 & 0xFF);
			buf[3] = (uint8_t)(tok >> 16 & 0xFF);
			buf[4] = (uint8_t)(tok >> 24 & 0xFF);
			buf[0] = STATUS_OK;
			return;

	default:
		buf[0] = STATUS_INVALID_CMD;
		return;
	}
}

void apply_voltage_relay_2(uint8_t* buf) //Подставить нужный пин, сейчас PB8
{
	switch (buf[1]) {
			case CLOSE_RELAY:
				apply_relay(RELAY_PORT, RELAY_2_PIN_0);
				if (READ_BIT(RELAY_PORT->IDR, RELAY_2_PIN_1) != 0){
						buf[0] = STATUS_EXEC_ERROR;
					} else {
					    buf[0] = STATUS_OK;
					}
				return;
			case OPEN_RELAY:
				apply_relay(RELAY_PORT, RELAY_2_PIN_1);
				if (READ_BIT(RELAY_PORT->IDR, RELAY_2_PIN_1) != 0){
						buf[0] = STATUS_OK;
					} else {
					    buf[0] = STATUS_EXEC_ERROR;
					}
				return;
			default:
				buf[0] = STATUS_INVALID_CMD;
				return;
		}
}

void test_voltage_11_point(uint8_t* buf)
{
	switch (buf[1])
	{
	case CHECKPOINT_1_2V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_1_8V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_2_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_GPS_5_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_VREF_ADC_4_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_5_5VA:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_5_5VA_NEG:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_1_8VA:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_OFFSET_2_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_LASER_5V:
		channel = ADC_MUX;
		break;

	case CHECKPOINT_VREF_DAC_2_048V:
		channel = ADC_MUX;
		break;

	default:
		buf[0] = STATUS_INVALID_CMD;
		return;
	}
	test_voltage(buf, channel);
}

void test_corrent_laser(uint8_t* buf)
{
	uint16_t adcSamples[SAMPLES_LASER];
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_LASER;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
		    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
		    {
		    	buf[0] = STATUS_EXEC_ERROR;
		    	return;
		    }
	HAL_ADC_Start(&hadc1);
		    for (int i = 0; i < SAMPLES_LASER; i++) {
			     while (!LL_ADC_IsActiveFlag_EOCS(ADC1)) {}
			 	 	    LL_ADC_ClearFlag_EOCS(ADC1);
			 	 	    adcSamples[i] = HAL_ADC_GetValue(&hadc1);
			}
		    HAL_ADC_Stop(&hadc1);
		    for (int i = 0; i < SAMPLES_LASER; i++){
			     vol_average = adcSamples[i] * REFERENCE_VOLTAGE / ADC_BIT_RATE;
			     buf[i * 2 + 1] = (uint8_t)(vol_average & 0xFF);
			     buf[i * 2 + 2] = (uint8_t)(vol_average >> 8 & 0xFF);
		    }
	buf[0] = STATUS_OK;
}

void test_voltage_peltie(uint8_t* buf)
{
	channel = ADC_PELTIE;
	test_voltage(buf, channel);
			return;
}

void apply_voltage_relay_5(uint8_t* buf) //Подставить нужный пин, сейчас PB7
{
	switch (buf[1]) {
			case CLOSE_RELAY:
				apply_relay(RELAY_PORT, RELAY_5_PIN_0);
				if (READ_BIT(RELAY_PORT->IDR, RELAY_5_PIN_1) != 0){
						buf[0] = STATUS_EXEC_ERROR;
					} else {
					    buf[0] = STATUS_OK;
					}
				return;
			case OPEN_RELAY:
				apply_relay(RELAY_PORT, RELAY_5_PIN_1);
				if (READ_BIT(RELAY_PORT->IDR, RELAY_5_PIN_1) != 0){
						buf[0] = STATUS_OK;
					} else {
					    buf[0] = STATUS_EXEC_ERROR;
					}
				return;
			default:
				buf[0] = STATUS_INVALID_CMD;
				return;
		}
}

void massage_rs232(uint8_t* buf)
{
	uint8_t rs_232_tx [RS_232] = "RS_232!";
	uint8_t rs_232_rx [RS_232];
	uart_tx_rx(&UART_RS_232, buf, rs_232_tx, rs_232_rx, RS_232);

	if (buf[0] == STATUS_TIMED_OUT){ return; }

	if (compare_arrays(rs_232_tx, rs_232_rx, RS_232) == 0){
		buf[0] = STATUS_OK;
	}
	else {
		buf[0] = STATUS_EXEC_ERROR;
	}
	for (int i = 1; i < 5; i++){
		buf[i] = 0;
	}
}

void massage_gps(uint8_t* buf)
{
	uint8_t gps_tx [GPS_SIZE] = "$GNGLL,5502.49000,N,08256.07600,E,1235  .000,A,A*"; // GLL, version 4.1 and 4.2, NMEA 0183
	uint8_t gps_rx [GPS_SIZE];
	gps_tx[38] = (buf[1]/ 10) + '0';
	gps_tx[39] = (buf[1] % 10) + '0';
	uart_tx_rx(&UART_GPS, buf, gps_tx, gps_rx, GPS_SIZE);

	if (buf[0] == STATUS_TIMED_OUT){ return; }

	if (compare_arrays(gps_tx, gps_rx, GPS_SIZE) == 0){
			buf[0] = STATUS_OK;
		}
		else {
			buf[0] = STATUS_EXEC_ERROR;
		}
}
