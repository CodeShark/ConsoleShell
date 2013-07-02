#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <cassert>

#include <sstream>
#include <string>

using namespace std;

static void finish(int sig);

#define ENTER_KEY       13

#define DOWN_ARROW      258
#define UP_ARROW        259
#define LEFT_ARROW      260
#define RIGHT_ARROW     261

#define BACKSPACE       263

bool handleMotion(int& row, int& col, int promptlen, string& line, int c)
{
    assert(col >= promptlen);
    size_t pos = col - promptlen;

    switch (c) {
    case LEFT_ARROW:
        if (pos > 0) col--;
        return true;

    case RIGHT_ARROW:
        if (pos < line.size()) col++;
        return true;

    default:
        return false;
    }
}

bool handleVisible(int& row, int& col, int promptlen, string& line, int c, bool bReplace = false)
{
    if (c < ' ' || c > '~') return false;

    assert(col >= promptlen);
    size_t pos = col - promptlen;

    if (pos > line.size()) {
        line += c;
    }
    else if (bReplace) {
        line.replace(pos, 1, 1, c);
    }
    else {
        line.insert(pos, 1, c);
        mvprintw(row, col + 1, "%s", line.substr(pos + 1).c_str());
    }
    mvaddch(row, col, c);
    col++;
    return true;
}

bool handleEdit(int& row, int& col, int promptlen, string& line, int c)
{
    assert(col >= promptlen);
    size_t pos = col - promptlen;

    switch (c) {
    case BACKSPACE:
        if (pos > 0) {
            col--;
            line.erase(pos - 1, 1);
            mvprintw(row, col, "%s", line.substr(pos - 1).c_str());
            addch(' ');
        }
        return true;

    default:
        return false;
    }
}

string getLine(int row = 0, int col = 0, const string& prompt = "> ")
{
    attrset(COLOR_PAIR(4));
    mvprintw(row, col, "%s", prompt.c_str());
    attrset(COLOR_PAIR(7));
    int promptlen = prompt.size();
    col += promptlen;
    string line = "";
    while (true)
    {
        move(row, col);
        chgat(1, A_STANDOUT, 0, NULL);
        int c = getch();
        chgat(1, A_NORMAL, 0, NULL);
        if (c == ENTER_KEY) break;

        if (!handleMotion(row, col, promptlen, line, c) &&
            !handleEdit(row, col, promptlen, line, c) &&
            !handleVisible(row, col, promptlen, line, c))
        {
            stringstream ss;
            ss << c;
            mvprintw(1, 0, "%s   ", ss.str().c_str());
            move(row, col);
        }
    }

    return line;
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
/*
    attrset(COLOR_PAIR(7));
    int row = 0;
    int col = 0;
    string line;
    for (;;)
    {
        move(row, col);
        chgat(1, A_STANDOUT, 0, NULL);
        int c = getch();
        chgat(1, A_NORMAL, 0, NULL);

        if (!handleMotion(row, col, line, c) &&
            !handleEdit(row, col, line, c) &&
            !handleVisible(row, col, line, c))
        {
            stringstream ss;
            ss << c;
            mvprintw(1, 0, "%s   ", ss.str().c_str());
            move(row, col);
        }

    }*/

    string lastline = "";
    while (true) {
        string line = getLine(0, 0, "> ");
        if (line == "exit") break;

        mvprintw(1, 0, "%s", line.c_str());
    }

    finish(0);               /* we're done */
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}
