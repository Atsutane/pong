/* Copyright (c) 2009, Thorsten Toepper
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of Thorsten Toepper nor the
 *   names of contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Thorsten Toepper ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Thorsten Toepper BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <time.h>
#include <ncurses.h>


struct player_data {
    unsigned int x;
    unsigned int y;
    unsigned int score;
    bool ai;
};

struct ball_data {
    unsigned int x;
    unsigned int y;
    bool mv_left;
    bool mv_right;
    bool mv_up;
    bool mv_down;
};

struct game_data {
    unsigned int max_field_x; /* field size x */
    unsigned int max_field_y; /* field size y */
    struct player_data *p1; /* Player 1 */
    struct player_data *p2; /* Player 2 */
    struct ball_data *ball; /* Ball */
};


/* Called in the main loop to verify the size
 * of the current game field and if it changed
 * to update the size.
 */
bool check_field_size(struct game_data *gd) {
    unsigned int y, x; /* temporary data */
    if (gd == NULL) {
        return FALSE;
    }

    /* Get current size of the terminal */
    getmaxyx(stdscr, y, x); 
    
    x--;
    y--;

    /* If size changed, update the data and return FALSE so
     * it's easy to check if the data changed.
     */
    if ((gd->max_field_y != y) || (gd->max_field_x != x)) {
        gd->max_field_x = x;
        gd->max_field_y = y;
        return FALSE;
    }
    return TRUE;
}


/* Allocates memory for every item and
 * fills them with the initial data.
 */
struct game_data *init_game_data(void) {
    struct game_data *gd;

    /* Allocate memory for every item */
    gd = malloc(sizeof(struct game_data));
    if (gd == NULL) {
        return NULL;
    }
  
    gd->p1 = malloc(sizeof(struct player_data));
    if (gd->p1 == NULL) {
        free(gd);
        return NULL;
    }

    gd->p2 = malloc(sizeof(struct player_data));
    if (gd->p2 == NULL) {
        free(gd->p1);
        free(gd);
        return NULL;
    }

    gd->ball = malloc(sizeof(struct ball_data));
    if (gd->ball == NULL) {
        free(gd->p2);
        free(gd->p1);
        free(gd);
        return NULL;
    }

    /* Fill items with the default data */
    check_field_size(gd);

    gd->p1->x = 0;
    gd->p1->y = gd->max_field_y / 2;
    gd->p1->score = 0;
    gd->p1->ai = FALSE;

    gd->p2->x = gd->max_field_x;
    gd->p2->y = gd->max_field_y / 2;
    gd->p2->score = 0;
    gd->p2->ai = TRUE;

    gd->ball->x = gd->max_field_x / 2;
    gd->ball->y = (gd->max_field_y-1) / 2; /* w/o statusbar */
    gd->ball->mv_left = FALSE;
    gd->ball->mv_right = FALSE;
    gd->ball->mv_up = FALSE;
    gd->ball->mv_down = FALSE;

    return gd;
}

/* Controls the movement of the ball */
void ball_movement(struct game_data *gd) {
    if (gd == NULL) {
        return;
    }
    
    /* Clear current position of the ball */
    mvaddch(gd->ball->y, gd->ball->x, ' ');
    
    if (gd->ball->x == 1) {
        /* Does it hit p1s pad? */
        if ((gd->ball->y > gd->p1->y-2) &&
                (gd->ball->y < gd->p1->y+2)) {
            gd->ball->mv_left = FALSE;
            gd->ball->mv_right = TRUE;
            gd->ball->mv_up = FALSE;
            gd->ball->mv_down = FALSE;

            if ((gd->ball->y > gd->p1->y-2) &&
                    (gd->ball->y < gd->p1->y)) {
                gd->ball->mv_up = TRUE;
            }
            if ((gd->ball->y > gd->p1->y) &&
                    (gd->ball->y < gd->p1->y+2)) {
                gd->ball->mv_down = TRUE;
            }
        }
    }

    if (gd->ball->x == gd->max_field_x-1) {
        /* Does it hit p2s pad? */
        if ((gd->ball->y > gd->p2->y-2) &&
                (gd->ball->y < gd->p2->y+2)) {
            gd->ball->mv_left = TRUE;
            gd->ball->mv_right = FALSE;
            gd->ball->mv_up = FALSE;
            gd->ball->mv_down = FALSE;

            if ((gd->ball->y > gd->p2->y-2) &&
                    (gd->ball->y < gd->p2->y)) {
                gd->ball->mv_up = TRUE;
            }
            if ((gd->ball->y > gd->p2->y) &&
                    (gd->ball->y < gd->p2->y+2)) {
                gd->ball->mv_down = TRUE;
            }
        }
    }

    /* Check if it hits the top of the terminal */
    if (gd->ball->y == 0) {
        gd->ball->mv_up = FALSE;
        gd->ball->mv_down = TRUE;
    }
    /* Check if it hits the statusbar */
    else if (gd->ball->y == gd->max_field_y-1) {
        gd->ball->mv_down = FALSE;
        gd->ball->mv_up = TRUE;
    }


    /* Check if p2 scores and if, reset location to the front of
     * p1s pad launching the ball will be handled elsewhere.
     */
    if (gd->ball->x == 0) {
        gd->p2->score +=1 ;

        gd->ball->x = 1;
        gd->ball->y = (gd->max_field_y-1) / 2;
        gd->ball->mv_left = FALSE;
        gd->ball->mv_right = FALSE;
        gd->ball->mv_up = FALSE;
        gd->ball->mv_down = FALSE;
    }
    /* Check if p1 scores and if, reset the location to the front of
     * p2s pad, launching the ball will be handled elsewhere.
     */
    if (gd->ball->x == gd->max_field_x) {
        gd->p1->score +=1 ;

        gd->ball->x = gd->max_field_x-1;
        gd->ball->y = (gd->max_field_y-1) / 2;
        gd->ball->mv_left = FALSE;
        gd->ball->mv_right = FALSE;
        gd->ball->mv_up = FALSE;
        gd->ball->mv_down = FALSE;
    }
 
    /* Movement of the ball */
    if (gd->ball->mv_left == TRUE) {
        gd->ball->x--;
    }
    else if (gd->ball->mv_right == TRUE) {
        gd->ball->x++;
    }
    if (gd->ball->mv_up == TRUE) {
        gd->ball->y--;
    }
    else if (gd->ball->mv_down == TRUE) {
        gd->ball->y++;
    }
    mvaddch(gd->ball->y, gd->ball->x, '0');
}

void quit(void) {
    endwin();
}


int main(void) {
    struct game_data *game; /* contains all our data */
    
    game = init_game_data();

    return EXIT_SUCCESS;
}

