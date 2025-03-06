#pragma once

#ifndef ___HTTP_H
#define ___HTTP_H

void http_server_init(void);
void http_register_on_draw_text(void (*callback)(const char* text, int x, int y));

#endif
