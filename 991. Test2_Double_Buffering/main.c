#include "device_driver.h"
#include <stdlib.h>

#define LCDW            (320)
#define LCDH            (240)
#define X_MIN           (0)
#define X_MAX           (LCDW - 1)
#define Y_MIN           (0)
#define Y_MAX           (LCDH - 1)

#define TIMER_PERIOD    (10)
#define PLAYER_SIZE_X   (16)
#define PLAYER_SIZE_Y   (16)
#define PLAYER_SPEED    (5)
#define BULLET_MAX      (10)
#define BULLET_SPEED    (10)
#define ENEMY_MAX       (5)
#define ENEMY_SPEED     (1)
#define ENEMY_STOP_TIME (500)
#define SPAWN_INTERVAL (20)
#define ITEM_MAX        (5)
/* #define ITEM_SPEED      (1) */
#define SW0_PIN         (13) // PA13

static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};

extern volatile int TIM4_expired;
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
} BULLET;

// Enemy structure
typedef struct {
    int x, y;
    int active;
    int stop_timer;
    int moving_down;
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

void System_Init(void)
{
    Clock_Init();
    LED_Init();
    Key_Poll_Init();
    Uart1_Init(115200);

    SCB->VTOR = 0x08003000;
    SCB->SHCSR = 7<<16;
}

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

void Spawn_Enemy(void)
{
    for (int i = 0; i < ENEMY_MAX; i++)
    {
        if (!enemies[i].active)
        {
            enemies[i].x = (rand() % (LCDW - 20));
            enemies[i].y = 0;
            enemies[i].stop_timer = ENEMY_STOP_TIME;
            enemies[i].moving_down = 0;
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

void Enemy_Update(void)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
    {
        if (enemies[i].active)
        {
            if (!enemies[i].moving_down)
            {
                if (enemies[i].y < LCDH / 3)
                {
                    enemies[i].y += ENEMY_SPEED;
                }
                else
                {
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


void Item_Update(void)
{
    for (int i = 0; i < ITEM_MAX; i++)
    {
        if (items[i].active)
        {
            items[i].y += ITEM_SPEED;
            if (items[i].y > Y_MAX)
                items[i].active = 0;
        }
    }
}

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

void Fire_Bullet(void)
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = player_x + PLAYER_SIZE_X/2 - 2;
            bullets[i].y = player_y;
            bullets[i].active = 1;
            break;
        }
    }
}

void Bullet_Update(void)
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            bullets[i].y -= BULLET_SPEED;
            if (bullets[i].y < Y_MIN)
            {
                bullets[i].active = 0;
            }
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

void Game_Init(void)
{
    player_life = 3;
    bomb_item = 0;
    up_item = 0;
    speed_item = 0;
    player_x = LCDW/2 - PLAYER_SIZE_X/2;
    player_y = LCDH - PLAYER_SIZE_Y - 10;
    Lcd_Clr_Screen();
    Bullet_Init();
    Enemy_Init();
    Item_Init();
}

int Check_Collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    return (x1 < x2 + w2) && (x1 + w1 > x2) && (y1 < y2 + h2) && (y1 + h1 > y2);
}

void Draw_HUD(void)
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
}

void Fire_Enemy_Bullet(int ex, int ey)
{
    int i;
    for (i = 0; i < BULLET_MAX; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = ex + 6;
            bullets[i].y = ey + 16;
            bullets[i].active = 1;
            break;
        }
    }
}

void Collision_Update(void)
{
    int i, j;

    for (i = 0; i < BULLET_MAX; i++)
    {
        if (bullets[i].active)
        {
            for (j = 0; j < ENEMY_MAX; j++)
            {
                if (enemies[j].active && Check_Collision(bullets[i].x, bullets[i].y, 4, 8, enemies[j].x, enemies[j].y, 16, 16))
                {
                    bullets[i].active = 0;
                    enemies[j].active = 0;
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
            else if (items[i].type == ITEM_UP && up_item < 5)
                up_item++;
            else if (items[i].type == ITEM_SPEED && speed_item < 3)
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
        switch (Jog_key)
        {
            case 0: if (player_y > Y_MIN) player_y -= move_speed; break;
            case 1: if (player_y + PLAYER_SIZE_Y < Y_MAX) player_y += move_speed; break;
            case 2: if (player_x > X_MIN) player_x -= move_speed; break;
            case 3: if (player_x + PLAYER_SIZE_X < X_MAX) player_x += move_speed; break;
        }
    }

    if (fire_delay > 0) fire_delay--;

    int fire_interval = (speed_item > 0) ? 5 : 10;
    int bullets_to_fire = (up_item > 0) ? (1 + up_item) : 1;

    if ((GPIOA->IDR & (1 << SW0_PIN)) == 0 && fire_delay == 0)
    {
        int i;
        for (i = 0; i < bullets_to_fire; i++)
        {
            Fire_Bullet();
        }
        fire_delay = fire_interval;
    }

    if ((GPIOA->IDR & (1 << 14)) == 0 && bomb_item > 0)
    {
        bomb_item--;
        Use_Bomb();
    }

    Bullet_Update();
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

    if (enemy_fire_timer > 50)
    {
        int i;
        for (i = 0; i < ENEMY_MAX; i++)
        {
            if (enemies[i].active)
            {
                Fire_Enemy_Bullet(enemies[i].x, enemies[i].y);
            }
        }
        enemy_fire_timer = 0;
    }
}

void Game_Draw(void)
{
    Lcd_Clr_Screen();
    Lcd_Draw_Box(player_x, player_y, PLAYER_SIZE_X, PLAYER_SIZE_Y, color[0]);
    Bullet_Draw();
    Enemy_Draw();
    Item_Draw();
    Draw_HUD();
}

void Start_Screen(void)
{
    Lcd_Clr_Screen();
    Lcd_Printf(60, 100, WHITE, BLACK, 2, 2, "STREET FROG EX");
    Lcd_Printf(80, 160, GREEN, BLACK, 1, 1, "Press any key to start");
}

void Game_Over_Screen(void)
{
    Lcd_Clr_Screen();
    Lcd_Printf(80, 100, RED, BLACK, 2, 2, "GAME OVER");
    Lcd_Printf(60, 160, BLUE, BLACK, 1, 1, "Press any key to retry");
}

void Main(void)
{
    System_Init();
    Uart_Printf("Street Frog EX Shooter Start!\n");

    Lcd_Init(3);
    Jog_Poll_Init();
    Jog_ISR_Enable(1);
    Uart1_RX_Interrupt_Enable(1);
    SysTick_Run(1);

    while (1)
    {
        switch (game_state)
        {
        case STATE_START:
            Start_Screen();
            if (Jog_key_in)
            {
                Jog_key_in = 0;
                game_state = STATE_PLAY;
                Game_Init();
                TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*10);
            }
            break;

        case STATE_PLAY:
            Game_Update();
            Game_Draw();
            break;

        case STATE_GAMEOVER:
            Game_Over_Screen();
            if (Jog_key_in)
            {
                Jog_key_in = 0;
                game_state = STATE_START;
            }
            break;
        }
    }
}
