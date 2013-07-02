#include <stdlib.h>
#include <curses.h>
#include <signal.h>

#include <sstream>
#include <string>

using namespace std;

static void finish(int sig);

#define DOWN_ARROW      258
#define UP_ARROW        259
#define LEFT_ARROW      260
#define RIGHT_ARROW     261

#define BACKSPACE       263

bool handleMotion(int& row, int& col, string& line, int c)
{
    switch (c) {
    case LEFT_ARROW:
        if (col > 0) {
            chgat(1, A_NORMAL, 0, NULL);
            move(row, --col);
        }
        return true;

    case RIGHT_ARROW:
        if (col < (int)line.size()) {
            chgat(1, A_NORMAL, 0, NULL);
            move(row, ++col);
        }
        return true;

    default:
        return false;
    }
}

bool handleVisible(int& row, int& col, string& line, int c, bool bReplace = false)
{
    if (c >= ' ' && c <= '~') {
        if (col > (int)line.size()) {
            line += c;
        }
        else if (bReplace) {
            line.replace(col, 1, 1, c);
        }
        else {
            line.insert(col, 1, c);
            mvprintw(row, col + 1, "%s", line.substr(col + 1).c_str());
        }
        mvaddch(row, col, c);
        col++;
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    /* initialize your non-curses data structures here */

    (void) signal(SIGINT, finish);      /* arrange interrupts to terminate */

    (void) initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    (void) nonl();         /* tell curses not to do NL->CR/NL on output */
    (void) cbreak();       /* take input chars one at a time, no wait for \n */
    (void) noecho();    

    if (has_colors())
    {
        start_color();

        /*
         * Simple color assignment, often all we need.  Color pair 0 cannot
     * be redefined.  This example uses the same value for the color
     * pair as for the foreground color, though of course that is not
     * necessary:
         */
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }

    attrset(COLOR_PAIR(7));
    int row = 0;
    int col = 0;
    string line;
    for (;;)
    {
        chgat(1, A_STANDOUT, 0, NULL);
        int c = getch();     /* refresh, accept single keystroke of input */
        if (!handleMotion(row, col, line, c) && !handleVisible(row, col, line, c)) {
            stringstream ss;
            ss << c;
            mvprintw(1, 0, "%s   ", ss.str().c_str());
            move(row, col);
        }

        /* process the command keystroke */
    }

    finish(0);               /* we're done */
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}
