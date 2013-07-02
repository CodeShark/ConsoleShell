SHELL=/bin/bash

CXX_FLAGS = -Wall -std=c++0x

ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

LIBS = \
    -l ncurses

SRC = \
    src/console.cpp \
    src/console_session.cpp \
    src/command_interpreter.cpp

HEADERS = \
    src/console_session.h \
    src/command_interpreter.h \
    src/dirty_vector.h

all: console

console: $(SRC) $(HEADERS) 
	$(CXX) $(CXX_FLAGS) -o $@ $^ \
	$(LIBS)

clean:
	-rm console
