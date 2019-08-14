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

#define CHIP_CATEGORY_MASK 0xf0
#define CHIP_CATEGORY_CHARACTER 0x10
#define CHIP_CATEGORY_DOTS 0x20
#define CHIP_CATEGORY_BAMBOO 0x30
#define CHIP_CATEGORY_WINDS 0x40
#define CHIP_CATEGORY_DRAGONS 0x50
#define CHIP_CATEGORY_SEASONS 0x60
#define CHIP_CATEGORY_FLOWERS 0x70
#define CHIP_CATEGORY_BLOCK 0x80

#define CHIP_CATEGORY2_MASK 0xe0
#define CHIP_CATEGORY2_BONUS 0x60

#define CHIP_PLACEHOLDER 0xff


typedef struct {
	chip_t chips[MAX_HEIGHT];
} column_t;

typedef struct {
	column_t columns[MAX_ROW_COUNT][MAX_COL_COUNT];
	int chip_count;
} board_t;

typedef struct {
	unsigned char x, y, z;
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
	unsigned char row_count;
	unsigned char col_count;
	position_t chip[CHIP_COUNT]; /* Mahjong tiles */
	position_t *block; /* Blocker tiles */
	unsigned int block_count;
} map_t;

void generate_board(board_t *board, map_t *map);

int fits(chip_t a, chip_t b);

#endif
