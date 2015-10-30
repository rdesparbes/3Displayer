#include <stdio.h>
#include <stdlib.h>

#include "ncurses.h"
#include "coord.h"
#include "color.h"


void initDisplay_(int screenWidth, int screenHeight, const Color *background)
{
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    if (has_colors() == TRUE) {	
	start_color();
    }
    curs_set(0);
    init_pair(0, COLOR_BLACK, COLOR_BLACK);
    init_pair(1, COLOR_BLUE,COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_GREEN);
    init_pair(3, COLOR_CYAN, COLOR_CYAN);
    init_pair(4, COLOR_RED, COLOR_RED); 
    init_pair(5, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(6, COLOR_YELLOW,COLOR_YELLOW);
    init_pair(7, COLOR_WHITE, COLOR_WHITE);
}

void resetDisplay_() 
{
    clear();
}

static int cutChar(unsigned char c)
{
    return c < 128 ? 0 : 1;
}

static int getColor(const Color *color)
{
    int c = 0;
    c = c | cutChar(color->b) << 0;
    c = c | cutChar(color->g) << 1;
    c = c | cutChar(color->r) << 2;
    return c;
}

void pixelDisplay_(const Coord *A, const Color *color)
{
    int maxW = 0;
    int maxH = 0;
    getmaxyx(stdscr, maxH, maxW);
    if (A->w >= 0 && A->w < maxW && A->h >= 0 && A->h < maxH) {
	int c = getColor(color);
	attron(COLOR_PAIR(c));
	mvprintw(A->h, 2 * A->w, "  ");
	attroff(COLOR_PAIR(c));
    }
}

void blitDisplay_()
{
    refresh();
}

void freeDisplay_()
{
    endwin();
}

void (*initDisplay)(int, int, const Color *) = &initDisplay_;
void (*resetDisplay)() = &resetDisplay_;
void (*pixelDisplay)(const Coord *, const Color *) = &pixelDisplay_;
void (*blitDisplay)() = &blitDisplay_;
void (*freeDisplay)() = &freeDisplay_;