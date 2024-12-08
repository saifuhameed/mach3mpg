/*
 * button.h
 *
 *  Created on: Nov 16, 2024
 *      Author: Saifu
 */
uint16_t _debounse_time=50;
GPIO_TypeDef *_button_port;
GPIO_TypeDef *_pinA_port;
GPIO_TypeDef *_pinB_port;
uint16_t _button_pin=0;
uint16_t _pinA=0;
uint16_t _pinB=0;

uint8_t state = 0;

extern TIM_HandleTypeDef htim1;

typedef  void(*CallBackFunc)(uint8_t,uint16_t);

CallBackFunc mCallBackFunc;
uint16_t long_click=0;
uint16_t _long_press_duration=2000; //in milli seconds

char rotation_type=0;
uint16_t rotation_count=0;

#define IS_PIN_LOW(p,n) 	HAL_GPIO_ReadPin(p,n)==GPIO_PIN_RESET
#define IS_PIN_HIGH(p,n) 	HAL_GPIO_ReadPin(p,n)==GPIO_PIN_SET
void button_loop();

uint32_t click_start_pos=0;
uint32_t click_elapsed=0;
enum{

	BUTTON_UP, //
	BUTTON_DOWN,
	BUTTON_PRESSED, //Brief press
	BUTTON_LONG_PRESSED, // Long press,
	ENCODER_FORWARD, // Rotory Encoder Rotating Forward
	ENCODER_BACKWARD // Rotory Encoder Rotating Backward
};

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == _button_pin && state == GPIO_PIN_RESET){
		click_start_pos=HAL_GetTick();
		HAL_TIM_Base_Start_IT(&htim1);
		state = 1;
		mCallBackFunc(BUTTON_DOWN,1);
	}
	else if(GPIO_Pin == _pinA && IS_PIN_LOW(_pinB_port,_pinB)  ){
		if(rotation_type==ENCODER_FORWARD)rotation_count=0;
		rotation_type=ENCODER_BACKWARD ;
		rotation_count++;
		mCallBackFunc(ENCODER_BACKWARD,rotation_count);
	}
	else if(GPIO_Pin == _pinB && IS_PIN_LOW(_pinA_port, _pinA) ){
		if(rotation_type==ENCODER_BACKWARD )rotation_count=0;
		rotation_type=ENCODER_FORWARD;
		rotation_count++;
		mCallBackFunc(ENCODER_FORWARD,rotation_count);
	}
	else{
		__NOP();
	}
}


void HAL_TIM_PeriodElapsedCallback_TIM1()
{
	if(IS_PIN_HIGH(_button_port, _button_pin) ){

		state = 0;
		if(long_click<_long_press_duration){
			mCallBackFunc(BUTTON_PRESSED,0);
		}
		mCallBackFunc(BUTTON_UP,1);
		long_click=0;
		HAL_TIM_Base_Stop_IT(&htim1);
	}else{
		if(long_click<_long_press_duration){
			long_click+=_debounse_time;
			if(long_click>=_long_press_duration){ //Long click
				mCallBackFunc(BUTTON_LONG_PRESSED,0);
			}
		}

	}


}


void setup_Button(GPIO_TypeDef *button_port,uint16_t button_pin,GPIO_TypeDef *pinA_port,uint16_t pinA,GPIO_TypeDef *pinB_port,uint16_t pinB,  CallBackFunc  cb, uint16_t debounce_time/* in milli seconds*/, uint16_t long_press /* in milli seconds*/)
{
	_button_port=button_port;
	_button_pin=button_pin;

	_pinA_port=pinA_port;
	_pinA=pinA;

	_pinB_port=pinB_port;
	_pinB=pinB;

	mCallBackFunc=cb;
	_debounse_time=debounce_time;
	_long_press_duration=(long_press/_debounse_time)*_debounse_time;
}



