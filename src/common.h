#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define SCREEN_WIDTH (ScreenWidth())
#define SCREEN_HEIGHT (ScreenHeight())

#define COLOR_FACE (0xffffff)
#define COLOR_FLOWERS (0xbbbbbb)
#define COLOR_SEASONS (0x777777)
#define COLOR_SIDES (0xaaaaaa)
#define COLOR_DEDGES (0x333333)
#define COLOR_MEDGES (0x777777)
#define COLOR_LEDGES (0xffffff)
#define COLOR_BORDER (0x000000)

static inline int min_int(int x, int y)
{
  return x < y ? x : y;
}

static inline int max_int(int x, int y)
{
  return x > y ? x : y;
}

int rrand(int m);

void shuffle(void *obj, size_t nmemb, size_t size);
void topological_sort(void *array, size_t nmemb, size_t size, int (*has_edge)(const void*, const void*));

#endif
