#pragma once
#ifndef DICE_GUI
#define DICE_GUI
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <Windows.h>
int WINAPI GUIMain();
#endif
#endif
