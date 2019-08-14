#include <stdbool.h>
#include <string.h>
#include "board.h"
#include "common.h"

int position_equal(const position_t *pos1, const position_t *pos2)
{
	return pos1->x == pos2->x
		&& pos1->y == pos2->y
		&& pos1->z == pos2->z;
}

chip_t board_get(const board_t *board, const position_t *pos)
{
	if(pos->y < 0 || pos->y >= MAX_ROW_COUNT)
		return 0;
	if(pos->x < 0 || pos->x >= MAX_COL_COUNT)
		return 0;
	if(pos->z < 0 || pos->z >= MAX_HEIGHT)
		return 0;

	return board->columns[pos->y][pos->x].chips[pos->z];
}

void board_set(board_t *board, const position_t *pos, chip_t chip)
{
	if(pos->y < 0 || pos->y >= MAX_ROW_COUNT)
		return;
	if(pos->x < 0 || pos->x >= MAX_COL_COUNT)
		return;
	if(pos->z < 0 || pos->z >= MAX_HEIGHT)
		return;

	board->columns[pos->y][pos->x].chips[pos->z] = chip;
}

static int column_height(board_t *board, int y, int x)
{
	int k;

	if(y < 0 || y >= MAX_ROW_COUNT)
		return 0;
	if(x < 0 || x >= MAX_COL_COUNT)
		return 0;

	for(k = MAX_HEIGHT-1; k >= 0; --k)
	{
		chip_t chip = board->columns[y][x].chips[k];
		if(chip)
			return k + 1;
	}
	return 0;
}

static chip_t safe_get(const board_t *board, int y, int x, int k)
{
	if(y < 0 || y >= MAX_ROW_COUNT)
		return 0;
	if(x < 0 || x >= MAX_COL_COUNT)
		return 0;
	if(k < 0 || k >= MAX_HEIGHT)
		return 0;

	return board->columns[y][x].chips[k];
}

static int selectable(board_t *board, int y, int x)
{
	int i, j;
	int h = column_height(board, y, x);
	int l, r;

	/* No chip or a blocker? */
	if(h == 0 || (safe_get(board, y, x, h - 1) & CHIP_CATEGORY_MASK) == CHIP_CATEGORY_BLOCK)
		return 0;

	/* Anything on top? */
	for(i = -1; i <= 1; ++i)
		for(j = -1; j <= 1; ++j)
			if(safe_get(board, y+i, x+j, h) != 0)
				return 0;

	/* Anything to the left or right? */
	l = safe_get(board, y-1, x-2, h-1)
		|| safe_get(board, y, x-2, h-1)
		|| safe_get(board, y+1, x-2, h-1);

	r = safe_get(board, y-1, x+2, h-1)
		|| safe_get(board, y, x+2, h-1)
		|| safe_get(board, y+1, x+2, h-1);

	if(l && r)
		return 0;

	return h;
}

positions_t* get_selectable_positions(board_t *board)
{
	int i, j;

	positions_t* positions = malloc(sizeof(positions_t));
	positions->count = 0;

	for(i = 0; i < MAX_ROW_COUNT; ++i) {
		for(j = 0; j < MAX_COL_COUNT; ++j) {
			int h = selectable(board, i, j);
			if(h) {
				positions->positions[positions->count].y = i;
				positions->positions[positions->count].x = j;
				positions->positions[positions->count].z = h - 1;
				++positions->count;
			}
		}
	}

	return positions;
}

static int colorize(board_t *board, chip_t *pairs, int pile_size, board_t *result_board)
{
	int i, j;

	positions_t *positions = get_selectable_positions(board);

	if(positions->count < 2) {
		free(positions);
		return 0;
	}

	shuffle(positions->positions, positions->count, sizeof(position_t));

	if(pile_size == 2) {
		board_set(result_board, &positions->positions[0], pairs[0]);
		board_set(result_board, &positions->positions[1], pairs[1]);

		free(positions);
		return 1;
	}

	for(i = 0; i < positions->count - 1; ++i) {
		for(j = i + 1; j < positions->count; ++j) {
			const position_t* p1 = &positions->positions[i];
			const position_t* p2 = &positions->positions[j];

			board_set(board, p1, 0);
			board_set(board, p2, 0);

			if(colorize(board, &pairs[2], pile_size - 2, result_board)) {
					board_set(result_board, p1, pairs[0]);
					board_set(result_board, p2, pairs[1]);

					free(positions);
					return 1;
				}

			board_set(board, p1, CHIP_PLACEHOLDER);
			board_set(board, p2, CHIP_PLACEHOLDER);
		}
	}

	free(positions);
	return 0;
}

static void get_pile(chip_t pile[CHIP_COUNT])
{
	int rank, count;
	int index = 0;

	/* Simples */
	for(rank = 1; rank <= 9; ++rank) {
		for(count = 0; count < 4; ++count)
			pile[index++] = CHIP_CATEGORY_CHARACTER | rank;
		for(count = 0; count < 4; ++count)
			pile[index++] = CHIP_CATEGORY_DOTS | rank;
		for(count = 0; count < 4; ++count)
			pile[index++] = CHIP_CATEGORY_BAMBOO | rank;
	}
	/* Honors */
	for(rank = 1; rank <= 4; ++rank) {
		for(count = 0; count < 4; ++count)
			pile[index++] = CHIP_CATEGORY_WINDS | rank;
	}
	for(rank = 1; rank <= 3; ++rank) {
		for(count = 0; count < 4; ++count)
			pile[index++] = CHIP_CATEGORY_DRAGONS | rank;
	}
	/* Bonus */
	for(rank = 1; rank <= 4; ++rank)
		pile[index++] = CHIP_CATEGORY_SEASONS | rank;
	for(rank = 1; rank <= 4; ++rank)
		pile[index++] = CHIP_CATEGORY_FLOWERS | rank;
}

static void clear_board(board_t *board)
{
	int i, j;

	for(i = 0; i < MAX_ROW_COUNT; ++i)
		for(j = 0; j < MAX_COL_COUNT; ++j)
			memset(&board->columns[i][j].chips[0], 0, sizeof(chip_t) * MAX_HEIGHT);

	board->chip_count = 0;
}

void generate_board(board_t *board, map_t *map)
{
	int i;
	board_t tmp;
	chip_t pile[CHIP_COUNT];

	/* prepare pile */
	get_pile(pile);
	shuffle(&pile[136], 4, sizeof(chip_t)); /* Shuffle seasons */
	shuffle(&pile[140], 4, sizeof(chip_t)); /* Shuffle flowers */
	shuffle(pile, 72, 2 * sizeof(chip_t)); /* Shuffle everything, keeping pairs together */

	/* Build temporary board that will be taken apart according to the rules in random order */
	clear_board(&tmp);
	for(i = 0; i < CHIP_COUNT; ++i) {
		const int x = map->chip[i].x;
		const int y = map->chip[i].y;
		const int z = map->chip[i].z;

		tmp.columns[y][x].chips[z] = CHIP_PLACEHOLDER;
	}
	for(i = 0; i < map->block_count; ++i) {
		const int x = map->block[i].x;
		const int y = map->block[i].y;
		const int z = map->block[i].z;

		tmp.columns[y][x].chips[z] = CHIP_CATEGORY_BLOCK | 1;
	}

	/* Now take the temporary board apart and fill the result board that way */
	clear_board(board);
	colorize(&tmp, pile, CHIP_COUNT, board);

	/* Blockers are still missing since they couldn't be taken, add them now */
	for(i = 0; i < map->block_count; ++i) {
		const int x = map->block[i].x;
		const int y = map->block[i].y;
		const int z = map->block[i].z;

		board->columns[y][x].chips[z] = CHIP_CATEGORY_BLOCK | 1;
	}

	board->chip_count = CHIP_COUNT + map->block_count;
}

int fits(chip_t a, chip_t b)
{
	if((a & CHIP_CATEGORY_MASK) == CHIP_CATEGORY_BLOCK || (b & CHIP_CATEGORY_MASK) == CHIP_CATEGORY_BLOCK) /* Blockers */
		return false;

	int bonus = a & CHIP_CATEGORY2_MASK;
	if(bonus == CHIP_CATEGORY2_BONUS) /* Flowers and seasons */
		return (a & CHIP_CATEGORY_MASK) == (b & CHIP_CATEGORY_MASK);
	else
		return a == b;
}
