///////////////////////////////////////////////////////////////////////////////
//
// console_session.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "console_session.h"

#include <stdlib.h>
#include <sstream>

// KEY_ENTER = 232 rather than 13
#define _KEY_ENTER      13

#define CTRL_F          6
#define CTRL_B          2

inline static int mapRow(int row, int col, int mode = MAP_WRAP_AROUND)
{
    if (mode == MAP_WRAP_AROUND) return row + col/COLS;
    else return row;
}

inline static int mapCol(int row, int col, int mode = MAP_WRAP_AROUND)
{
    if (mode == MAP_WRAP_AROUND) return col % COLS;
    else return col;
}

//
// Public Methods
//
ConsoleSession::ConsoleSession(const std::string& _prompt, int _mode) :
    cursorRow(0), cursorCol(0), scrollRows(0), prompt(_prompt), mode(_mode), bReplace(false), currentInput(0)
{
}

ConsoleSession::~ConsoleSession()
{
}

std::string ConsoleSession::getLine()
{
    logical_move(cursorRow, cursorCol);
    attrset(COLOR_PAIR(4));
    logical_mvaddstr(cursorRow, 0, prompt.c_str());
    attrset(COLOR_PAIR(7));

    // clear rest of line if necessary
    std::string blank(LINES - prompt.size(), ' ');
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
            std::stringstream ss;
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

void ConsoleSession::putLine(const std::string& line)
{
    attrset(COLOR_PAIR(7));
    logical_mvaddstr(cursorRow, 0, line.c_str());
    cursorRow++;
    cursorCol = 0;

    lines.push_back(line);
    prompts.push_back("");
}

bool ConsoleSession::isCursorInScreen() const
{
    return ((int)(cursorRow - scrollRows) < LINES);
}

//
// Protected Methods
//
void ConsoleSession::update()
{
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
    //chgat(1, A_STANDOUT, 0, NULL);
}

void ConsoleSession::autoScroll(unsigned int row)
{
    if (row < scrollRows) {
        scrollTo(row);
    }
    else if (row >= scrollRows + LINES) {
        scrollTo(row - LINES + 1);
    }
}

int ConsoleSession::logical_move(int row, int col, bool bAutoScroll)
{
    int newRow = mapRow(row, col, mode);
    if (bAutoScroll) autoScroll(newRow);
    return move(newRow - scrollRows, mapCol(row, col, mode));
}

int ConsoleSession::logical_mvchgat(int row, int col, int n, attr_t attr, short color, const void* opts, bool bAutoScroll)
{
    int newRow = mapRow(row, col, mode);
    if (bAutoScroll) autoScroll(newRow);
    return mvchgat(newRow - scrollRows, mapCol(row, col, mode), n, attr, color, opts);
}

int ConsoleSession::logical_mvaddch(int row, int col, const chtype c, bool bAutoScroll)
{
    int newRow = mapRow(row, col, mode);
    if (bAutoScroll) autoScroll(newRow);
    return mvaddch(newRow - scrollRows, mapCol(row, col, mode), c);
}

int ConsoleSession::logical_mvaddstr(int row, int col, const char* str, bool bAutoScroll)
{
    int newRow = mapRow(row, col, mode);
    if (bAutoScroll) autoScroll(newRow);
    return mvaddstr(newRow - scrollRows, mapCol(row, col, mode), str);
}

void ConsoleSession::replaceEdit(std::string& newEdit)
{
    std::string blanks(pEdit->size(), ' ');
    logical_mvaddstr(cursorRow, prompt.size(), blanks.c_str());
    pEdit = &newEdit;
    logical_mvaddstr(cursorRow, prompt.size(), pEdit->c_str());
    cursorCol = prompt.size() + pEdit->size();
    updateCursor();
}

bool ConsoleSession::handleMotion(int c)
{
    assert(cursorCol >= prompt.size());
    size_t pos = cursorCol - prompt.size();

    switch (c) {
    case KEY_LEFT:
        if (pos > 0) {
            cursorCol--;
            updateCursor();
        }
        return true;

    case KEY_RIGHT:
        if (pos < pEdit->size()) {
            cursorCol++;
            updateCursor();
        }
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

bool ConsoleSession::handleEdit(int c)
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
            updateCursor();
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

bool ConsoleSession::handleVisible(int c)
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

