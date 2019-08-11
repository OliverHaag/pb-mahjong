#ifndef BOARD_H
#define BOARD_H

/*
  category - 4-bit
  rank - 4-bit
*/
typedef unsigned char chip_t;

#define MAX_ROW_COUNT 24
#define MAX_COL_COUNT 40
#define MAX_HEIGHT 16
#define CHIP_COUNT 144

typedef struct {
  chip_t chips[MAX_HEIGHT];
} column_t;

typedef struct {
  column_t columns[MAX_ROW_COUNT][MAX_COL_COUNT];
} board_t;

typedef struct {
  int y, x, k;
} position_t;

int position_equal(const position_t *pos1, const position_t *pos2);

typedef struct {
  position_t positions[CHIP_COUNT];
  int count;
} positions_t;

positions_t* get_selectable_positions(board_t *board);

chip_t board_get(const board_t *board, const position_t *pos);
void board_set(board_t *board, const position_t *pos, chip_t chip);

/*******************************************************/

typedef struct tag_map {
  char *name;
  int row_count;
  int col_count;
  struct {
    int x, y, z;
  } map[CHIP_COUNT];
} map_t;

void generate_board(board_t *board, map_t *map);

int fits(chip_t a, chip_t b);

#endif
