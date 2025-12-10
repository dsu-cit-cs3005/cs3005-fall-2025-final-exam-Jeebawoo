#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <regex>
#include <dlfcn.h>
#include <random>
#include <thread>
#include <chrono>

#include "Arena.h"

Arena::Arena() : m_height(10), m_width(10) {
    // load_robots();
    m_board.resize(m_height*m_width);
    for (size_t i = 0; i < m_board.size(); i++) {
        m_board[i] = '.';
    }
}

Arena::Arena(int height, int width) : m_height(height), m_width(width) {
    // load_robots();
    // load arena config
    m_board.resize(m_height*m_width);
    for (size_t i = 0; i < m_board.size(); i++) {
        m_board[i] = '.';
    }
}

Arena::~Arena() {}

void Arena::load_robots() {
    std::vector<std::string> uncompiled_robots;

    std::regex pattern("Robot_.*.cpp");

    DIR *dir;
    struct dirent *ent;
    
    if ((dir = opendir("./")) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string filename = ent->d_name;
            if (std::regex_match(filename, pattern)) {
                uncompiled_robots.push_back(filename);
            }
        }
        closedir(dir);
    } else {
        perror("Could not open directory");
    }

    std::string compile_cmd;
    std::string filename;
    std::string shared_lib;
    for (size_t i = 0; i < uncompiled_robots.size(); i++) {
        filename = uncompiled_robots[i];
        shared_lib = filename;
        size_t pos = shared_lib.rfind(".cpp");
        shared_lib.replace(pos, 4, ".so");
        compile_cmd = "g++ -shared -fPIC -o " + shared_lib + " " + filename + " RobotBase.o -I. -std=c++20";
        std::cout << "Compiling " << filename << " to " << shared_lib << "...\n";

        int compile_result = std::system(compile_cmd.c_str());
        if (compile_result != 0) 
        {
            std::cerr << "Failed to compile " << filename << " with command: " << compile_cmd << std::endl;
            continue;
        }

        shared_lib = "./" + shared_lib;
        void* handle = dlopen(shared_lib.c_str(), RTLD_LAZY);
        if (!handle) 
        {
            std::cerr << "Failed to load " << shared_lib << ": " << dlerror() << std::endl;
            continue;
        }

        RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
        if (!create_robot) 
        {
            std::cerr << "Failed to find create_robot in " << shared_lib << ": " << dlerror() << std::endl;
            dlclose(handle);
            continue;
        }
        
        RobotBase* robot = create_robot();
        m_robots_list.push_back(robot);
        m_robots_handles.push_back(handle);
    }
}

int Arena::random_index() {
    return std::rand() % (m_height*m_width - 1);
}

void Arena::place_obstacles(int mounds, int pits, int flames) {
    // for each obstacle type while there are some to be placed, choose a random cell
    // if full choose again, otherwise place ob on board and decrement
    int cell;
    int i;

    i = mounds;
    while (i > 0) {
        cell = random_index();
        if (m_board[cell] == '.') {
            m_board[cell] = 'M';
            i--;
        }
    }
    
    i = pits;
    while (i > 0) {
        cell = random_index();
        if (m_board[cell] == '.') {
            m_board[cell] = 'P';
            i--;
        }
    }
    
    i = flames;
    while (i > 0) {
        cell = random_index();
        if (m_board[cell] == '.') {
            m_board[cell] = 'F';
            i--;
        }
    }
}

void Arena::index_to_pos(int index, int& row, int& col) {
    (void) row;
    col = index % m_width;
    row = (index - col) / m_height;
}

void Arena::place_robots() {
    // for each robot select a random cell
    // if taken choose again
    // place robot on board and set robots position
    for (size_t i = 0; i < m_robots_list.size(); i++) {
        int cell = random_index();
        while (m_board[cell] != '.') {
            cell = random_index();
        }
        m_board[cell] = 'R';
        int row = 0;
        int col = 0;
        index_to_pos(cell, row, col);
        m_robots_list[i]->move_to(row, col);
        m_robots_list[i]->set_boundaries(m_height, m_width);
    }
}

int Arena::position_to_robot(int row, int col) {
    // iterates through each robot checking row and col
    // if matches then return index of robot
    // else return -1
    int curr_row;
    int curr_col;
    for (size_t i = 0; i < m_robots_list.size(); i++) {
        m_robots_list[i]->get_current_location(curr_row, curr_col);
        if (curr_row == row && curr_col == col) {
            return i;
        }
    }
    return -1;
}

void Arena::display_board() {
    // displayes the formatted board with each cell as a char
    // if an R is encountered uses pos to robot to find its char
    std::cout << std::endl << "   ";
    for (int cl = 0; cl < m_width; cl++) {
        if (cl < 10) {
            std::cout << " " << cl << " ";
        } else {
            std::cout << cl << " ";
        }
    }
    std::cout << std::endl;
    for (int rw = 0; rw < m_height; rw++) {
        std::cout << rw << "  ";
        for (int cl = 0; cl < m_width; cl++) {
            char cell = m_board[pos_to_index(rw, cl)];
            std::cout << " " << cell;
            if (cell == 'R' && m_robots_list[position_to_robot(rw, cl)]->m_character) {
                std::cout << m_robots_list[position_to_robot(rw, cl)]->m_character;
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl << std::endl;
    }
}

bool Arena::is_winner() {
    // look over all cells and determines if one robot remains
    // if yes then uses pos to robot to get its name and prints victory
    // else return false
    int row = 0;
    int col = 0;
    bool first_time = true;
    for (size_t i = 0; i < m_board.size(); i++) {
        if (m_board[i] == 'R') {
            if (!first_time) {
                return false;
            }
            index_to_pos(i, row, col);
            first_time = false;
        }
    }
    std::cout << "End of game! " << m_robots_list[position_to_robot(row, col)]->m_name << " Wins!!!" << std::endl;
    return true;
}

int Arena::pos_to_index(int row, int col) {
    return row*m_width + col;
}

bool Arena::pos_in_bounds(int row, int col) {
    return (row > 0 && row < m_height) && (col > 0 && col < m_width);
}

std::vector<RadarObj> Arena::scan_radar(int dir, int start_row, int start_col) {
    // switch on direction for iteration loop params and kernal shape
    // iterate kernal one away from robot and check all kernal cells for radobj
    // for each scan add to vec
    // continue scan until all kernal cells are out of bounds
    // return vec
    std::vector<RadarObj> scanned_objects;
    bool in;
    int row;
    int col;
    switch (dir) {
        case 0:
            // all around
            row = start_row;
            col = start_col;
            while (true) {
                in = false;
                // top right
                if (pos_in_bounds(row+1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col=1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col=1});
                }
                // middle right
                if (pos_in_bounds(row, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col+1});
                }
                // bottom right
                if (pos_in_bounds(row-1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col+1});
                }
                // bottom middle
                if (pos_in_bounds(row-1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col});
                }
                // bottom left
                if (pos_in_bounds(row-1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col-1});
                }
                // middle left
                if (pos_in_bounds(row, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col-1});
                }
                // top left
                if (pos_in_bounds(row+1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col-1});
                }
                // top middle
                if (pos_in_bounds(row+1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col});
                }
                row++;
            }
            break;
        case 1:
            // up
            row = start_row + 1;
            col = start_col;
            while (true) {
                in = false;
                // middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // left
                if (pos_in_bounds(row, col-1)) {
                    in =true;
                    char cell = m_board[pos_to_index(row, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col-1});
                }
                // right
                if (pos_in_bounds(row, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col+1});
                }
                if (!in) {
                    break;
                }
                row++;
            }
            break;
        case 2:
            // up right
            row = start_row + 1;
            col = start_col + 1;
            while (true) {
                in = false;
                // top left
                if (pos_in_bounds(row+1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col-1});
                }
                // top middle
                if (pos_in_bounds(row+1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col});
                }
                // middle middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // middle right
                if (pos_in_bounds(row, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col+1});
                }
                // bottom right
                if (pos_in_bounds(row-1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col+1});
                }
                if (!in) {
                    break;
                }
                row++;
                col++;
            }
            break;
        case 3:
            // right
            row = start_row;
            col = start_col + 1;
            while (true) {
                in = false;
                // middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // bottom
                if (pos_in_bounds(row-1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col});
                }
                // top
                if (pos_in_bounds(row+1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col});
                }
                if (!in) {
                    break;
                }
                col++;
            }
            break;
        case 4:
            // down right
            row = start_row - 1;
            col = start_col + 1;
            while (true) {
                in = false;
                // top right
                if (pos_in_bounds(row+1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col+1});
                }
                // middle right
                if (pos_in_bounds(row, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col+1});
                }
                // middle middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // bottom middle
                if (pos_in_bounds(row-1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col});
                }
                // bottom left
                if (pos_in_bounds(row-1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col-1});
                }
                if (!in) {
                    break;
                }
                row--;
                col++;
            }
            break;
        case 5:
            // down
            row = start_row - 1;
            col = start_col;
            while (true) {
                in = false;
                // middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // left
                if (pos_in_bounds(row, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col-1});
                }
                // right
                if (pos_in_bounds(row, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col+1});
                }
                if (!in) {
                    break;
                }
                row--;
            }
            break;
        case 6:
            // down left
            row = start_row - 1;
            col = start_col - 1;
            while (true) {
                in = false;
                // bottom right
                if (pos_in_bounds(row-1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col+1});
                }
                // bottom middle
                if (pos_in_bounds(row-1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col});
                }
                // middle middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // middle left
                if (pos_in_bounds(row, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col-1});
                }
                // top left
                if (pos_in_bounds(row+1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col-1});
                }
                if (!in) {
                    break;
                }
                row--;
                col--;
            }
            break;
        case 7:
            // left
            row = start_row;
            col = start_col - 1;
            while (true) {
                in = false;
                // middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // bottom
                if (pos_in_bounds(row-1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col});
                }
                // top
                if (pos_in_bounds(row+1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col});
                }
                if (!in) {
                    break;
                }
                col--;
            }
            break;
        case 8:
            // up left
            row = start_row + 1;
            col = start_col - 1;
            while (true) {
                in = false;
                // bottom left
                if (pos_in_bounds(row-1, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row-1, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row-1, col-1});
                }
                // middle left
                if (pos_in_bounds(row, col-1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col-1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col-1});
                }
                // middle middle
                if (pos_in_bounds(row, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row, col});
                }
                // top middle
                if (pos_in_bounds(row+1, col)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col});
                }
                // top right
                if (pos_in_bounds(row+1, col+1)) {
                    in = true;
                    char cell = m_board[pos_to_index(row+1, col+1)];
                    if (cell != '.') scanned_objects.push_back(RadarObj {cell, row+1, col+1});
                }
                if (!in) {
                    break;
                }
                row++;
                col--;
            }
            break;
        default:
            break;
    }
    return scanned_objects;
}

void Arena::handle_shot(WeaponType weapon, int aim_row, int aim_col, int start_row, int start_col) {
    (void) aim_row;
    (void) aim_col;
    (void) start_row;
    (void) start_col;
    // iterates through all affected weapon cells
    // for each robot encountered finds its index with pos to robot
    // it then calls do damage
    switch (weapon) {
        case flamethrower:
            // 30-50 dp, 4 long, 3 wide ray
            // ???
            break;
        case railgun:
            // 10-20 dp, 1 wide piercing ray
            // ???
            break;
        case grenade:
            // 10-40 dp, 3x3 area anywhere
            // ???
            break;
        case hammer:
            // 50-60 dp, one adjecent cell
            // ???
            break;
    }

    std::cout << "\tdid not deal damage" << std::endl << std::endl;
}

void Arena::do_damage(int low_damage, int high_damage, int robot) {
    // calculates damage from range
    // finds robot at index
    // decreases health based on damage calculated and sheilds
    // decrements robots shield
    // if robot dies updates board
    int damage = low_damage + std::rand() % (high_damage - low_damage + 1);
    double block_percent = m_robots_list[robot]->get_armor() * 0.1;
    damage = (damage - damage*block_percent) / 1;
    int health = m_robots_list[robot]->get_health();
    if (damage >= health) {
        m_robots_list[robot]->take_damage(health);
        m_robots_list[robot]->disable_movement();
        int row;
        int col;
        m_robots_list[robot]->get_current_location(row, col);
        m_board[pos_to_index(row, col)] = 'X';
    } else {
        m_robots_list[robot]->take_damage(damage);
        m_robots_list[robot]->reduce_armor(1);
    }
}

void Arena::move_robot(int start_row, int start_col, int dir, int speed) {
    // steps through motion up to speed steps in dir
    // if obstacle encountered handled accordingly
    // updates board
    // updates robot position
    int d_row;
    int d_col;
    int dist = speed;
    int row = start_row;
    int col = start_col;
    switch (dir) {
        case 1:
            // up
            d_row = 1;
            d_col = 0;
            break;
        case 2:
            // up right
            d_row = 1;
            d_col = 1;
            break;
        case 3:
            // right
            d_row = 0;
            d_col = 1;
            break;
        case 4:
            // down right
            d_row = -1;
            d_col = 1;
            break;
        case 5:
            // down
            d_row = -1;
            d_col = 0;
            break;
        case 6:
            // down left
            d_row = -1;
            d_col = -1;
            break;
        case 7:
            // left
            d_row = 0;
            d_col = -1;
            break;
        case 8:
            // up left
            d_row = 1;
            d_col = -1;
            break;
        default:
            d_row = 0;
            d_col = 0;
    }
    int max_speed = m_robots_list[position_to_robot(start_row, start_col)]->get_move_speed();
    if (speed > max_speed) {
        dist = max_speed;
    }
    for (int d = 0; d < dist; d++) {
        row += d_row;
        col += d_col;
        if (!pos_in_bounds(row, col)) {
            row -= d_row;
            col -= d_col;
            break;
        }
        char cell = m_board[pos_to_index(row, col)];
        if (cell == 'M' || cell == 'R' || cell == 'X') {
            row -= d_row;
            col -= d_col;
            break;
        } 
        else if (cell == 'P') {
            m_robots_list[position_to_robot(start_row, start_col)]->disable_movement();
            break;
        }
        else if (cell == 'F') {
            do_damage(30, 50, position_to_robot(start_row, start_col));
        }
    }
    if (m_board[pos_to_index(row, col)] == 'F') {
        row -= d_row;
        col -= d_col;
    }
    m_robots_list[position_to_robot(start_row, start_col)]->move_to(row, col);
    m_board[pos_to_index(start_row, start_col)] = '.';
    m_board[pos_to_index(row, col)] = 'R';
    std::cout << "\tmoving to (" << row << "," << col << ")" << std::endl << std::endl;
}

int Arena::game_loop() {
    // on startup
    // load robots
    load_robots();
    // place obstacles
    place_obstacles(5, 5, 5);   // mounds, pits, flames
    // place robots
    place_robots();
    bool cond = true;
    int round = 1;
    while (cond) {
        // per round:
        // display round number [in loop]
        std::cout << "         =========== starting round " << round << " ===========";
        // display board [board func]
        display_board();

        // per robot:
        for (size_t i = 0; i < m_robots_list.size(); i++) {
            std::cout << m_robots_list[i]->m_name << " " << m_robots_list[i]->m_character;
            int row;
            int col;
            m_robots_list[i]->get_current_location(row, col);
            std::cout << " (" << row << "," << col << ")";
            // check for winner [board func]
            if (is_winner()) {
                return 0;
            }
            // check if alive [in loop]
            if (m_robots_list[i]->get_health() <= 0) {
                // if dead display so and next robot [in loop]
                std::cout << " - is out" << std::endl << std::endl;
                continue;
            }
            std::cout << " Health: " << m_robots_list[i]->get_health() << " Armor: " << m_robots_list[i]->get_armor() << std::endl;
            // call get radar dir [robot func]
            int dir;
            m_robots_list[i]->get_radar_direction(dir);
            // scan using direction and robot pos [arena func]
            std::vector<RadarObj> radar_results = scan_radar(dir, row, col);
            std::cout << "\tradar scan returned ";
            if (!radar_results.size()) {
                std::cout << "nothing" << std::endl;
            } else {
                std::string spacer = "";
                for (size_t i = 0; i < radar_results.size(); i++) {
                    std::cout << spacer;
                    std::cout << radar_results[i].m_type << " at (" << radar_results[i].m_row << "," << radar_results[i].m_col << ")";
                    spacer = ", and ";
                }
                std::cout << std::endl;
            }
            // call process radar [robot func]
            m_robots_list[i]->process_radar_results(radar_results);
            // call get shot [robot func]
            int start_row = row;
            int start_col = col;
            if (m_robots_list[i]->get_shot_location(row, col)) {
                // if true handle shot and damage [arena funcs]
                std::cout << "\tfiring " << m_robots_list[i]->get_weapon() << " at (" << row << "," << col << ")" << std::endl;
                handle_shot(m_robots_list[i]->get_weapon(), row, col, start_row, start_col);
            } else {
                // else call get move direction and handle movement [robot func and board func]
                int dist;
                m_robots_list[i]->get_move_direction(dir, dist);
                std::cout << "\tnot firing" << std::endl;
                move_robot(start_row, start_col, dir, dist);
            }
        }

        // per round:
        // sleep for live replay [in loop]
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // incriment round [in loop]
        round++;
    }
    return 0;
}
