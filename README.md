소개
==============
- **장르** : 종스크롤 슈팅게임

- **내용** : 등장하는 적 비행기를 격추시키는 슈팅게임

- **환경** : STM32, LCD 모듈

------------
실행 방법
===
- **Tera Term VT**   
Tera Term 실행

- **확장자명 .bin 파일 Tera Term에 업로드**   
Alt + F, T, Y, S -> .bin 파일 선택 열기   

- **실행**   
아무 키 입력 시 Start   

-----
조작키    
===
**JOG** : 플레이어 상 하 좌 우 이동   
**SW0** : 총알 발사 (유지 가능)   
**SW1** : 폭탄 사용   

---------------
주요 기능   
====

- **게임 상태 전환**  
  시작 화면 → 플레이 화면 → 게임 오버 화면으로 전환
  - `STATE_START`, `STATE_PLAY`, `STATE_GAMEOVER`의 상태 기반 스위칭
  - 화면 잔상 제거를 위한 `Lcd_Clr_Screen()`과 플래그 사용

- **Object**  
  - Player, Player_Bullet, Enemy, Enemy_Bullet, Item(Bomb, Up, Speed)   
  - `Bullet_Update()` 함수 내에서 Up 아이템 수에 따라 총알 수 증가 및 좌우 대칭 발사  
  - 화면 밖으로 나가는 총알과 적은 미리 검사해 deactivate 처리

- **Enemy 동작 구조**  
  - `Spawn_Enemy()` : x좌표와 stop_y 위치를 난수로 설정  
  - `Enemy_Update()` : 랜덤 위치에서 정지 후 랜덤 시간 뒤 아래로 이동  
  - 적은 랜덤하게 총알 발사 (fire_timer 사용)

- **Item 기능 구현**  
  - Bomb: 폭탄 사용 횟수 증가 (최대 3개)  
  - Up: 발사 총알 개수 증가 (최대 4개, 양 옆으로 확장)  
  - Speed: 이동속도 + 발사 속도 증가 (3단계 속도)

- **충돌 처리**  
  - `Collision_Update()`에서 Object 간 충돌 감지  
  - 총알과 적, 적 총알과 플레이어, 플레이어와 아이템, 플레이어와 적 간의 충돌 처리  
  - 충돌 시 점수 증가, 아이템 획득, 목숨 감소 등의 상태 반영

- **배경음 및 효과음**  
  - `Play_Background_Music()` 함수로 음악 재생 (TIM3 주파수 출력 기반)  

- **난수 제어**  
  - `srand(SysTick->VAL + TIM2->CNT + i * 37)` 방식으로 난수 초기화  
  - 적 x위치, 정지 위치(stop_y), 발사 타이밍을 다양하게 설정

- **최적화 기술**  
  - 잔상 제거: `Draw_Object()` 호출 시 이전 위치를 BLACK으로 지우는 방식  
  - 깜빡임 방지: 전체 화면 Clear 대신 각 오브젝트 개별적으로 Draw

- **점수 시스템**  
  - 적 격추 시 `score++` 처리
