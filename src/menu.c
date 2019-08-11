#include "menu.h"

#include "geometry.h"

#define MENU_FONT_NAME (DEFAULTFONT)
#define MENU_FONT_SIZE (40)
#define MENU_MARGIN (20)
#define MENU_ITEM_HEIGHT (60)
#define MENU_SEPARATOR_HEIGHT (20)

typedef struct {
  const ibitmap *background;
  message_id message;

  message_id *items;
  char **list;
  int count;
  iv_menuhandler proc;
  int current;

  int calc_required;
  struct rect bounds;
} menu_t;

static ifont *g_menu_font = NULL;
static ifont *get_menu_font(void)
{
  if (g_menu_font == NULL)
    g_menu_font = OpenFont(MENU_FONT_NAME, MENU_FONT_SIZE, 1);
  return g_menu_font;
}

void menu_calc(menu_t *menu)
{
  if (!menu->calc_required)
    return;
  menu->calc_required = 1;

  SetFont(get_menu_font(), BLACK);

  int i;
  menu->bounds.w = 0;
  menu->bounds.h = 0;
  if(menu->items != NULL)
    {
      for (i = 0; i < menu->count; ++i)
        {
          if (menu->items[i] > MSG_NONE)
            {
              const int lw = StringWidth((char*)get_message(menu->items[i]));
              if (lw > menu->bounds.w)
                menu->bounds.w = lw;
              menu->bounds.h += MENU_ITEM_HEIGHT;
            }
          else /*separator*/
            {
              menu->bounds.h += MENU_SEPARATOR_HEIGHT;
            }
        }
    }
  else if(menu->list != NULL)
    {
      for (i = 0; i < menu->count; ++i)
        {
          if (menu->list[i] != NULL)
            {
              const int lw = StringWidth(menu->list[i]);
              if (lw > menu->bounds.w)
                menu->bounds.w = lw;
              menu->bounds.h += MENU_ITEM_HEIGHT;
            }
          else /*separator*/
            {
              menu->bounds.h += MENU_SEPARATOR_HEIGHT;
            }
        }
      menu->bounds.h += MENU_SEPARATOR_HEIGHT + MENU_ITEM_HEIGHT;
    }

  menu->bounds.x = (ScreenWidth() - menu->bounds.w - 2 * MENU_MARGIN) / 2 + MENU_MARGIN;
  menu->bounds.y = ScreenHeight() - menu->bounds.h - MENU_MARGIN - 30;
}

static void draw_popup(menu_t *menu)
{
  int i, y;

  menu_calc(menu);

  SetFont(get_menu_font(), BLACK);

  FillArea(menu->bounds.x - MENU_MARGIN,
           menu->bounds.y - MENU_MARGIN,
           menu->bounds.w + 2 * MENU_MARGIN,
           menu->bounds.h + 2 * MENU_MARGIN,
           WHITE);
  DrawRect(menu->bounds.x - MENU_MARGIN,
           menu->bounds.y - MENU_MARGIN,
           menu->bounds.w + 2 * MENU_MARGIN,
           menu->bounds.h + 2 * MENU_MARGIN,
           BLACK);

  y = menu->bounds.y;
  if(menu->items != NULL)
    {
      for (i = 0; i < menu->count; ++i)
        {
          if (menu->items[i] > MSG_NONE)
            {
              DrawTextRect(menu->bounds.x,
                           y,
                           menu->bounds.w,
                           MENU_ITEM_HEIGHT,
                           (char*)get_message(menu->items[i]),
                           ALIGN_LEFT | VALIGN_MIDDLE);

              y += MENU_ITEM_HEIGHT;
            }
          else /*separator*/
            {
              const int sy = y + MENU_SEPARATOR_HEIGHT / 2;
              DrawLine(menu->bounds.x, sy, menu->bounds.x + menu->bounds.w, sy, BLACK);
              y += MENU_SEPARATOR_HEIGHT;
            }
        }
    }
  else if(menu->list != NULL)
    {
      for (i = 0; i < menu->count; ++i)
        {
          if (menu->list[i] != NULL)
            {
              DrawTextRect(menu->bounds.x,
                           y,
                           menu->bounds.w,
                           MENU_ITEM_HEIGHT,
                           menu->list[i],
                           ALIGN_LEFT | VALIGN_MIDDLE);

              y += MENU_ITEM_HEIGHT;
            }
          else /*separator*/
            {
              const int sy = y + MENU_SEPARATOR_HEIGHT / 2;
              DrawLine(menu->bounds.x, sy, menu->bounds.x + menu->bounds.w, sy, BLACK);
              y += MENU_SEPARATOR_HEIGHT;
            }
        }
        const int sy = y + MENU_SEPARATOR_HEIGHT / 2;
        DrawLine(menu->bounds.x, sy, menu->bounds.x + menu->bounds.w, sy, BLACK);
        y += MENU_SEPARATOR_HEIGHT;
        DrawTextRect(menu->bounds.x,
                     y,
                     menu->bounds.w,
                     MENU_ITEM_HEIGHT,
                     (char *)get_message(MSG_BACK),
                     ALIGN_LEFT | VALIGN_MIDDLE);
    }
}

static void menu_update(menu_t *menu)
{
  PartialUpdate(menu->bounds.x - MENU_MARGIN,
                menu->bounds.y - MENU_MARGIN,
                menu->bounds.w + 2 * MENU_MARGIN,
                menu->bounds.h + 2 * MENU_MARGIN);
}

static menu_t g_menu1;

static int menu_handler(int type, int par1, int par2)
{
  menu_t *menu = &g_menu1;

  switch (type)
    {
    case EVT_SHOW:
      if (menu->background != NULL || menu->message != MSG_NONE)
        ClearScreen();
      if (menu->background != NULL)
        StretchBitmap(0, 0, ScreenWidth(), ScreenHeight(), (ibitmap*)menu->background, 0);
      if (menu->message != MSG_NONE)
        {
          const int x_margin = 100;
          const int y_margin = 130;
          const int height = 150;
          FillArea(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, WHITE);
          DrawSelection(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, DGRAY);

          ifont *font = OpenFont(MENU_FONT_NAME, 40, 1);
          SetFont(font, BLACK);
          DrawTextRect(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, (char*)get_message(menu->message), ALIGN_CENTER | VALIGN_MIDDLE);

          CloseFont(font);
        }
      draw_popup(menu);
      if (menu->background != NULL || menu->message != MSG_NONE)
        FullUpdate();
      else
        menu_update(menu);
      break;
    case EVT_KEYPRESS:
      switch (par1)
        {
        case IV_KEY_UP:
          if(menu->items != NULL)
            {
              do
                {
                  menu->current = (menu->current + menu->count - 1) % menu->count;
                }
              while (menu->items[menu->current] <= MSG_NONE);
            }
          else if(menu->list != NULL)
            {
              do
                {
                  menu->current = (menu->current + menu->count - 1) % menu->count;
                }
              while (menu->list[menu->current] == NULL);
            }

          draw_popup(menu);
          menu_update(menu);
          break;
        case IV_KEY_DOWN:
          if(menu->items != NULL)
            {
              do
                {
                  menu->current = (menu->current + 1) % menu->count;
                }
              while (menu->items[menu->current] <= MSG_NONE);
            }
          else if(menu->list != NULL)
            {
              do
                {
                  menu->current = (menu->current + 1) % menu->count;
                }
              while (menu->list[menu->current] == NULL);
            }

          draw_popup(menu);
          menu_update(menu);
          break;
        case IV_KEY_OK:
          if(menu->items != NULL)
            menu->proc(menu->items[menu->current]);
          else if(menu->list != NULL)
            menu->proc(menu->current);
          break;
        }
      break;
    case EVT_POINTERDOWN:
      {
        menu_calc(menu);

        int rx, ry;
        point_change_orientation(par1, par2, GetOrientation(), &rx, &ry);

        if (point_in_rect(rx, ry, &menu->bounds))
          {
            int y = menu->bounds.y;
            int i;
            if(menu->items != NULL)
              {
                for (i = 0; i < menu->count; ++i)
                  {
                    if (menu->items[i] > MSG_NONE)
                      {
                        if (ry >= y && ry < y + MENU_ITEM_HEIGHT)
                          {
                            menu->current = i;
                            draw_popup(menu);
                            menu_update(menu);
                            menu->proc(menu->items[menu->current]);
                            break;
                          }

                        y += MENU_ITEM_HEIGHT;
                      }
                    else /*separator*/
                      {
                        y += MENU_SEPARATOR_HEIGHT;
                      }
                  }
              }
            else if(menu->list != NULL)
              {
                for (i = 0; i < menu->count; ++i)
                  {
                    if (menu->list[i] != NULL)
                      {
                        if (ry >= y && ry < y + MENU_ITEM_HEIGHT)
                          {
                            menu->current = i;
                            draw_popup(menu);
                            menu_update(menu);
                            menu->proc(menu->current);
                            break;
                          }

                        y += MENU_ITEM_HEIGHT;
                      }
                    else /*separator*/
                      {
                        y += MENU_SEPARATOR_HEIGHT;
                      }
                  }
                /* Back item */
                if (i >= menu->count)
                  {
                    y += MENU_SEPARATOR_HEIGHT;
                    if (ry >= y && ry < y + MENU_ITEM_HEIGHT)
                      {
                        menu->current = i;
                        draw_popup(menu);
                        menu_update(menu);
                        menu->proc(-1);
                      }
                  }
              }
          }
      }
      break;
    }
  return 0;
}

void show_popup(const ibitmap* background, message_id message, message_id *items, iv_menuhandler hproc)
{
  menu_t *menu = &g_menu1;

  menu->calc_required = 1;
  menu->background = background;
  menu->message = message;
  menu->items = items;
  menu->list = NULL;
  menu->count = 0;
  while (items[menu->count] != MSG_NONE)
    ++menu->count;
  menu->proc = hproc;
  menu->current = 0;

  SetEventHandler(menu_handler);
}

void show_popup_list(const ibitmap* background, message_id message, char **list, iv_menuhandler hproc)
{
  menu_t *menu = &g_menu1;

  menu->calc_required = 1;
  menu->background = background;
  menu->message = message;
  menu->items = NULL;
  menu->list = list;
  menu->count = 0;
  while (list[menu->count] != NULL)
    ++menu->count;
  menu->proc = hproc;
  menu->current = 0;

  SetEventHandler(menu_handler);
}
