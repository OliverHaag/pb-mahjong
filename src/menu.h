#ifndef MENU_H
#define MENU_H

#include "inkview.h"
#include "messages.h"

extern void show_popup(const ibitmap* background, message_id message, message_id *menu, iv_menuhandler hproc);
extern void show_popup_list(const ibitmap* background, message_id message, char **list, iv_menuhandler hproc);

#endif
