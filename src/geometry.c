#include "common.h"

#include "geometry.h"

void point_change_orientation(int x, int y, int orientation, int *rx, int *ry)
{
#ifdef __EMU__ // bug in emulator
  const int w = 600;
  const int h = 800;
  switch (orientation)
    {
    case 0: /* portrait */
      *rx = x;
      *ry = y;
      break;
    case 1: /* landscape 90 */
      *rx = y;
      *ry = w - x;
      break;
    case 2: /* landscape 270 */
      *rx = h - y;
      *ry = x;
      break;
    case 3: /* portrait 180 */
      *rx = w - x;
      *ry = h - y;
      break;
    }
#else
  *rx = x;
  *ry = y;
#endif
}

int point_in_rect(int x, int y, const struct rect* r)
{
  return
    (x >= r->x && x < r->x + r->w) &&
    (y >= r->y && y < r->y + r->h);
}
