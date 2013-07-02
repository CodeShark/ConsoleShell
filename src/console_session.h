///////////////////////////////////////////////////////////////////////////////
//
// console_session.h
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

#ifndef _CONSOLE_SESSION__H_
#define _CONSOLE_SESSION__H_

#include <cassert>

#include "dirty_vector.h"
#include <string>
#include <vector>

#include <curses.h>

enum {
    MAP_NONE = 0,
    MAP_WRAP_AROUND
};

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
    // screen operations
    void update();
    void scrollTo(unsigned int row) { scrollRows = row; update(); }
    void autoScroll(unsigned int row);

    // cursor motion and output operations
    int logical_move(int row, int col, bool bAutoScroll = true);
    int logical_mvchgat(int row, int col, int n, attr_t attr, short color, const void* opts, bool bAutoScroll = true);
    int logical_mvaddch(int row, int col, const chtype c, bool bAutoScroll = true);
    int logical_mvaddstr(int row, int col, const char* str, bool bAutoScroll = true);

    // input and edit operations
    void replaceEdit(std::string& newEdit);
    bool handleMotion(int c); // motion keys
    bool handleEdit(int c); // insertion/deletion keys
    bool handleVisible(int c); // handle visible character keys

public:
    // constructors & destructors
    ConsoleSession(const std::string& _prompt = "> ", int _mode = MAP_WRAP_AROUND);
    ~ConsoleSession();

    // line operations
    std::string getLine();
    void putLine(const std::string& line);

    bool isCursorInScreen() const;
};

#endif // _CONSOLE_SESSION__H_
