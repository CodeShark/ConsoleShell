#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <cassert>

#include <sstream>
#include <string>
#include <vector>

#include "command_interpreter.h"

int main(int argc, char *argv[])
{
    initCommands();
    startInterpreter(argc, argv);
    return 0;
}
