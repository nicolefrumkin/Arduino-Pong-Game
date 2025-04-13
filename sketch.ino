#include <TVout.h>
#include "fontALL.h"
#include <avr/wdt.h>
#include <stdlib.h>

// PINs defines:
#define SCREEN_WIDTH    128  // Maximum width
#define SCREEN_HEIGHT   96   // Maximum height
#define PLAYER1_INPUT   A1   // Player 1 control input
#define PLAYER2_INPUT   A2   // Player 2 control input
#define TOTAL_POINTS    5 
#define MODE_SWITCH_PIN 5  // Pin connected to toggle switch


typedef struct {
  int x;
  int y;
} Vec;

typedef struct {
  Vec position;
  Vec speed;
  int size;
} Ball;

typedef struct {
  Vec position;
  Vec oldPos;
  bool dir; // only for computer. true is up
} Paddle;

typedef struct {
  unsigned long prev_time1;
  unsigned long prev_time2;
  unsigned long prev_time3;
  Ball ball;
  float paddle_size;
  int paddle_difficulty;
  Vec lastBallSpeed;
  int ball_size;
  char* mode;
  Paddle player1;
  Paddle player2;
  int score1;
  int score2;
  int speed;
  bool is_single_player;
  bool game_on;
  bool game_over;
} GameState;

//global vars
TVout TV;
GameState game;

void startup () {
  randomSeed(analogRead(0));
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP); // assumes toggle switch between pin and GND
  // Read game mode
  if (digitalRead(MODE_SWITCH_PIN) == LOW) {
    game.mode = "Single player Mode";
    game.is_single_player = true;
  } else {
    game.mode = "Duel Mode (1 vs 1)";
    game.is_single_player = false;
  }

  // initalize all variables
  game.game_on = false;  // Make sure game doesn't start immediately
  game.game_over = false;
  game.speed = 1;
  game.ball_size = 1;
  game.paddle_size = 12.5;  // Starting paddle size as float
  game.paddle_difficulty = 1;

  // Clear all struct variables to avoid garbage values
  Paddle player1 = {0};
  Paddle player2 = {0};
  Ball ball = {0};

  // Initialize last ball speed for first round
  game.lastBallSpeed.x = 1;
  game.lastBallSpeed.y = 1;

  // Initialize ball
  game.ball = ball;
  game.ball.size = 12;
  game.ball.speed.x = 1;
  game.ball.speed.y = 1;

  // Initialize player structs
  game.player1 = player1;
  game.player2 = player2;
  game.score1 = 0;
  game.score2 = 0;

  // Initialize player positions
  game.player1.position.x = 125;
  game.player2.position.x = 24;

  // Initialize oldPos to EXACTLY current position to prevent immediate game start
  game.player2.position.y = map(analogRead(PLAYER2_INPUT), 0, 1023, 10 + int(game.paddle_size), 87 - int(game.paddle_size));
  posPlayers();

  // Reset ball position
  game.ball.position.x = SCREEN_WIDTH / 2 + 10;
  game.ball.position.y = SCREEN_HEIGHT / 2;

  // print message

  TV.select_font(font6x8);
  TV.println(45, 30, "Welcome to");
  TV.select_font(font8x8);
  TV.println(35, 40, "Pong game");
  TV.select_font(font4x6);
  TV.println(20, 60, "the game mode you chose is:");
  //add game mode choice
  TV.println(40, 70, game.mode);
  soundGame(200, 200);
  delay(200);
  soundGame(300, 200);
  delay(200);
  soundGame(400, 200);
  TV.delay(4600);
}

void drawScene()
{
  // Draw the position of the players
  TV.clear_screen();
  TV.select_font(font6x8);
  TV.print(64, 0, game.score2);
  TV.print(80, 0, game.score1);
  TV.select_font(font4x6);
  int offset = 27;
  TV.print(offset, 90, "Speed:");
  TV.print(offset + 25, 90, game.speed);
  TV.print(offset + 35, 90, "Ball:");
  TV.print(offset + 55, 90, game.ball_size);
  TV.print(offset + 65, 90, "Paddle:");
  TV.print(offset + 95, 90, game.paddle_difficulty);

  // Draw rectangular square
  TV.draw_rect(22, 8, 105, 79, WHITE);

  // Draw the center column of the court
  for (int i = 9; i < SCREEN_HEIGHT - 10; i = i + 5) {
    TV.draw_column((SCREEN_WIDTH / 2) + 10, i, i + 2, WHITE);
  }

  TV.draw_column(game.player1.position.x,
                 (int)(game.player1.position.y - game.paddle_size),
                 (int)(game.player1.position.y + game.paddle_size), WHITE);

  TV.draw_column(game.player2.position.x,
                 (int)(game.player2.position.y - game.paddle_size),
                 (int)(game.player2.position.y + game.paddle_size), WHITE);


  // Draw ball
  TV.draw_circle(game.ball.position.x, game.ball.position.y, game.ball.size / 2, WHITE, WHITE);
}

void posPlayers()
{
  // Store old positions before updating
  game.player1.oldPos.y = game.player1.position.y;
  game.player2.oldPos.y = game.player2.position.y;

  // Update using float paddle_size but convert to integer for mapping
  game.player1.position.y = map(analogRead(PLAYER1_INPUT), 0, 1023, 10 + int(game.paddle_size), 87 - int(game.paddle_size));
  if (game.is_single_player) {
    if (game.game_on) {
      if (game.player2.dir) game.player2.position.y++;
      else game.player2.position.y--;

      if (game.player2.position.y >= 87 - int(game.paddle_size) || game.player2.position.y <= 9 + int(game.paddle_size)) {
        game.player2.dir = !game.player2.dir;
      }
    }
  }
  else {
    game.player2.position.y = map(analogRead(PLAYER2_INPUT), 0, 1023, 10 + int(game.paddle_size), 87 - int(game.paddle_size));
  }
}

void moveBall()
{
  if (game.game_on) {
    game.ball.position.x += game.ball.speed.x;
    game.ball.position.y += game.ball.speed.y;

    // when ball is touching the wall
    if (game.ball.position.y >= 86 - game.ball.size / 2 || game.ball.position.y <= 9 + game.ball.size / 2 ) { // ball touching bottom or top line
      game.ball.speed.y = -1 * game.ball.speed.y;
      soundGame(300, 30);
      delay(20); // for sound clearity
      soundGame(600, 30);
    }

    if ((game.ball.position.x >= 127 - game.ball.size / 2) && (game.ball.speed.x > 0)) { // ball touching player 1 side
      game.score2++;
      soundGame(400, 30);
      game.ball.speed.x = -1 * game.ball.speed.x;
    }

    // Collision detection for player 1 paddle
    if (game.ball.position.x >= game.player1.position.x - game.ball.size / 2 &&
        game.ball.position.x <= game.player1.position.x + 1
        && game.ball.speed.x > 0) { // ball touching player 1 paddle
      if (game.ball.position.y <= (game.player1.position.y + game.paddle_size) &&
          game.ball.position.y >= (game.player1.position.y - game.paddle_size)) {
        game.ball.speed.x = -1 * game.ball.speed.x;
        soundGame(500, 30);
      }
    }

    if ((game.ball.position.x <= 22 + game.ball.size / 2) && (game.ball.speed.x < 0)) { // ball touching player 2/comp side
      game.score1++;
      soundGame(400, 30);
      game.ball.speed.x = -1 * game.ball.speed.x;
    }

    // Collision detection for player 2 paddle
    if (game.ball.position.x >= game.player2.position.x - 1 &&
        game.ball.position.x <= game.player2.position.x + game.ball.size / 2
        && game.ball.speed.x < 0) { // ball touching player 2 paddle
      if (game.ball.position.y <= (game.player2.position.y + game.paddle_size) &&
          game.ball.position.y >= (game.player2.position.y - game.paddle_size)) {
        game.ball.speed.x = -1 * game.ball.speed.x;
        soundGame(500, 30);
      }
    }
  }
}


void checkMotion()
{
  posPlayers(); // update players location
  if (game.game_on == false) {
    if (game.is_single_player) {
      if (game.player1.oldPos.y != game.player1.position.y) { // player 1 moved, start game
        game.game_on = true;
        set_time();
        game.ball.speed.x = 1; // Positive = toward right
        game.ball.speed.y = random(0, 2) == 0 ? 1 : -1; // Random up/down
         
      }
    }
    else {
      if (game.player1.oldPos.y != game.player1.position.y || game.player2.oldPos.y != game.player2.position.y) {
        game.game_on = true;
        set_time();
        game.ball.speed.x = 1; // Positive = toward right
        game.ball.speed.y = random(0, 2) == 0 ? 1 : -1; // Random up/down
      }
    }
  }
}


void soundGame(int frequency, int duration)
{
  TV.tone(frequency, duration);
}

void gameover() {
  TV.clear_screen();
  if (game.is_single_player) {
    if (game.score1 > game.score2) {
      TV.select_font(font6x8);
      TV.println(25, 40, "Congratulations!");
      TV.select_font(font4x6);
      TV.println(20, 55, "You won the most computer!");
    }
    else {
      TV.select_font(font8x8);
      TV.println(40, 40, "You lose!");
      TV.select_font(font4x6);
      TV.println(15, 60, "too bad, good luck next time");
    }
  } else {
    int winner = game.score1 > game.score2 ? 1 : 2;
    TV.println(30, 30, "The Winner is player");
    TV.println(115, 30, winner);
  }
  soundGame(400, 100);
  delay(100);
  soundGame(300, 200);
  delay(100);
  soundGame(200, 200);
  delay(50);
  soundGame(400, 50);
  delay(9750); 
}


void change_difficulty() {
  unsigned long current_time = millis(); // time minus all time off (waiting for start)

  if (current_time - game.prev_time1 >= 7000 && game.ball.size > 1) {
    game.ball.size = game.ball.size / 2;
    TV.draw_circle(game.ball.position.x, game.ball.position.y, game.ball.size / 2, WHITE, WHITE);
    game.prev_time1 = current_time;
    game.ball_size += 1;
  }

  if (current_time - game.prev_time2 >= 10000) {
    if ((abs(game.ball.speed.x) < 4) && (abs(game.ball.speed.y) < 4)) {
      game.ball.speed.x += (game.ball.speed.x > 0 ? 1 : -1);
      game.ball.speed.y += (game.ball.speed.y > 0 ? 1 : -1);
      game.prev_time2 = current_time;
      game.speed += 1;
    }
  }
  if ((current_time - game.prev_time3 >= 12000) && (game.paddle_size > 0.5))
  {
    game.paddle_size = game.paddle_size - 3.0;
    if (game.paddle_size < 0.5) {
      game.paddle_size = 0.5; // Ensure we don't go below 0.5 (1 pixel total height)
    }
    game.paddle_difficulty += 1;
    game.prev_time3 = current_time;
  }
}

void set_time(){
  game.prev_time1 = millis();
  game.prev_time2 = game.prev_time1;
  game.prev_time3 = game.prev_time1;
}

void setup() {
  TV.begin(PAL, SCREEN_WIDTH, SCREEN_HEIGHT);
  startup();
  drawScene();
  //set_time();
}


void loop() {
  if (!game.game_over && game.game_on) {
    posPlayers(); // position players because game is running
    drawScene();
    delay(30); // Give enough time for display to reset
    moveBall();
    change_difficulty();
  } else if (!game.game_over && !game.game_on) {
    // Game is not running yet, but not game over
    checkMotion(); // check motion when game is not running
    drawScene();
    delay(50);
  }

  if (game.score1 == TOTAL_POINTS || game.score2 == TOTAL_POINTS) {
    game.game_over = true;
    gameover();
    delay(30);
    TV.clear_screen();
    startup();
    drawScene();
    set_time();
  }
}
