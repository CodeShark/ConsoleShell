#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <cassert>

#include <sstream>
#include <string>
#include <vector>

#include "dirty_vector.h"

using namespace std;

static void finish(int sig);


// KEY_ENTER = 232 rather than 13
#define _KEY_ENTER      13

#define CTRL_F          6
#define CTRL_B          2

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
    // logical coordinates. must be transformed to screen coordinates.
    unsigned int cursorRow;
    unsigned int cursorCol;

    unsigned int scrollRows;

    std::string prompt;
    std::string newLine;
    std::string* pEdit;

    int mode;
    bool bReplace;
    std::vector<std::string> prompts;
    std::vector<std::string> lines;
    dirty_vector<std::string> input;

    size_t currentInput;

protected:
    void update()
    {
//        endwin();
//        initscr();
        refresh();
        clear();

        unsigned int i = 0;
        unsigned int j = 0;
        for (; i < lines.size(); i++) {
            if (i >= scrollRows + LINES) return;

            if (prompts[i] != "") {
                attrset(COLOR_PAIR(4));
                logical_mvaddstr(i, 0, prompts[i].c_str(), false);
                attrset(COLOR_PAIR(7));
                logical_mvaddstr(i, prompts[i].size(), input[j++].c_str(), false);
            }
            else {
                logical_mvaddstr(i, 0, lines[i].c_str(), false);
            }
        }

        attrset(COLOR_PAIR(4));
        logical_mvaddstr(i, 0, prompt.c_str(), false);
        attrset(COLOR_PAIR(7));
        logical_mvaddstr(i, prompt.size(), pEdit->c_str(), false);
        chgat(1, A_STANDOUT, 0, NULL);
    }

    void scrollTo(unsigned int row)
    {
        scrollRows = row;
        update();
    }

    void autoScroll(unsigned int row)
    {
        if (row < scrollRows) {
            scrollTo(row);
        }
        else if (row >= scrollRows + LINES) {
            scrollTo(row - LINES + 1);
        }
    }

    int logical_move(int row, int col, bool bAutoScroll = true)
    {
        int newRow = mapRow(row, col, mode);
        if (bAutoScroll) autoScroll(newRow);
        return move(newRow - scrollRows, mapCol(row, col, mode));
    }

    int logical_mvchgat(int row, int col, int n, attr_t attr, short color, const void* opts, bool bAutoScroll = true)
    {
        int newRow = mapRow(row, col, mode);
        if (bAutoScroll) autoScroll(newRow);
        return mvchgat(newRow - scrollRows, mapCol(row, col, mode), n, attr, color, opts);
    }

    int logical_mvaddch(int row, int col, const chtype c, bool bAutoScroll = true)
    {
        int newRow = mapRow(row, col, mode);
        if (bAutoScroll) autoScroll(newRow);
        return mvaddch(newRow - scrollRows, mapCol(row, col, mode), c);
    }

    int logical_mvaddstr(int row, int col, const char* str, bool bAutoScroll = true)
    {
        int newRow = mapRow(row, col, mode);
        if (bAutoScroll) autoScroll(newRow);
        return mvaddstr(newRow - scrollRows, mapCol(row, col, mode), str);
    }

    void replaceEdit(std::string& newEdit)
    {
        std::string blanks(pEdit->size(), ' ');
        logical_mvaddstr(cursorRow, prompt.size(), blanks.c_str());
        pEdit = &newEdit;
        logical_mvaddstr(cursorRow, prompt.size(), pEdit->c_str());
        if (cursorCol > prompt.size() + pEdit->size()) {
            cursorCol = prompt.size() + pEdit->size();
        }
    }

    bool handleMotion(int c)
    {
        assert(cursorCol >= prompt.size());
        size_t pos = cursorCol - prompt.size();

        switch (c) {
        case KEY_LEFT:
            if (pos > 0) cursorCol--;
            return true;

        case KEY_RIGHT:
            if (pos < pEdit->size()) cursorCol++;
            return true;

        case CTRL_F:
            scrollTo(scrollRows + 1);
            return true;

        case CTRL_B:
            if (scrollRows > 0) {
                scrollTo(scrollRows - 1);
            }
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
                pEdit->erase(pos - 1, 1);
                logical_mvaddstr(cursorRow, cursorCol, pEdit->substr(pos - 1).c_str());
                addch(' ');
                logical_move(cursorRow, cursorCol);
            }
            return true;

        case KEY_UP:
            if (currentInput > 0) {
                currentInput--;
                replaceEdit(input.getDirty(currentInput));
            }
            return true;

        case KEY_DOWN:
            if (currentInput < input.size()) {
                currentInput++;
                if (currentInput == input.size()) {
                    replaceEdit(newLine);
                }
                else {
                    replaceEdit(input.getDirty(currentInput));
                }
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

        if (pos > pEdit->size()) {
            *pEdit += c;
        }
        else if (bReplace) {
            pEdit->replace(pos, 1, 1, c);
        }
        else {
            pEdit->insert(pos, 1, c);
            logical_mvaddstr(cursorRow, cursorCol + 1, pEdit->substr(pos + 1).c_str());
        }
        logical_mvaddch(cursorRow, cursorCol, c);
        cursorCol++;

        return true;
    }
 
public:
    ConsoleSession(const std::string& _prompt = "> ", int _mode = MAP_WRAP_AROUND) :
        cursorRow(0), cursorCol(0), scrollRows(0), prompt(_prompt), mode(_mode), bReplace(false), currentInput(0) { attrset(A_NORMAL); }

    bool isCursorInScreen() const
    {
        return ((int)(cursorRow - scrollRows) < LINES);
    }

    string getLine()
    {
        logical_move(cursorRow, cursorCol);
        attrset(COLOR_PAIR(4));
        logical_mvaddstr(cursorRow, 0, prompt.c_str());
        attrset(COLOR_PAIR(7));

        // clear rest of line if necessary
        string blank(LINES - prompt.size(), ' ');
        addstr(blank.c_str());

        newLine = "";
        pEdit = &newLine;
        currentInput = input.size();
        cursorCol = prompt.size();

        logical_move(cursorRow, cursorCol);

        while (true)
        {
            if (isCursorInScreen()) {
                // only highlight cursor if it's on the screen.
                chgat(1, A_STANDOUT, 0, NULL);
            }
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

        std::string newInput(*pEdit);
        input.clean();
        input.push_back(newInput);
        lines.push_back(prompt + newInput);
        prompts.push_back(prompt);
        return newInput;
    }

    void putLine(const std::string& line)
    {
        attrset(COLOR_PAIR(7));
        logical_mvaddstr(cursorRow, 0, line.c_str());
        cursorRow++;
        cursorCol = 0;

        lines.push_back(line);
        prompts.push_back("");
    }
};

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
