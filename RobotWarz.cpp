#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <regex>
#include <dlfcn.h>

#include "Arena.h"

int main() {
    Arena arena = Arena();
    return arena.game_loop();
}