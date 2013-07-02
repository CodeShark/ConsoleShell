///////////////////////////////////////////////////////////////////////////////
//
// command_interpreter.cpp
//
// Copyright (c) 2011-2013 Eric Lombrozo
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

#include "command_interpreter.h"
#include "console_session.h"

#include <iostream>
#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdexcept>

typedef std::map<std::string, fAction>  command_map_t;
command_map_t command_map;

std::vector<std::string> input_history;
std::vector<std::string> output_history;

ConsoleSession cs("> ");

///////////////////////////////////
//
// Common Functions
//

result_t console_help(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() > 0) {
        return "help - displays help information.";
    }

    std::stringstream ss;
    ss << "List of commands:";
    command_map_t::iterator it = command_map.begin();
    for (; it != command_map.end(); ++it) {
        ss << std::endl << it->second(true, params);
    }
    return ss.str();
}

result_t console_echo(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() == 0) {
        return "echo <arg1> [<arg2> <arg3> ...] - repeats the arguments to output.";
    }

    std::string result = params[0];
    for (auto it = params.begin() + 1; it != params.end(); ++it) {
        result += " ";
        result += *it;
    }
    return result;
}

///////////////////////////////////
//
// Command Registration Functions
//
void addCommand(const std::string& cmdName, fAction cmdFunc)
{
    command_map[cmdName] = cmdFunc;
}

void initCommands()
{
    command_map.clear();
    command_map["help"] = &console_help;
    command_map["echo"] = &console_echo;
}

//////////////////////////////////
//
// I/O Functions
//
bool getInput(std::string& input)
{
    input = cs.getLine();
    if (input != "") {
        input_history.push_back(input);
        return true;
    }
    return false;
}

void doOutput(const std::string& output)
{
    output_history.push_back(output);
    std::stringstream out;
    out << "Out: [" << output_history.size() << "] " << output;
    cs.putLine(out.str());
}

void doError(const std::string& error)
{
    std::stringstream err;
    err << "Error: " << error;
    cs.putLine(err.str());
}

void showCommand(const std::string command, params_t& params)
{
    std::stringstream cmd;
    cmd << "In: " << command;
    for (uint i = 0; i < params.size(); i++) {
        cmd << " " << params[i];
    }
    cs.putLine(cmd.str());
}

void newline()
{
    cs.putLine("\n");
}
 
// precondition:    input is not empty
// postcondition:   command contains the first token, params contains the rest
void parseInput(const std::string& input, std::string& command, params_t& params)
{
    using namespace std;
    istringstream iss(input);
    vector<string> tokens;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(tokens));

    command = tokens[0];
    params.clear();
    for (uint i = 1; i < tokens.size(); i++) {
       params.push_back(tokens[i]);
    } 
}

// tilde negates the number
int parseInt(const std::string& text)
{
    if (text.size() == 0) return 0;

    bool bNeg = false;
    uint start = 0;
    std::string num = text;
    if (num[0] == '~') {
        bNeg = true;
        start = 1;
        num = num.substr(1);
    }

    // make sure we only have digits 0-9
    for (uint i = start; i < num.size(); i++) {
        if (num[i] < '0' || num[i] > '9') {
            throw std::runtime_error("NaN");
        }
    }

    int n = strtol(text.c_str(), NULL, 10);
    if (bNeg) n *= -1;
    return n;
}

void substituteTokens(params_t& params)
{
    int last_output = output_history.size();

    for (uint i = 0; i < params.size(); i++) {
        if (params[i] == "") continue;

        if (params[i] == "null") {
            params[i] = "";
        }
        else if (params[i][0] == '%') {
            int n;
            try {
                n = parseInt(params[i].substr(1));
            }
            catch (...) {
                std::stringstream ss;
                ss << "Invalid token " << params[i] << ".";
                throw std::runtime_error(ss.str());
            }

            if (n <= 0) {
                n = last_output - n;
            }
            n--;

            if (n < 0 || n > last_output) {
                std::stringstream ss;
                ss << "Index out of range for token " << params[i] << ".";
                throw std::runtime_error(ss.str());
            }

            params[i] = output_history[n];
        }
    }
}

//////////////////////////////////
//
// Command interpreter
//
std::string execCommand(const std::string& command, params_t& params)
{
    command_map_t::iterator it = command_map.find(command);
    if (it == command_map.end()) {
        std::stringstream ss;
        ss << "Invalid command " << command << ".";
        throw std::runtime_error(ss.str());
    }

    bool bHelp = (params.size() == 1 && (params[0] == "-h" || params[0] == "--help"));
    return it->second(bHelp, params);
}

//////////////////////////////////
//
// Main Loop
//
static void loop()
{
    std::string input;
    std::string output;
    std::string command;
    std::vector<std::string> params;

    while (true) {
        while (!getInput(input));
        if (input == "exit") break;

        parseInput(input, command, params);
        try {
            substituteTokens(params);
            newline();
            showCommand(command, params);
            output = execCommand(command, params);
            doOutput(output);
            newline();
        }
        catch (const std::exception& e) {
            doError(e.what());
            newline();
        }
    }
}

int startConsole(int argc, char** argv)
{
    if (argc == 1) {
        loop();
        return 0;
    }

    params_t params;
    for (int i = 2; i < argc; i++) {
        params.push_back(argv[i]);
    }

    try {
        std::cout << execCommand(argv[1], params) << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
