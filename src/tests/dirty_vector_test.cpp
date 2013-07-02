#include "../dirty_vector.h"
#include <iostream>

int main()
{
    dirty_vector<std::string> dv;
    dv.push_back("first");
    dv.push_back("second");
    dv.push_back("third");

    std::cout << "Initialized" << std::endl;
    for (int i = 0; i < dv.size(); i++) {
        std::cout << dv[i] << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Make dirty." << std::endl;
    std::string& edit = dv.getDirty(1);
    edit = "dirty now";

    for (int i = 0; i < dv.size(); i++) {
        std::cout << dv.getDirty(i) << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Access normally again." << std::endl;
    for (int i = 0; i < dv.size(); i++) {
        std::cout << dv[i] << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Clean." << std::endl;
    dv.clean();

    for (int i = 0; i < dv.size(); i++) {
        std::cout << dv.getDirty(i) << std::endl;
    }
    std::cout << std::endl;

    dv.clean();


    return 0;
}

