#pragma once

#include <vector>

#include "RobotBase.h"

class Arena {
private:
    std::vector<RobotBase*> m_robots_list;
    std::vector<void*> m_robots_handles;
    int m_height;
    int m_width;
    std::vector<char> m_board;
public:
    Arena();    // basic size and no obstacles
    Arena(int height, int width); // takes width, height, num obstacles
    virtual ~Arena();
    void load_robots();
    int random_index();
    void place_obstacles(int mounds, int pits, int flames);
    void index_to_pos(int index, int row, int col);
    void place_robots();
    int position_to_robot(int row, int col);
    void display_board();
    bool is_winner();
    int pos_to_index(int row, int col);
    bool pos_in_bounds(int row, int col);
    std::vector<RadarObj> scan_radar(int dir, int start_row, int start_col);
    void handle_shot(WeaponType weapon, int aim_row, int aim_col, int start_row, int start_col);
    void do_damage(int low_damage, int high_damage, int robot);
    void move_robot(int start_row, int start_col, int dir, int speed);
    int game_loop();
};