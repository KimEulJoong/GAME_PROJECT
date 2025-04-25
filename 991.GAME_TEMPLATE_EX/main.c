#include "device_driver.h"

#define LCDW			(320)
#define LCDH			(240)
#define X_MIN	 		(0)
#define X_MAX	 		(LCDW - 1)
#define Y_MIN	 		(0)
#define Y_MAX	 		(LCDH - 1)

#define TIMER_PERIOD	(10)

static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};

extern volatile int TIM4_expired;
extern volatile int Jog_key_in;
extern volatile int Jog_key;

void System_Init(void)
{
	Clock_Init();
	LED_Init();
	Key_Poll_Init();
	Uart1_Init(115200);

	SCB->VTOR = 0x08003000;
	SCB->SHCSR = 7<<16;
}

#define DIPLAY_MODE		3

void Main(void)
{
	System_Init();
	Uart_Printf("Game Project\n");

	Lcd_Init(DIPLAY_MODE);
	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);
	Lcd_Clr_Screen();

	int frog_pos_x = (LCDW / 2) - 5, frog_pos_y = Y_MAX - 10;
	int car_pos_x = 0, car_pos_y = (LCDH / 2) - 5;
	int car_color = 1;
	int tick_count = 0;

	TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*10);

	Lcd_Printf(0,0,BLUE,WHITE,1,1,"Hello %d", 100);

	Lcd_Draw_Box(frog_pos_x, frog_pos_y, 10, 10, color[0]);
	Lcd_Draw_Box(car_pos_x, car_pos_y, 20, 20, color[0]);

	for(;;)
	{
		if(Jog_key_in) 
		{
			Uart_Printf("KEY = %d\n", Jog_key);
			Lcd_Printf(200,0,RED,BLACK,2,2,"KEY = %d", Jog_key);
			Lcd_Draw_Box(frog_pos_x, frog_pos_y, 10, 10, color[Jog_key]);
			Jog_key_in = 0;
		}

		if(TIM4_expired) 
		{
			tick_count = (tick_count + 1) % 10;

			if(tick_count == 0) 
			{
				Lcd_Draw_Box(car_pos_x, car_pos_y, 20, 20, color[car_color]);
				car_color = (car_color + 1) % (sizeof(color)/sizeof(color[0]));
			}
			
			TIM4_expired = 0;
		}
	}
}
