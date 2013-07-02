SHELL=/bin/bash

CXX_FLAGS = -Wall

LIBS = \
    -l ncurses

all: console

console: src/console.cpp src/console_session.cpp src/console_session.h src/dirty_vector.h
	$(CXX) $(CXX_FLAGS) -o $@ $^ \
	$(LIBS)

clean:
	-rm console
