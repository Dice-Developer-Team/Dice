#pragma once
#include "DiceSchedule.h"

inline time_t tNow = time(NULL);

void frame_restart(AttrObject& job);
void frame_reload(AttrObject& job);

void auto_save(AttrObject& job);

void check_system(AttrObject& job);

void clear_image(AttrObject& job);

void check_update(AttrObject& job);
void dice_update(AttrObject& job);
void dice_exit();

string print_master();

string list_deck();
string list_extern_deck();
string list_order_ex();