#pragma once
#ifndef _NP3_PLUGINS_H_
#define _NP3_PLUGINS_H_
#include "TypeDefs.h"

void Plugins_Init(void);
void Plugins_Release(void);
bool Plugins_InsertMenu(HMENU hMenuBar);
bool Plugins_Command(int cmdID);

#endif
