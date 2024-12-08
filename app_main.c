
#include "main.h"
#include "button.h"

uint16_t counter=0;
int32_t last_tick=0;
int32_t new_rot_pulse_width=0;
float rot_pulse_width=0;
int8_t direction=0;
uint8_t click_type=0;

enum{
	AXIS_OFF,
	AXIS_X,
	AXIS_Y,
	AXIS_Z
};


enum{
	SPEED_X1,
	SPEED_X10,
	SPEED_X100
};

uint8_t selected_axis	=	AXIS_OFF;
uint8_t selected_speed	=	SPEED_X1;



#define SET_Pin(p,n)	HAL_GPIO_WritePin(p, n,GPIO_PIN_SET)
#define RESET_Pin(p,n)	HAL_GPIO_WritePin(p, n,GPIO_PIN_RESET)

uint32_t pulse_width; //pulse duration
uint16_t pulse_slice_1;
uint16_t pulse_slice_2;
uint16_t pulse_slice_3;
uint16_t pulse_slice_4;
uint32_t countAB;
uint32_t pulse_required;

uint16_t AccelCurve[500]		=	{0};


void send_AB_pulses();
void recalculate_velocity(float velocity);

extern void button_loop();
extern TIM_HandleTypeDef htim3;
extern uint16_t rotation_count;
uint32_t pulse_count=0;

uint32_t current_speed=0;
float prev_velocity=0;

uint16_t required_pulses=0;
uint16_t current_pulse_index=0;
uint16_t accel_profile_count;


void reset_pulse_status(){
	countAB=0;
	counter=0;
	rotation_count=0;

	required_pulses=0;
	current_pulse_index=0;
}

void update_axis_selector(){
	reset_pulse_status();
	SET_Pin(GPIOA, AXIS_SEL_OUT_X_Pin);
	SET_Pin(GPIOB, AXIS_SEL_OUT_Y_Pin);
	SET_Pin(GPIOA, AXIS_SEL_OUT_Z_Pin);
	if(selected_axis==AXIS_X)RESET_Pin(GPIOA, AXIS_SEL_OUT_X_Pin);
	if(selected_axis==AXIS_Y)RESET_Pin(GPIOB, AXIS_SEL_OUT_Y_Pin); //PORTB
	if(selected_axis==AXIS_Z)RESET_Pin(GPIOA, AXIS_SEL_OUT_Z_Pin);
}

void update_speed_selector(){

	reset_pulse_status();
	SET_Pin(GPIOA, SPEED_SEL_OUT_1X_Pin); //1X Selector
	SET_Pin(GPIOA, SPEED_SEL_OUT_10X_Pin);
	SET_Pin(GPIOA, SPEED_SEL_OUT_100X_Pin);
	if(selected_speed==SPEED_X1)	RESET_Pin(GPIOA, 	SPEED_SEL_OUT_1X_Pin);
	if(selected_speed==SPEED_X10)	RESET_Pin(GPIOA, 	SPEED_SEL_OUT_10X_Pin);
	if(selected_speed==SPEED_X100)	RESET_Pin(GPIOA, 	SPEED_SEL_OUT_100X_Pin);

}

void buttonEvent(uint8_t _type, uint16_t data){
	if(_type==BUTTON_PRESSED){
		selected_axis++;
		if(selected_axis>AXIS_Z)selected_axis=AXIS_OFF;
		update_axis_selector();
	}
	if(_type==BUTTON_LONG_PRESSED){
		selected_speed++;
		if(selected_speed>SPEED_X100)selected_speed=SPEED_X1;
		update_speed_selector();
	}
	if(_type==ENCODER_FORWARD || _type==ENCODER_BACKWARD){

		if(_type==ENCODER_FORWARD)counter=counter+1 ;
		if(_type==ENCODER_BACKWARD)counter=counter-1 ;

		required_pulses+=4;
		rotation_count+=4;
		direction=_type;
		new_rot_pulse_width=HAL_GetTick()-last_tick;

		rot_pulse_width=(rot_pulse_width+new_rot_pulse_width)/2.0;

		if(rot_pulse_width>1000)rot_pulse_width=1000;

		last_tick=HAL_GetTick();

	}
}


void recalculate_velocity_from_accel_array(){
	uint16_t top_speed_index=(required_pulses<accel_profile_count)?required_pulses-1:accel_profile_count-1;
	uint16_t remaining_pulse= required_pulses-current_pulse_index;
	uint16_t acce_profile_index=  (remaining_pulse<top_speed_index)?remaining_pulse:current_pulse_index;
	acce_profile_index=acce_profile_index+5;
	if(acce_profile_index>=accel_profile_count)acce_profile_index=accel_profile_count-1;
	pulse_width=AccelCurve[acce_profile_index];
	pulse_slice_1=pulse_width/4;
	pulse_slice_2=pulse_width/2;
	pulse_slice_3=pulse_slice_1*3;
	pulse_slice_4=pulse_width;

}

void recalculate_velocity(float velocity){
	float new_velocity=(velocity+prev_velocity)/2;
	prev_velocity=new_velocity;
	pulse_width=3000/new_velocity; //4000
	pulse_slice_1=pulse_width/4;
	pulse_slice_2=pulse_width/2;
	pulse_slice_3=pulse_slice_1*3;
	pulse_slice_4=pulse_width;

}

void makeAccelCurveArray(){
	accel_profile_count=sizeof(AccelCurve)/sizeof(AccelCurve[0]);
	uint16_t feed_number=5;
	uint8_t offset=10;
	for (uint16_t i=0+offset;i<(accel_profile_count+offset);i++ ){
		feed_number=(feed_number+i+1)/2;
		AccelCurve[i-offset]=6000/feed_number;
	}
	feed_number=25;
}

void app_setup(){
	setup_Button(GPIOA,BUTTON_IN_Pin,GPIOA,ROT_ENC_IN_A_Pin, GPIOA,ROT_ENC_IN_B_Pin, (CallBackFunc)&buttonEvent,50,800);
	update_axis_selector();
	update_speed_selector();
	HAL_TIM_Base_Start_IT(&htim3);
	prev_velocity=10;

	RESET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
	RESET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
	makeAccelCurveArray();
	////accel_profile_count= sizeof(AccelCurve)/sizeof(AccelCurve[0]);
	//recalculate_velocity(prev_velocity);
	recalculate_velocity_from_accel_array();
}

void send_AB_pulses(){
	if(direction==ENCODER_FORWARD){
		if (countAB<pulse_slice_1){
			RESET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
		}else
		if (countAB>=pulse_slice_1 && countAB < pulse_slice_2){
			SET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
		}else
		if (countAB>=pulse_slice_2 && countAB < pulse_slice_3){
			RESET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
		}else
		if (countAB>=pulse_slice_3 && countAB < pulse_slice_4){
			RESET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
			RESET_Pin(GPIOA, SIGNAL_OUT_B_Pin);

		}
	}else if (direction==ENCODER_BACKWARD){
		if (countAB<pulse_slice_1){
			RESET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
		}else
		if (countAB>=pulse_slice_1 && countAB < pulse_slice_2){
			SET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
		}else
		if (countAB>=pulse_slice_2 && countAB < pulse_slice_3){
			RESET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
			SET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
		}else
		if (countAB>=pulse_slice_3 && countAB < pulse_slice_4){
			RESET_Pin(GPIOA, SIGNAL_OUT_B_Pin);
			RESET_Pin(GPIOA, SIGNAL_OUT_A_Pin);
		}
	}
	if (countAB>= pulse_slice_4){


		current_pulse_index++; //Used with accelration array
		recalculate_velocity_from_accel_array();
		countAB=0;

		/*
		uint16_t rpulse=rotation_count;
		if(rpulse<10)rpulse=5;
		if(rpulse>40)rpulse=40;
		uint16_t velocity=rpulse ;
		recalculate_velocity(velocity);
		rotation_count--;
		pulse_count++;
		if(rotation_count==0)pulse_count=0;
		 */





	}


}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM1)
	{
		HAL_TIM_PeriodElapsedCallback_TIM1();
	}
	if(htim->Instance==TIM3)
	{		

		if(current_pulse_index < required_pulses){
		 	countAB++;
		 	send_AB_pulses();
		 }else{
		 	required_pulses=0;
			current_pulse_index=0;
		}

		/*
		if(rotation_count>0 && pulse_width>0){
			countAB++;
			send_AB_pulses();
		}
		*/

	}
}


void app_main()
{

}
