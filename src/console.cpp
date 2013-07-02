#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <cassert>

#include <sstream>
#include <string>
#include <vector>

using namespace std;

static void finish(int sig);


// KEY_ENTER = 232 rather than 13
#define _KEY_ENTER       13

enum {
    MAP_NONE = 0,
    MAP_WRAP_AROUND
};

inline int mapRow(int row, int col, int mode = MAP_WRAP_AROUND)
{
    if (mode == MAP_WRAP_AROUND) return row + col/COLS;
    else return row;
}

inline int mapCol(int row, int col, int mode = MAP_WRAP_AROUND)
{
    if (mode == MAP_WRAP_AROUND) return col % COLS;
    else return col;
}

int logical_move(int row, int col, int mode = MAP_WRAP_AROUND)
{
    return move(mapRow(row, col, mode), mapCol(row, col, mode));
}

int logical_mvchgat(int row, int col, int n, attr_t attr, short color, const void* opts, int mode = MAP_WRAP_AROUND)
{
    return mvchgat(mapRow(row, col, mode), mapCol(row, col, mode), n, attr, color, opts);
}

int logical_mvaddch(int row, int col, const chtype c, int mode = MAP_WRAP_AROUND)
{
    return mvaddch(mapRow(row, col, mode), mapCol(row, col, mode), c);
}

int logical_mvaddstr(int row, int col, const char* str, int mode = MAP_WRAP_AROUND)
{
    return mvaddstr(mapRow(row, col, mode), mapCol(row, col, mode), str);
}

class ConsoleSession
{
private:
    int mode;
    std::vector<std::string> lines;

public:
    ConsoleSession(int _mode = MAP_WRAP_AROUND) : mode(_mode) { }
};

bool handleMotion(int& row, int& col, int promptlen, string& line, int c)
{
    assert(col >= promptlen);
    size_t pos = col - promptlen;

    switch (c) {
    case KEY_LEFT:
        if (pos > 0) col--;
        return true;

    case KEY_RIGHT:
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
        logical_mvaddstr(row, col + 1, line.substr(pos + 1).c_str());
    }
    logical_mvaddch(row, col, c);
    col++;
    return true;
}

bool handleEdit(int& row, int& col, int promptlen, string& line, int c)
{
    assert(col >= promptlen);
    size_t pos = col - promptlen;

    switch (c) {
    case KEY_BACKSPACE:
        if (pos > 0) {
            col--;
            line.erase(pos - 1, 1);
            logical_mvaddstr(row, col, line.substr(pos - 1).c_str());
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
    logical_mvaddstr(row, col, prompt.c_str());
    attrset(COLOR_PAIR(7));
    int promptlen = prompt.size();
    col += promptlen;
    string line = "";
    while (true)
    {
        logical_move(row, col);
        chgat(1, A_STANDOUT, 0, NULL);
        int c = getch();
        chgat(1, A_NORMAL, 0, NULL);
        if (c == _KEY_ENTER) break;

        if (!handleMotion(row, col, promptlen, line, c) &&
            !handleEdit(row, col, promptlen, line, c) &&
            !handleVisible(row, col, promptlen, line, c))
        {
            stringstream ss;
            ss << c;
            logical_mvaddstr(1, 0, ss.str().c_str());
            addstr("     ");
        }
    }

    return line;
}

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

    while (true) {
        string line = getLine(0, 0, "> ");
        if (line == "exit") break;

        mvprintw(10, 0, line.c_str());
    }

    finish(0);               /* we're done */
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}
