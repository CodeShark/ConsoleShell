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

std::string dummystr;

class ConsoleSession
{
private:
    unsigned int cursorRow;
    unsigned int cursorCol;

    std::string prompt;
    std::string& editLine;

    int mode;
    bool bReplace;
    std::vector<std::string> lines;
    std::vector<std::string> input;

    size_t currentInput;

protected:
    bool handleMotion(int c)
    {
        assert(cursorCol >= prompt.size());
        size_t pos = cursorCol - prompt.size();

        switch (c) {
        case KEY_LEFT:
            if (pos > 0) cursorCol--;
            return true;

        case KEY_RIGHT:
            if (pos < editLine.size()) cursorCol++;
            return true;

        default:
            return false;
        }
    }

    bool handleEdit(int c)
    {
        assert(cursorCol >= prompt.size());
        size_t pos = cursorCol - prompt.size();

        switch (c) {
        case KEY_BACKSPACE:
            if (pos > 0) {
                cursorCol--;
                editLine.erase(pos - 1, 1);
                logical_mvaddstr(cursorRow, cursorCol, editLine.substr(pos - 1).c_str(), mode);
                addch(' ');
            }
            return true;

        case KEY_UP:
            if (currentInput > 0) {
                editLine = input[--currentInput];
                addstr(editLine.c_str());
                logical_mvaddstr(cursorRow, prompt.size(), editLine.c_str(), mode);
            }
            return true;

        case KEY_DOWN:
            if (currentInput < input.size() - 1) {
                editLine = input[++currentInput];
                logical_mvaddstr(cursorRow, prompt.size(), editLine.c_str(), mode);
            }
            return true;

        default:
            return false;
        }
    }

    bool handleVisible(int c)
    {
        if (c < ' ' || c > '~') return false;

        assert(cursorCol >= prompt.size());
        size_t pos = cursorCol - prompt.size();

        if (pos > editLine.size()) {
            editLine += c;
        }
        else if (bReplace) {
            editLine.replace(pos, 1, 1, c);
        }
        else {
            editLine.insert(pos, 1, c);
            logical_mvaddstr(cursorRow, cursorCol + 1, editLine.substr(pos + 1).c_str());
        }
        logical_mvaddch(cursorRow, cursorCol, c);
        cursorCol++;
        return true;
    }
 
public:
    ConsoleSession(const std::string& _prompt = "> ", int _mode = MAP_WRAP_AROUND) : cursorRow(0), cursorCol(0), prompt(_prompt), editLine(dummystr), mode(_mode), bReplace(false), currentInput(0) { }

    string getLine()
    {   
        attrset(COLOR_PAIR(4));
        logical_mvaddstr(cursorRow, cursorCol, prompt.c_str());
        attrset(COLOR_PAIR(7));
        int promptlen = prompt.size();
        cursorCol += promptlen;
        input.push_back("");
        editLine = input.back();
        while (true)
        {   
            logical_move(cursorRow, cursorCol);
            chgat(1, A_STANDOUT, 0, NULL);
            int c = getch();
            chgat(1, A_NORMAL, 0, NULL);
            if (c == _KEY_ENTER) break;

            if (!handleMotion(c) &&
                !handleEdit(c) &&
                !handleVisible(c))
            {
                stringstream ss;
                ss << c;
                logical_mvaddstr(1, 0, ss.str().c_str());
                addstr("     ");
            }
        }
        cursorRow++;
        cursorCol = 0;

        lines.push_back(prompt + editLine);
        currentInput = input.size();

        return editLine;
    }

};

/*
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
*/
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

        mvprintw(20, 0, line.c_str());
    }

    finish(0);               /* we're done */
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}
