#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <cassert>

#include <sstream>
#include <string>
#include <vector>

#include "console_session.h"

using namespace std;

static void finish(int sig);

// SIGWINCH is called when the window is resized.
void handle_winch(int sig)
{
    signal(SIGWINCH, SIG_IGN);

    // Reinitialize the window to update data structures.
    endwin();
    initscr();
    refresh();
    //clear();

    stringstream ss;
    ss << COLS << " x " << LINES;
    const string& dims = ss.str();

    // Approximate the center
    int x = COLS / 2 - dims.size() / 2;
    int y = LINES / 2 - 1;

    mvaddstr(y, x, dims.c_str());
    refresh();

    signal(SIGWINCH, handle_winch);
}

int main(int argc, char *argv[])
{
    /* initialize your non-curses data structures here */

    signal(SIGINT, finish);
    signal(SIGWINCH, handle_winch);

    initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    noecho();    

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

    ConsoleSession cs;
    while (true) {
        string line = cs.getLine();
        if (line == "exit") break;

        cs.putLine(line);
    }

    finish(0);               /* we're done */
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}
