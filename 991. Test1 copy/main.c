#include "device_driver.h"
#include <stdlib.h>
#include <stdio.h>

//------------------------------------------define---------------------------------------------//
#define LCDW            (320)
#define LCDH            (240)
#define X_MIN           (0)
#define X_MAX           (LCDW - 1)
#define Y_MIN           (0)
#define Y_MAX           (LCDH - 1)

#define TIMER_PERIOD    (10)
#define PLAYER_SIZE_X   (16)
#define PLAYER_SIZE_Y   (16)
#define PLAYER_SPEED    (10)
#define BULLET_MAX      (20)
#define BULLET_SPEED    (10)
#define ENEMY_MAX       (5)
#define ENEMY_SPEED     (1)
#define ENEMY_BULLET_SPEED (4)

#define ENEMY_STOP_TIME (100 + rand() % 150)
#define SPAWN_INTERVAL (75)
#define ITEM_MAX        (3)

#define OWNER_PLAYER (0)
#define OWNER_ENEMY  (1)

#define ENEMY_STOP_MIN (LCDH / 7)
#define ENEMY_STOP_MAX (LCDH / 3)
#define ENEMY_RANDOM_STOP(i) \
({ \
    int levels[] = {LCDH/7, LCDH/6, LCDH/5, LCDH/4, LCDH/3}; \
    levels[(SysTick->VAL + TIM2->CNT + (i * 19)) % 5]; \
})

/* #define ITEM_SPEED      (1) */
#define SW0_PIN         (13) // PA13
#define BASE  (500) //msec

//----------------------------------------전역변수----------------------------------------------//
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};
static int score = 0;
static int bomb_used = 0;

/* extern volatile int TIM4_expired; */
extern volatile int Jog_key_in;
extern volatile int Jog_key;

typedef enum {
    STATE_START,
    STATE_PLAY,
    STATE_GAMEOVER
} GAME_STATE;

typedef enum {
    ITEM_BOMB,
    ITEM_UP,
    ITEM_SPEED
} ITEM_TYPE;

static GAME_STATE game_state = STATE_START;
static unsigned int player_life = 3;
static unsigned int bomb_item = 0;
static unsigned int up_item = 0;
static unsigned int speed_item = 0;

static int player_x = LCDW/2 - PLAYER_SIZE_X/2;
static int player_y = LCDH - PLAYER_SIZE_Y - 10;

// Bullet structure
typedef struct {                                 
    int x, y;
    int active;
    int owner;  // 0:player, 1:enemy
} BULLET;

// Enemy structure
typedef struct {                                
    int x, y;
    int active;
    int stop_timer;
    int moving_down;
    int stop_y;
    int fire_timer;
} ENEMY;

// Item structure
typedef struct {                                   
    int x, y;
    int active;
    ITEM_TYPE type;
} ITEM;

static BULLET bullets[BULLET_MAX];
static ENEMY enemies[ENEMY_MAX];
static ITEM items[ITEM_MAX];

//-------------------------------------------BGM------------------------------------------------//
volatile int song_index = 0;
volatile int note_timer = 0;

const static unsigned short tone_value[] = {
    261,277,293,311,329,349,369,391,415,440,466,493,
    523,554,587,622,659,698,739,783,830,880,932,987
};

enum key {C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1,
          C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2, REST};
enum note{N16=BASE/4, N8=BASE/2, N4=BASE, N2=BASE*2, N1=BASE*4};

extern volatile int note_timer;      
extern volatile int song_index;       

const int song1[][2] = {
    {E1, N8}, {G1, N8}, {A1, N8}, {C2, N8},
    {B1, N8}, {A1, N8}, {G1, N8}, {E1, N8},

    {F1, N8}, {A1, N8}, {C2, N8}, {D2, N8},
    {C2, N8}, {A1, N8}, {F1, N8}, {REST, N8}
};
const char * note_name[] = {"C1", "C1#", "D1", "D1#", "E1", "F1", "F1#", "G1", "G1#", "A1", "A1#", "B1", "C2", "C2#", "D2", "D2#", "E2", "F2", "F2#", "G2", "G2#", "A2", "A2#", "B2"};

void Play_Background_Music(void)
{
    if (note_timer > 0)
    {
        note_timer--;
        return;
    }

    int tone = song1[song_index][0];
    int duration = song1[song_index][1];

    if (tone != REST)
        TIM3_Out_Freq_Generation(tone_value[tone]);
    else
        TIM3_Out_Stop();

    note_timer = duration / 10; 
    song_index++;

    if (song_index >= sizeof(song1)/sizeof(song1[0]))
        song_index = 0;
}

// ----------------------------------------System_Init-------------------------------------------//
void System_Init(void)
{
    song_index = 0;
    note_timer = 0;

    Clock_Init();
    LED_Init();
    Key_Poll_Init();
    Uart1_Init(115200);
    TIM3_Out_Init();
    TIM4_10ms_Interrupt_Init();

    SCB->VTOR = 0x08003000;
    SCB->SHCSR = 7<<16;

    srand(SysTick->VAL + TIM2->CNT);
}

// ----------------------------------------Init-------------------------------------------//
void Bullet_Init(void)
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        bullets[i].active = 0;
    }
}

void Enemy_Init(void)
{
    for (int i = 0; i < ENEMY_MAX; i++)
    {
        enemies[i].active = 0;
    }
}

void Item_Init(void)
{
    for (int i = 0; i < ITEM_MAX; i++)
    {
        items[i].active = 0;
    }
}

// ----------------------------------------Draw_Object------------------------------------//
static void Draw_Object(int x, int y, int w, int h, int ci)
{
    Lcd_Draw_Box(x, y, w, h, color[ci]);
}

//-------------------------------------------Spawn----------------------------------------------//
void Spawn_Enemy(void)
{
    for (int i = 0; i < ENEMY_MAX; i++)
    {
        if (!enemies[i].active)
        {
            enemies[i].x = (rand() % (LCDW - 20));
            enemies[i].y = 0;
            enemies[i].stop_timer = ENEMY_STOP_TIME;
            enemies[i].fire_timer = 20 + (rand() % 60);
            enemies[i].moving_down = 0;

            srand(SysTick->VAL + TIM2->CNT + i * 37);
            enemies[i].stop_y = ENEMY_RANDOM_STOP(i); 

            enemies[i].active = 1;
            break;
        }
    }
}

void Spawn_Item(void)
{
    for (int i = 0; i < ITEM_MAX; i++)
    {
        if (!items[i].active)
        {
            items[i].x = (rand() % (LCDW - 10));
            items[i].y = 0;
            items[i].type = (ITEM_TYPE)(rand() % 3);
            items[i].active = 1;
            break;
        }
    }
}

// -------------------------------------Update : Object 행동 기술-------------------------------------//
void Bullet_Enemy_Update(int ex, int ey)
{
    int i;
    for (i = 0; i < BULLET_MAX; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].owner = OWNER_ENEMY;
            bullets[i].x = ex + 6;
            bullets[i].y = ey + 16;
            bullets[i].active = 1;
            break;
        }
    }
}

void Enemy_Update(void)
{
    for (int i = 0; i < ENEMY_MAX; i++)
    {
        if (enemies[i].active)
        {
            Draw_Object(enemies[i].x, enemies[i].y, 16, 16, 5);

            if (!enemies[i].moving_down)
            {
                if (enemies[i].y < enemies[i].stop_y)
                {
                    enemies[i].y += ENEMY_SPEED;
                }
                else
                {
                    if (enemies[i].fire_timer > 0)
                    {
                        enemies[i].fire_timer--;
                    }
                    else if (enemies[i].y + 16 <= Y_MAX)
                    {
                        Bullet_Enemy_Update(enemies[i].x, enemies[i].y);
                        enemies[i].fire_timer = 150 + (rand() % 100); 
                    }

                    if (enemies[i].stop_timer > 0)
                        enemies[i].stop_timer--;
                    else
                        enemies[i].moving_down = 1;
                }
            }
            else
            {
                enemies[i].y += ENEMY_SPEED;
                if (enemies[i].y > Y_MAX)
                    enemies[i].active = 0;
            }
        }
    }
}


void Bullet_Update(int cnt)
{
    int center_x = player_x + PLAYER_SIZE_X / 2 - 2;  // 총알 기준 위치 중심

    for (int i = 0; i < cnt; i++)
    {
        int offset = (i - (cnt - 1) / 2) * 10;  // 좌우 대칭으로 수정 (-20, -10, 0, +10, +20 )
        int bullet_x = center_x + offset;  //총알 범위 검사

        if (bullet_x < X_MIN || bullet_x > X_MAX - 4)   //화면을 벗어난 총알 생성 금지
        continue;

        for (int j = 0; j < BULLET_MAX; j++)
        {
            if (!bullets[j].active)
            {
                bullets[j].owner = OWNER_PLAYER;
                bullets[j].x = bullet_x;
                bullets[j].y = player_y;
                bullets[j].active = 1;
                break;
            }
        }
    }
}

void Item_Update(void)
{
    for (int i = 0; i < ITEM_MAX; i++)
    {
        if (items[i].active)
        {
            Draw_Object(items[i].x, items[i].y, 10, 10, 5);
            items[i].y += ITEM_SPEED;
            if (items[i].y > Y_MAX)
                items[i].active = 0;
        }
    }
}

//충돌 체크 선언 및 충돌 업데이트
int Check_Collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    return (x1 < x2 + w2) && (x1 + w1 > x2) && (y1 < y2 + h2) && (y1 + h1 > y2);
}

void Collision_Update(void)
{
    int i, j;

    for (i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active && bullets[i].owner == OWNER_PLAYER)
        {
            for (j = 0; j < ENEMY_MAX; j++)
            {
                if (enemies[j].active && Check_Collision(bullets[i].x, bullets[i].y, 4, 8, enemies[j].x, enemies[j].y, 16, 16))
                {
                    bullets[i].active = 0;
                    enemies[j].active = 0;
                    score++;
                    break;
                }
            }
        }
        
    }

    for (i = 0; i < ITEM_MAX; i++)
    {
        if (items[i].active && Check_Collision(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, items[i].x, items[i].y, 10, 10))
        {
            if (items[i].type == ITEM_BOMB && bomb_item < 3)
                bomb_item++;
            else if (items[i].type == ITEM_UP && up_item < 3)
                up_item++;
            else if (items[i].type == ITEM_SPEED && speed_item < 2)
                speed_item++;
            items[i].active = 0;
        }
    }

    for (i = 0; i < ENEMY_MAX; i++)
    {
        if (enemies[i].active && Check_Collision(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, enemies[i].x, enemies[i].y, 16, 16))
        {
            enemies[i].active = 0;
            if (player_life > 0)
                player_life--;
        }
    }

    for (i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            if (Check_Collision(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, bullets[i].x, bullets[i].y, 4, 8))
            {
                bullets[i].active = 0;
                if (player_life > 0)
                    player_life--;
            }
        }
    }
}

//---------------------------------------------Draw -------------------------------------------//
void Enemy_Draw(void)
{
    for (int i = 0; i < ENEMY_MAX; i++)
    {
        if (enemies[i].active)
        {
            Lcd_Draw_Box(enemies[i].x, enemies[i].y, 16, 16, color[1]);
        }
    }
}

void Bullet_Draw(void)
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            Lcd_Draw_Box(bullets[i].x, bullets[i].y, 4, 8, color[2]);
        }
    }
}

void Item_Draw(void)
{
    for (int i = 0; i < ITEM_MAX; i++)
    {
        if (items[i].active)
        {
            unsigned short item_color = (items[i].type == ITEM_BOMB) ? RED : (items[i].type == ITEM_UP) ? YELLOW : GREEN;
            Lcd_Draw_Box(items[i].x, items[i].y, 10, 10, item_color);
        }
    }
}

//------------------------------------Game_Init---------------------------------------------//
void Game_Init(void)
{
    player_life = 3;
    bomb_item = 0;
    up_item = 0;
    speed_item = 0;
    player_x = LCDW/2 - PLAYER_SIZE_X/2;
    player_y = LCDH - PLAYER_SIZE_Y - 10;
    score = 0;

    Lcd_Clr_Screen();
    Bullet_Init();
    Enemy_Init();
    Item_Init();
}

//--------------------------------------추가 기능 ----------------------------------------------//

void HUD_Draw(void)
{
    Lcd_Printf(200, 220, WHITE, BLACK, 1, 1, "B:%d U:%d S:%d", bomb_item, up_item, speed_item);
    Lcd_Printf(0, 220, WHITE, BLACK, 1, 1, "LIFE:%d", player_life);
}

void Use_Bomb(void)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        enemies[i].active = 0;
    for (i = 0; i < BULLET_MAX; i++)
        bullets[i].active = 0;
    Lcd_Clr_Screen();
}

// ------------------------------------------Ingame play 작동 ----------------------------------------//
void Game_Update(void)
{
    static int fire_delay = 0;
    static int move_speed = PLAYER_SPEED;
    static int spawn_timer = 0;
    static int item_spawn_timer = 0;
    static int enemy_fire_timer = 0;

    if (player_life == 0)
    {
        game_state = STATE_GAMEOVER;
    }

    move_speed = (speed_item > 0) ? (PLAYER_SPEED + 2) : PLAYER_SPEED;

    if (Jog_key_in)
    {
        Jog_key_in = 0;
        Draw_Object(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, 5);

        switch (Jog_key)
        {
            case 0: if (player_y > Y_MIN) player_y -= move_speed; break;
            case 1: if (player_y + PLAYER_SIZE_Y < Y_MAX) player_y += move_speed; break;
            case 2: if (player_x > X_MIN) player_x -= move_speed; break;
            case 3: if (player_x + PLAYER_SIZE_X < X_MAX) player_x += move_speed; break;
        }
    }

    if (fire_delay > 0) fire_delay--;

    int fire_interval;

    switch (speed_item)
    {
        case 0: fire_interval = 15; break;
        case 1: fire_interval = 10; break;
        case 2: fire_interval = 6; break;
        case 3: fire_interval = 3; break;
        default: fire_interval = 15; break;
    }

    int bullets_to_fire = (up_item > 0) ? (1 + up_item) : 1;

    if ((GPIOA->IDR & (1 << SW0_PIN)) == 0 && fire_delay == 0)
    {
        Bullet_Update(bullets_to_fire);

        fire_delay = fire_interval;
    }

    if ((GPIOA->IDR & (1 << 14)) == 0)
    {
        if (!bomb_used && bomb_item > 0)
        {
            bomb_item--;
            Use_Bomb();
            bomb_used = 1;
        }
    }
    else
    {
        bomb_used = 0;
    } 

    int i;
    for (i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            Draw_Object(bullets[i].x, bullets[i].y, 4, 8, 5); // 이전 위치 지우기

            if (bullets[i].owner == OWNER_PLAYER)
            {
                bullets[i].y -= BULLET_SPEED; // 플레이어 총알은 위로
            }
            else if (bullets[i].owner == OWNER_ENEMY)
            {
                bullets[i].y += ENEMY_BULLET_SPEED; // 적 총알은 아래로
            }
    
            // 화면을 벗어나면 비활성화
            if (bullets[i].y < Y_MIN || bullets[i].y > Y_MAX)
                bullets[i].active = 0;
        }
    }

    Enemy_Update();
    Item_Update();
    

    Collision_Update();

    spawn_timer++;
    item_spawn_timer++;
    enemy_fire_timer++;

    if (spawn_timer > SPAWN_INTERVAL)
    {
        Spawn_Enemy();
        spawn_timer = 0;
    }

    if (item_spawn_timer > 300)
    {
        Spawn_Item();
        item_spawn_timer = 0;
    }

    if (enemy_fire_timer > 100)
    {
        for (i = 0; i < ENEMY_MAX; i++)
        {
            if (enemies[i].active && enemies[i].y + 16 <= Y_MAX)  // 화면 안에 있는지 체크
            {
                Bullet_Enemy_Update(enemies[i].x, enemies[i].y);
            }
        }
        enemy_fire_timer = 0;
    }
}

void Play_Screen(void)
{
    Draw_Object(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, 2);

    int i;
    for (i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            int color_index = (bullets[i].owner == OWNER_PLAYER) ? 3 : 1;
            Draw_Object(bullets[i].x, bullets[i].y, 4, 8, color_index);
        }
    }

    for (i = 0; i < ENEMY_MAX; i++)
    {
        if (enemies[i].active)
        {
            Draw_Object(enemies[i].x, enemies[i].y, 16, 16, 0);
        }
    }

    for (i = 0; i < ITEM_MAX; i++)
    {
        if (items[i].active)
        {
            unsigned short item_color_index = (items[i].type == ITEM_BOMB) ? 2 : (items[i].type == ITEM_UP) ? 3 : 4;
            Draw_Object(items[i].x, items[i].y, 10, 10, item_color_index);
        }
    }

    HUD_Draw();
}

// ------------------------------------- Start, Game_Over_Screen-------------------------------//
void Start_Screen(void)
{
    
    Lcd_Printf(65, 90, WHITE, BLACK, 2, 2, "Pixel_Wing");
    Lcd_Printf(70, 160, GREEN, BLACK, 1, 1, "Press any key to start");
}

void Game_Over_Screen(void)
{
    char buf[32];
    sprintf(buf, "score: %d", score);
    Lcd_Printf(80, 90, RED, BLACK, 2, 2, "GAME OVER");
    Lcd_Printf(120, 130, YELLOW, BLACK, 1, 1, buf);
    Lcd_Printf(65, 160, BLUE, BLACK, 1, 1, "Press any key to retry");
}

// ----------------------------------------Main--------------------------------------------//
void Main(void)
{
    System_Init();
    Uart_Printf("Pixel_Wing Start!\n");

    Lcd_Init(3);
    Jog_Poll_Init();
    Jog_ISR_Enable(1);
    Uart1_RX_Interrupt_Enable(1);
    SysTick_Run(1);

    static int screen_clr = 0;

    while (1)
    {
        switch (game_state)
        {
            case STATE_START:
                if (!screen_clr) {
                    Lcd_Clr_Screen();
                    screen_clr = 1;
                }
                Start_Screen();
                if (Jog_key_in)
                {
                    Jog_key_in = 0;
                    game_state = STATE_PLAY;
                    screen_clr = 0;  
                    Game_Init();
                    /* TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*10); */
                }
                break;
    
            case STATE_PLAY:
                Game_Update();
                Play_Screen();
                break;
    
            case STATE_GAMEOVER:
                if (!screen_clr) {
                    Lcd_Clr_Screen();
                    screen_clr = 1;
                }
                Game_Over_Screen();
                if (Jog_key_in)
                {
                    Jog_key_in = 0;
                    game_state = STATE_START;
                    screen_clr = 0;
                }
                break;
        }
    }
    
}
