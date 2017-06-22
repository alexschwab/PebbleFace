#ifndef APP_MESSAGE_H
#define APP_MESSAGE_H
#pragma once
#include <pebble.h>

void message_init(void);
void message_deinit(void);

void pull_weather(void);

extern char icon[7];
extern TextLayer* s_weather_temp;

#endif
