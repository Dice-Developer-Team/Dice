#pragma once
#include "DiceSchedule.h"

inline time_t tNow = time(NULL);

int sendSelf(const string& msg);

void cq_exit(DiceJob& job);
void frame_restart(DiceJob& job);
void frame_reload(DiceJob& job);

void auto_save(DiceJob& job);

void check_system(DiceJob& job);


void clear_image(DiceJob& job);

void clear_group(DiceJob& job);

void list_group(DiceJob& job);

void cloud_beat(DiceJob& job);
void dice_update(DiceJob& job);
void dice_cloudblack(DiceJob& job);

void log_put(DiceJob& job); 
void global_exit();


string print_master();

string list_deck();
string list_extern_deck();
string list_order_ex();
string list_dice_sister();