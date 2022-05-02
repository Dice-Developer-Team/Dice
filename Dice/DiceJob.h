#pragma once
#include "DiceSchedule.h"

inline time_t tNow = time(NULL);

int sendSelf(const string& msg);

void cq_exit(AttrObject& job);
void frame_restart(AttrObject& job);
void frame_reload(AttrObject& job);

void auto_save(AttrObject& job);

void check_system(AttrObject& job);


void clear_image(AttrObject& job);

void clear_group(AttrObject& job);

void list_group(AttrObject& job);

void cloud_beat(AttrObject& job);
void dice_update(AttrObject& job);
void dice_cloudblack(AttrObject& job);

void log_put(AttrObject& job); 
void global_exit();


string print_master();

string list_deck();
string list_extern_deck();
string list_order_ex();
string list_dice_sister();