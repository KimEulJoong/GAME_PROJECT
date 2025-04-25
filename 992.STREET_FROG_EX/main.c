#include "device_driver.h"

#define LCDW			(320)
#define LCDH			(240)
#define X_MIN	 		(0)
#define X_MAX	 		(LCDW - 1)
#define Y_MIN	 		(0)
#define Y_MAX	 		(LCDH - 1)

#define TIMER_PERIOD	(10)
#define RIGHT			(1)
#define LEFT			(-1)
#define HOME			(0)
#define SCHOOL			(1)

#define CAR_STEP		(10)
#define CAR_SIZE_X		(20)
#define CAR_SIZE_Y		(20)
#define FROG_STEP		(10)
#define FROG_SIZE_X		(20)
#define FROG_SIZE_Y		(20)

#define BACK_COLOR		(5)
#define CAR_COLOR		(0)
#define FROG_COLOR		(1)

#define GAME_OVER		(1)

typedef struct
{
	int x,y;
	int w,h;
	int ci;
	int dir;
}QUERY_DRAW;

static QUERY_DRAW car;
static QUERY_DRAW frog;

static int score;
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};

static int Check_Collision(void)
{
	int col = 0;

	if((car.x >= frog.x) && ((frog.x + FROG_STEP) >= car.x)) col |= 1<<0;
	else if((car.x < frog.x) && ((car.x + CAR_STEP) >= frog.x)) col |= 1<<0;
	
	if((car.y >= frog.y) && ((frog.y + FROG_STEP) >= car.y)) col |= 1<<1;
	else if((car.y < frog.y) && ((car.y + CAR_STEP) >= frog.y)) col |= 1<<1;

	if(col == 3)
	{
		Uart_Printf("SCORE = %d\n", score);	
		return GAME_OVER;
	}

	if((frog.dir == SCHOOL) && (frog.y == Y_MIN)) 
	{
		Uart_Printf("SCHOOL\n");		
		frog.dir = HOME;
	}

	if((frog.dir == HOME) && (frog.y == LCDH - frog.h))
	{
		frog.dir = SCHOOL;
		score++;
		Uart_Printf("HOME, %d\n", score);
		Lcd_Printf(0,0,BLUE,WHITE,2,2,"%d", score);
	}

	return 0;
}

static int Car_Move(void)
{
	car.x += CAR_STEP * car.dir;
	if((car.x + car.w >= X_MAX) || (car.x <= X_MIN)) car.dir = -car.dir;
	return Check_Collision();
}

static void k0(void)
{
	if(frog.y > Y_MIN) frog.y -= FROG_STEP;
}

static void k1(void)
{
	if(frog.y + frog.h < Y_MAX) frog.y += FROG_STEP;
}

static void k2(void)
{
	if(frog.x > X_MIN) frog.x -= FROG_STEP;
}

static void k3(void)
{
	if(frog.x + frog.w < X_MAX) frog.x += FROG_STEP;
}

static int Frog_Move(int k)
{
	// UP(0), DOWN(1), LEFT(2), RIGHT(3)
	static void (*key_func[])(void) = {k0, k1, k2, k3};
	if(k <= 3) key_func[k]();
	return Check_Collision();
}

static void Game_Init(void)
{
	score = 0;
	Lcd_Clr_Screen();
	frog.x = 150; frog.y = 220; frog.w = FROG_SIZE_X; frog.h = FROG_SIZE_Y; frog.ci = FROG_COLOR; frog.dir = SCHOOL;
	car.x = 0; car.y = 110; car.w = CAR_SIZE_X; car.h = CAR_SIZE_Y; car.ci = CAR_COLOR; car.dir = RIGHT;
	Lcd_Draw_Box(frog.x, frog.y, frog.w, frog.h, color[frog.ci]);
	Lcd_Draw_Box(car.x, car.y, car.w, car.h, color[car.ci]);
}

static void Draw_Object(QUERY_DRAW * obj)
{
	Lcd_Draw_Box(obj->x, obj->y, obj->w, obj->h, color[obj->ci]);
}

extern volatile int TIM4_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
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
	Uart_Printf("Street Froggy\n");

	Lcd_Init(DIPLAY_MODE);

	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);

	for(;;)
	{
		Game_Init();
		TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*10);
		Lcd_Printf(0,0,BLUE,WHITE,2,2,"%d", score);

		for(;;)
		{
			int game_over = 0;

			if(Jog_key_in) 
			{
				frog.ci = BACK_COLOR;
				Draw_Object(&frog);
				game_over = Frog_Move(Jog_key);
				frog.ci = FROG_COLOR;
				Draw_Object(&frog);
				Jog_key_in = 0;
			}

			if(TIM4_expired) 
			{
				car.ci = BACK_COLOR;
				Draw_Object(&car);
				game_over = Car_Move();
				car.ci = CAR_COLOR;
				Draw_Object(&car);
				TIM4_expired = 0;
			}

			if(game_over)
			{
				TIM4_Repeat_Interrupt_Enable(0, 0);
				Uart_Printf("Game Over, Please press any key to continue.\n");
				Jog_Wait_Key_Pressed();
				Jog_Wait_Key_Released();
				Uart_Printf("Game Start\n");
				break;
			}
		}
	}
}
