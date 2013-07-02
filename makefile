SHELL=/bin/bash

CXX_FLAGS = -Wall

LIBS = \
    -l ncurses

all: console

console: src/console.cpp
	$(CXX) $(CXX_FLAGS) -o $@ $< \
	$(LIBS)

clean:
	-rm console
