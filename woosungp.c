#include <osbind.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* --- 리소스 헤더 --- */
#include "background.h"
#include "title_data.h"
#include "pati_panel.h"
#include "esad_panel.h"
#include "visa_panel.h"
#include "gpt_panel.h"
#include "myvisa_panel.h"
#include "gameover_panel.h"

#include "player_left.h"
#include "player_right.h"
#include "player_left_shot.h"
#include "player_right_shot.h"
#include "bullet.h"
#include "paper.h"      
#include "paper2.h"     
#include "paper_dead.h" 
#include "player_right_walk1.h"
#include "player_right_walk2.h"
#include "player_left_walk1.h"
#include "player_left_walk2.h"

/* --- 설정 및 매크로 --- */
#define SCREEN_W 320
#define SCREEN_H 200
#define BPL      160
#define G_PW     32
#define G_PH     48
#define E_W      32
#define E_H      48
#define B_WIDTH  16
#define B_HEIGHT 16
#define MAX_BULLETS 5
#define MAX_ENEMIES 10

#define PHOTO_W  192
#define PHOTO_H  96
#define PHOTO_X  64
#define PHOTO_Y  24

#define ST_IDLE    0
#define ST_WALK    1
#define ST_ATTACK  2
#define ENEMY_ALIVE 1
#define ENEMY_DEAD  2

typedef struct { int x, y, active, direction; } Bullet;
typedef struct { 
    int x, y, active, speed; 
    int state;       
    int anim_timer;  
    int anim_frame;  
} Enemy;

Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];

/* --- [유틸리티 및 사운드 함수] --- */
static void write_psg(int reg, int val) { Giaccess(val, reg | 0x80); }
static void stop_all_sound() { int i; for(i=8; i<=10; i++) write_psg(i, 0); }

static void play_shoot_sound() {
    write_psg(6, 2); write_psg(7, 0xF7); write_psg(8, 10);
    write_psg(11, 0); write_psg(12, 1); write_psg(13, 0);
}

static void play_hit_sound() {
    write_psg(6, 15); write_psg(7, 0xF7); write_psg(8, 8);
    write_psg(11, 0); write_psg(12, 2); write_psg(13, 0);
}

static void vt52_clear(void) { Cconws("\033E"); }
static void vt52_goto(int row, int col) {
    char s[6]; s[0]=27; s[1]='Y'; s[2]=(char)(32+row); s[3]=(char)(32+col); s[4]=0; Cconws(s);
}

/* 🚩 관성 입력 방지용 대기 함수 */
static void wait_key_with_delay(int vbl_delay) {
    int i;
    for (i = 0; i < vbl_delay; i++) Vsync(); /* 지정된 시간 동안 강제 대기 */
    while (Cconis()) Cnecin();              /* 대기 중 눌린 키 버퍼 비우기 */
    while (!Cconis());                      /* 새로운 입력 대기 */
    Cnecin();
}

static void wait_key(void) { while(!Cconis()); Cnecin(); }
static void set_pal(const unsigned short *pal) { int i; for (i = 0; i < 16; i++) Setcolor(i, pal[i]); }

static void draw_topimage(unsigned char *screen, const unsigned char *data, int w, int h, int x, int y) {
    int row, bpl_src = (w / 16) * 8;
    for(row = 0; row < h; row++) {
        unsigned char *dst = screen + (y + row) * BPL + (x / 16) * 8;
        memcpy(dst, data + (row * bpl_src), bpl_src);
    }
}

static void blit_sprite_masked(unsigned char *screen, const unsigned char *data, const unsigned char *mask, int w, int h, int x, int y) {
    int row, i, src_bpl = (w / 16) * 8;
    if (x <= -w || x >= SCREEN_W || y <= -h || y >= SCREEN_H) return;
    for (row = 0; row < h; row++) {
        int target_y = y + row;
        if (target_y >= 0 && target_y < SCREEN_H) {
            unsigned char *dst = screen + (target_y * BPL) + (x / 16) * 8;
            const unsigned char *src_d = data + (row * src_bpl);
            const unsigned char *src_m = mask + (row * src_bpl);
            for (i = 0; i < src_bpl; i++) { dst[i] &= src_m[i]; dst[i] |= src_d[i]; }
        }
    }
}

int check_collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

static void print_centered(int start_row, const char *text) {
    char temp[64]; int i = 0, j = 0, cur_row = start_row;
    while (text[i] != '\0') {
        if (text[i] == '\n') {
            temp[j] = '\0'; int col = (40 - j) / 2;
            vt52_goto(cur_row++, (col < 0 ? 0 : col)); Cconws(temp); j = 0;
        } else { temp[j++] = text[i]; } i++;
    }
    temp[j] = '\0'; int col = (40 - j) / 2;
    vt52_goto(cur_row, (col < 0 ? 0 : col)); Cconws(temp);
}

/* --- [씬 관리] --- */
void show_title_screen(unsigned char *screen) {
    int counter = 0, blink = 0; memcpy(screen, title_screen, 32000); 
    set_pal(title_palette); Setcolor(15, 0x777);
    while (!Cconis()) {
        if (++counter >= 30) { 
            counter = 0; blink = !blink; 
            vt52_goto(20, 9); Cconws(blink ? "PRESS ANY KEY TO START" : "                      "); 
        } Vsync();
    } Cnecin();
}

void intro_scenes(unsigned char *screen) {
    const char *blocks[] = { 
        "Woosung Park is a student from\nPaju Typography Institute.\n\n"
        "Woosung Park est un etudiant de\nPaju Typography Institute.",
        "He decided to go to ESAD_orleans\nfor his exchange program.\n\n"
        "Il a decide d'aller a l'ESAD d'Orleans\npour son programme d'echange.",
        "To make it happen, he must deal with\ncountless paperwork and procedures.\n\n"
        "Pour y parvenir, il doit affronter\nune paperasse sans fin.",
        "With AI, defeat all the paperwork!!!\n\n"
        "Avec l'IA, gagnez contre la paperasse !!!" 
    };
    int i; for (i = 0; i < 4; i++) {
        vt52_clear();
        if (i == 0) { set_pal(pati_palette); draw_topimage(screen, pati_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y); }
        else if (i == 1) { set_pal(esad_palette); draw_topimage(screen, esad_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y); }
        else if (i == 2) { set_pal(visa_palette); draw_topimage(screen, visa_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y); }
        else { set_pal(gpt_palette); draw_topimage(screen, gpt_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y); }
        Setcolor(15, 0x777); print_centered(16, blocks[i]); wait_key();
    }
}

void show_success(unsigned char *screen) {
    vt52_clear(); set_pal(myvisa_palette); Setcolor(15, 0x777);
    draw_topimage(screen, myvisa_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y);
    print_centered(17, "CONGRATULATIONS!\nYou finally got your VISA.\n\n"
                       "FELICITATIONS !\nVous avez enfin obtenu votre VISA.");
    wait_key();
}

void show_game_over(unsigned char *screen) {
    vt52_clear(); set_pal(gameover_palette); Setcolor(15, 0x777);
    draw_topimage(screen, gameover_data, PHOTO_W, PHOTO_H, PHOTO_X, PHOTO_Y);
    print_centered(17, "FAILED TO GET VISA...\nYou have to stay in Korea.\n\n"
                       "ECHEC DE L'OBTENTION DU VISA...\nVous devez rester en Coree.");
    wait_key();
}

/* --- [메인 게임 루프] --- */
void main_game(unsigned char *screen) {
    int px = 144, py = 120, facing = 1, state = ST_IDLE;
    int anim_timer = 0, anim_frame = 0, attack_timer = 0;
    int spawn_timer = 0, sky_timer = 0, lives = 3, invul_timer = 0, papers_left = 30; 
    unsigned char *back_buffer = (unsigned char *)Malloc(32000);
    int i, j; if (!back_buffer) return;

    for (i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
    for (i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    set_pal(background_palette); stop_all_sound();

    while (lives > 0 && papers_left > 0) {
        memcpy(back_buffer, background_data, 32000);
        int moved = 0;
        if (Cconis()) {
            long k = Cnecin(); int scan = (int)(k >> 16);
            if (scan == 0x4B) { facing = 0; px -= 9; moved = 1; } 
            else if (scan == 0x4D) { facing = 1; px += 9; moved = 1; } 
            else if (scan == 0x39 && state != ST_ATTACK) { 
                state = ST_ATTACK; attack_timer = 0; play_shoot_sound();
                for (i = 0; i < MAX_BULLETS; i++) if (!bullets[i].active) {
                    bullets[i].active = 1; bullets[i].direction = facing;
                    bullets[i].x = facing ? (px + 28) : (px - 8); bullets[i].y = py + 18; break;
                }
            } else if ((k & 0xFF) == 27) break;
        }

        if (moved) { state = ST_WALK; if (px < 0) px = 0; if (px > 320-G_PW) px = 320-G_PW; }
        else if (state != ST_ATTACK) state = ST_IDLE;

        if (++spawn_timer > 15) {
            spawn_timer = 0;
            for (i=0; i<MAX_ENEMIES; i++) if (!enemies[i].active) {
                int side = rand() % 2;
                enemies[i].active=1; enemies[i].state=ENEMY_ALIVE; 
                enemies[i].x= side ? -E_W : SCREEN_W; 
                enemies[i].y=py; enemies[i].speed= side ? 6 : -6; 
                enemies[i].anim_timer=0; enemies[i].anim_frame=0; break;
            }
        }
        if (++sky_timer > 90) { 
            sky_timer = 0;
            for (i=0; i<MAX_ENEMIES; i++) if (!enemies[i].active) {
                enemies[i].active=1; enemies[i].state=ENEMY_ALIVE;
                int pos = (rand() % 3) + 2; 
                enemies[i].x=(pos-1)*64+(64-E_W)/2; enemies[i].y=-E_H; enemies[i].speed=999; break;
            }
        }
        if (invul_timer > 0) invul_timer--;
        for (j=0; j<MAX_BULLETS; j++) {
            if (bullets[j].active) {
                bullets[j].x += (bullets[j].direction ? 16 : -16);
                if (bullets[j].x < -16 || bullets[j].x > 320) bullets[j].active = 0;
                for (i=0; i<MAX_ENEMIES; i++) {
                    if (enemies[i].active && enemies[i].state == ENEMY_ALIVE && bullets[j].active) {
                        if (check_collision(bullets[j].x, bullets[j].y, B_WIDTH, B_HEIGHT, enemies[i].x, enemies[i].y, E_W, E_H)) {
                            bullets[j].active = 0; enemies[i].state = ENEMY_DEAD; enemies[i].anim_timer = 0; papers_left--;
                            play_hit_sound();
                        }
                    }
                }
            }
        }
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                if (enemies[i].state == ENEMY_ALIVE) {
                    if (enemies[i].speed == 999) { enemies[i].y += 4; if (enemies[i].y > py + E_H) enemies[i].active = 0; }
                    else { enemies[i].x += enemies[i].speed; if (enemies[i].x < -E_W-10 || enemies[i].x > SCREEN_W+10) enemies[i].active = 0; }
                    if (++enemies[i].anim_timer > 8) { enemies[i].anim_frame = !enemies[i].anim_frame; enemies[i].anim_timer = 0; }
                    if (invul_timer == 0 && check_collision(px, py, G_PW, G_PH, enemies[i].x, enemies[i].y, E_W, E_H)) {
                        lives--; enemies[i].active = 0; invul_timer = 30; play_hit_sound();
                    }
                } else if (enemies[i].state == ENEMY_DEAD) { if (++enemies[i].anim_timer > 10) enemies[i].active = 0; }
            }
        }
        if (state == ST_ATTACK && ++attack_timer > 6) state = ST_IDLE;
        if (state == ST_WALK && ++anim_timer > 5) { anim_frame = !anim_frame; anim_timer = 0; }

        for (i=0; i<MAX_ENEMIES; i++) if (enemies[i].active) {
            const unsigned char *d, *m;
            if (enemies[i].state == ENEMY_DEAD) { d = paper_dead_data; m = paper_dead_mask; }
            else { d = (enemies[i].anim_frame == 0) ? paper_data : paper2_data; m = (enemies[i].anim_frame == 0) ? paper_mask : paper2_mask; }
            blit_sprite_masked(back_buffer, d, m, E_W, E_H, enemies[i].x, enemies[i].y);
        }
        for (i=0; i<MAX_BULLETS; i++) if (bullets[i].active) blit_sprite_masked(back_buffer, bullet_data, bullet_mask, B_WIDTH, B_HEIGHT, bullets[i].x, bullets[i].y);
        
        if (invul_timer == 0 || (invul_timer / 3) % 2 == 0) {
            const unsigned char *d, *m;
            if (state == ST_ATTACK) { d = facing ? player_right_shot_data : player_left_shot_data; m = facing ? player_right_shot_mask : player_left_shot_mask; }
            else if (state == ST_WALK) {
                if (facing == 1) { d = anim_frame ? player_right_walk2_data : player_right_walk1_data; m = anim_frame ? player_right_walk2_mask : player_right_walk1_mask; }
                else { d = anim_frame ? player_left_walk2_data : player_left_walk1_data; m = anim_frame ? player_left_walk2_mask : player_left_walk1_mask; }
            } else { d = facing ? player_right_data : player_left_data; m = facing ? player_right_mask : player_left_mask; }
            blit_sprite_masked(back_buffer, d, m, G_PW, G_PH, px, py);
        }

        Vsync(); memcpy(screen, back_buffer, 32000); Setcolor(15, 0x777); 
        char ui_str[40]; sprintf(ui_str, "PAPERS LEFT: %02d   ATTEMPTS: %d", (papers_left < 0 ? 0 : papers_left), lives);
        vt52_goto(1, 2); Cconws(ui_str); Vsync();
    }
    stop_all_sound(); Mfree(back_buffer); Setcolor(15, 0x777);

    /* 🚩 결과 출력 및 입력 지연 적용 */
    if (papers_left <= 0) {
        vt52_goto(10, 4); Cconws("All the paperwork is complete!");
        vt52_goto(12, 3); Cconws("Tous les documents sont completes!");     
        wait_key_with_delay(70); /* 약 1.4초 대기 후 입력 받기 */
        show_success(screen);
    } else {
        vt52_goto(10, 1); Cconws("You couldn't finish the paperwork...");
        vt52_goto(12, 1); Cconws("La paperasse n'est pas encore terminee.");
        wait_key_with_delay(70); /* 약 1.4초 대기 후 입력 받기 */
        show_game_over(screen);
    }
}

int main(void) {
    unsigned char *screen = (unsigned char *)Physbase();
    int oldrez = Getrez(); unsigned short old_pal[16];
    int i; for (i = 0; i < 16; i++) old_pal[i] = Setcolor(i, -1);
    Setscreen((void*)-1, (void*)-1, 0); 
    
    while(1) { 
        show_title_screen(screen); intro_scenes(screen); main_game(screen); 
        int selection = 0, menu_done = 0;
        while(!menu_done) {
            Vsync(); Setcolor(15, 0x777); vt52_goto(23, 6);
            if (selection == 0) Cconws(" > RESTART <           EXIT    ");
            else Cconws("   RESTART           > EXIT <  ");
            if (Cconis()) {
                long k = Cnecin(); int scan = (int)(k >> 16), ascii = (int)(k & 0xFF);
                if (scan == 0x4B) selection = 0; else if (scan == 0x4D) selection = 1;
                else if (scan == 0x39 || ascii == 13) {
                    if (selection == 0) menu_done = 1;
                    else { 
                        stop_all_sound(); Setscreen((void*)-1, (void*)-1, oldrez);
                        for (i = 0; i < 16; i++) Setcolor(i, old_pal[i]);
                        vt52_clear(); return 0;
                    }
                }
            }
        }
    }
}