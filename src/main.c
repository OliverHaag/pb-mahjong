#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "inkview.h"

#include "common.h"
#include "board.h"
#include "maps.h"
#include "bitmaps.h"
#include "geometry.h"
#include "menu.h"
#include "messages.h"

#ifdef EMULATION
#undef STATEPATH
#define STATEPATH "."
#endif

#define SAVED_GAME_PATH (STATEPATH "/pb-mahjong.saved-game")
#define MAPS_DIR (CONFIGPATH "/pb-mahjong")
#define MAPS_EXT ".map"

static int orientation = ROTATE270;
static board_t g_board;
static int row_count;
static int col_count;
static int caret_pos;
static int selection_pos = -1;
static positions_t *g_selectable = NULL;
static int help_index = 0;
static int help_offset = 0;
static int game_active = 0;
static char **map_list;
static int map_list_size;

extern const ibitmap background;

#define HELP_HEIGHT (60)

static int game_handler(int type, int par1, int par2);
static void menu_handler(int index);
static void load_map_handler(int index);
static int main_handler(int type, int par1, int par2);
static void read_state(void);
static void write_state(void);
static int load_game(void);
static void save_game(void);
static void scan_maps(const char *directory);
static map_t *load_map(const char *name);

static struct
{
	position_t positions[144];
	chip_t chips[144];
	int count;
} undo_stack;

static int cmp_pos(const void *p1, const void *p2)
{
	const position_t *pos1 = p1;
	const position_t *pos2 = p2;
	return
		((pos1->y - 1) / 2 * MAX_COL_COUNT + pos1->x) -
		((pos2->y - 1) / 2 * MAX_COL_COUNT + pos2->x);
}

static void rebuild_selectables(void)
{
	if(g_selectable != NULL)
		free(g_selectable);

	g_selectable = get_selectable_positions(&g_board);
	qsort(&g_selectable->positions[0], g_selectable->count, sizeof(position_t), cmp_pos);

	help_index = 0;
	help_offset = 0;
}

static void undo()
{
	int i;

	if(undo_stack.count == 0)
		return;

	for(i = 0; i < 2; ++i) {
		board_set( &g_board, &undo_stack.positions[undo_stack.count - 1], undo_stack.chips[undo_stack.count - 1]);
		--undo_stack.count;
	}
	g_board.chip_count += 2;

	selection_pos = -1;
	rebuild_selectables();

	// find caret pos
	if(caret_pos >= g_selectable->count)
		caret_pos = g_selectable->count - 1;
}

static void clear_undo_stack()
{
	undo_stack.count = 0;
}

static void start_game(void)
{
	rebuild_selectables();
	caret_pos = 0;
	selection_pos = -1;
	game_active = 1;
	unlink(SAVED_GAME_PATH);
}

static void init_map(map_t *map)
{
	clear_undo_stack();

	generate_board(&g_board, map);
	row_count = map->row_count;
	col_count = map->col_count;

	start_game();
}

static void cell_rect(const position_t *pos, struct rect *r)
{

	const int screen_width = ScreenWidth();
	const int screen_height = ScreenHeight() - HELP_HEIGHT;
	int offset_x;
	int offset_y;

	int w, h;

	const int chip_width = IMG_WIDTH + 10;
	const int chip_height = IMG_HEIGHT + 10;
	if(chip_width * col_count * screen_height > screen_width * chip_height * row_count) {
		w = screen_width / col_count;
		h = w * chip_height / chip_width;
	}
	else {
		h = screen_height / row_count;
		w = h * chip_width / chip_height;
	}

	offset_x = (screen_width - w * col_count) / 2;
	offset_y = (screen_height - h * row_count) / 2;

	r->w = 2 * w;
	r->h = 2 * h;
	const int bw = r->w / 8;
	r->x = offset_x + pos->x * w + bw * pos->z;
	r->y = offset_y + pos->y * h + bw * pos->z;
}

static void draw_chip(const position_t *pos, chip_t chip)
{
	int i;
	struct rect r;

	cell_rect(pos, &r);

	const int bw = r.w / 8;

	/* Standard chips */
	int face = 0xffffff;
	int sides = 0xaaaaaa;
	int darkedges = 0x333333;
	int mediumedges = 0x777777;
	int lightedges = 0xffffff;
	int border = 0x000000;

	/* Blockers */
	if((chip & CHIP_CATEGORY_MASK) == CHIP_CATEGORY_BLOCK) {
		face = 0x555555;
		sides = 0x444444;
		darkedges = 0x111111;
		mediumedges = 0x222222;
		lightedges = 0x666666;
		border = 0x111111;
	}
	/* Bonus */
	else {
		if((chip & CHIP_CATEGORY2_MASK) == CHIP_CATEGORY2_BONUS) {
			if((chip & CHIP_CATEGORY_MASK) == CHIP_CATEGORY_SEASONS)
				face = 0x777777;
			else
				face = 0xbbbbbb;
		}
	}

	/* Left/top side of the chip */
	for(i = 1; i < bw; ++i) {
		DrawLine(r.x - i, r.y - i + 2, r.x - i, r.y - i + r.h - 4, sides);
		DrawLine(r.x - i + 2, r.y - i, r.x - i + r.w - 4, r.y - i, sides);
	}
	DrawPixel(r.x, r.y + 2, sides);
	DrawPixel(r.x + 2, r.y, sides);

	/* Edges of the back */
	DrawPixel(r.x + r.w - bw - 4, r.y - bw + 1, darkedges);
	DrawLine(r.x - bw + 3, r.y - bw, r.x + r.w - bw - 5, r.y - bw, darkedges);
	DrawPixel(r.x - bw + 2, r.y - bw + 1, darkedges);
	DrawPixel(r.x - bw + 1, r.y - bw + 2, darkedges);
	DrawLine(r.x - bw, r.y - bw + 3, r.x - bw, r.y + r.h - bw - 5, darkedges);
	DrawPixel(r.x - bw + 1, r.y + r.h - bw - 4, darkedges);

	/* Edges of the sides */
	DrawLine(r.x - bw + 2, r.y - bw + 2, r.x + 1, r.y + 1, lightedges);
	DrawLine(r.x - bw + 3, r.y - bw + 2, r.x + 1, r.y, mediumedges);
	DrawLine(r.x - bw + 2, r.y - bw + 3, r.x, r.y + 1, mediumedges);
	DrawLine(r.x + r.w - bw - 3, r.y - bw + 1, r.x + r.w - 5, r.y - 1, darkedges);
	DrawLine(r.x - bw + 1, r.y + r.h - bw - 3, r.x - 1, r.y + r.h - 5, darkedges);

	/* Front */
	FillArea(r.x + 2, r.y + 2, r.w - 4, r.h - 4, face);
	DrawLine(r.x + 3, r.y + 1, r.x + r.w - 4, r.y + 1, face);
	DrawLine(r.x + 3, r.y + r.h - 2, r.x + r.w - 4, r.y + r.h - 2, face);
	DrawLine(r.x + 1, r.y + 3, r.x + 1, r.y + r.h - 4, face);
	DrawLine(r.x + r.w - 2, r.y + 3, r.x + r.w - 2, r.y + r.h - 4, face);
	DrawLine(r.x + 3, r.y, r.x + r.w - 4, r.y, border);
	DrawLine(r.x + 3, r.y + r.h - 1, r.x + r.w - 4, r.y + r.h - 1, border);
	DrawLine(r.x, r.y + 3, r.x, r.y + r.h - 4, border);
	DrawLine(r.x + r.w - 1, r.y + 3, r.x + r.w - 1, r.y + r.h - 4, border);
	DrawLine(r.x + 2, r.y + 1, r.x + 1, r.y + 2, border);
	DrawLine(r.x + r.w - 3, r.y + 1, r.x + r.w - 2, r.y + 2, border);
	DrawLine(r.x + 2, r.y + r.h - 2, r.x + 1, r.y + r.h - 3, border);
	DrawLine(r.x + r.w - 3, r.y + r.h - 2, r.x + r.w - 2, r.y + r.h - 3, border);

	StretchBitmap(r.x + 5, r.y + 5, r.w - 10, r.h - 10, (ibitmap*)bitmaps[chip], 0);

	if(selection_pos >= 0 && selection_pos < g_selectable->count) {
		position_t *selection = &g_selectable->positions[selection_pos];
		if(position_equal(pos, selection))
			InvertArea(r.x + 1, r.y + 1, r.w - 2, r.h - 2);
	}
}

static int is_covered_by(const void *p1, const void *p2)
{
	const position_t *s1 = p1;
	const position_t *s2 = p2;

	if(s1->z < s2->z)
		return 1;
	if(s1->z > s2->z)
		return 0;

	if(s2->y >= s1->y + 2)
		return 0;

	if(abs(s2->y - s1->y) <= 1)
		return s2->x < s1->x;

	if(s2->y == s1->y - 2)
		return s2->x >= s1->x - 2 && s2->x < s1->x - 2;

	return 0;
}

static ifont *g_help_font = NULL;
static ifont *get_help_font(void)
{
	if(g_help_font == NULL)
		g_help_font = OpenFont(DEFAULTFONTB, 36, 1);
	return g_help_font;
}

static void main_repaint(void)
{
	int i, j, k;
	position_t *chips = (position_t *) malloc(sizeof(position_t) * g_board.chip_count);
	int chip_count = 0;

	for(i = 0; i < MAX_ROW_COUNT; ++i) {
		for(j = 0; j < MAX_COL_COUNT; ++j) {
			for(k = 0; k < MAX_HEIGHT; ++k) {
				if(chip_count >= g_board.chip_count)
					break;
				const chip_t chip = g_board.columns[i][j].chips[k];
				if(chip) {
					chips[chip_count].x = j;
					chips[chip_count].y = i;
					chips[chip_count].z = k;
					++chip_count;
				}
			}
		}
	}
	topological_sort(chips, chip_count, sizeof(position_t), is_covered_by);

	ClearScreen();

	for(i = chip_count - 1; i >= 0; --i) {
		const position_t *pos = &chips[i];
		draw_chip(pos, board_get(&g_board, pos));
	}

	/* status bar */
	{
		struct rect r;
		r.x = 0;
		r.y = ScreenHeight() - HELP_HEIGHT;
		r.w = ScreenWidth();
		r.h = HELP_HEIGHT;

		DrawLine(r.x, r.y, r.x + r.w, r.y, BLACK);
		DrawLine(r.x, r.y + 1, r.x + r.w, r.y + 1, LGRAY);
		FillArea(r.x, r.y + 2, r.w, r.h - 2, DGRAY);

		SetFont(get_help_font(), WHITE);

		r.x += 10;
		r.w -= 20;
		r.y += 8;
		r.h -= 2;

		{
			int pairs = 0;
			for(i = 0; i < g_selectable->count - 1; ++i) {
				const chip_t chip1 = board_get(&g_board, &g_selectable->positions[i]);
				for(j = i + 1; j < g_selectable->count; ++j) {
					const chip_t chip2 = board_get(&g_board, &g_selectable->positions[j]);
					if(fits(chip1, chip2))
						++pairs;
				}
			}

			char buffer[256];
			snprintf(buffer, 256, get_message(MSG_MOVES_LEFT), pairs);
			DrawTextRect(r.x, r.y, r.w, r.h, buffer, ALIGN_FIT | ALIGN_LEFT);
		}

		DrawTextRect(r.x, r.y, r.w, r.h, (char*)get_message(MSG_HELP), ALIGN_FIT | ALIGN_RIGHT);
	}
}

static int finished(void)
{
	int i, j, k;

	for(i = 0; i < MAX_ROW_COUNT; ++i) {
		for(j = 0; j < MAX_COL_COUNT; ++j) {
			for(k = 0; k < MAX_HEIGHT; ++k) {
				position_t pos;
				pos.y = i;
				pos.x = j;
				pos.z = k;
				chip_t chip = board_get(&g_board, &pos);
				if(chip != 0 && (chip & CHIP_CATEGORY_MASK) != CHIP_CATEGORY_BLOCK)
					return 0;
			}
		}
	}
	return 1;
}

static int pair_exists(board_t *board)
{
	int i, j;

	for(i = 0; i < g_selectable->count - 1; ++i) {
		const chip_t chip1 = board_get(board, &g_selectable->positions[i]);
		for(j = i + 1; j < g_selectable->count; ++j) {
			const chip_t chip2 = board_get(board, &g_selectable->positions[j]);
			if(fits(chip1, chip2))
				return 1;
		}
	}
	return 0;
}

static void select_cell(void)
{
	if(selection_pos == caret_pos) {
		struct rect r;
		cell_rect(&g_selectable->positions[selection_pos], &r);
		selection_pos = -1;
		main_repaint();
		PartialUpdate(r.x, r.y, r.w, r.h);
		return;
	}

	position_t *position1 = NULL;
	chip_t chip1 = 0;
	struct rect r1;
	if(selection_pos >= 0) {
		position1 = &g_selectable->positions[selection_pos];
		chip1 = board_get(&g_board, position1);
		cell_rect(position1, &r1);
	}
	position_t *position2 = &g_selectable->positions[caret_pos];
	chip_t chip2 = board_get(&g_board, position2);
	struct rect r2;
	cell_rect(position2, &r2);

	if(chip1 != 0 && fits(chip1, chip2)) {
		undo_stack.positions[undo_stack.count] = *position1;
		undo_stack.chips[undo_stack.count] = chip1;
		++undo_stack.count;
		undo_stack.positions[undo_stack.count] = *position2;
		undo_stack.chips[undo_stack.count] = chip2;
		++undo_stack.count;

		board_set(&g_board, position1, 0);
		board_set(&g_board, position2, 0);
		g_board.chip_count -= 2;

		selection_pos = -1;

		rebuild_selectables();
		// find caret pos
		if(caret_pos >= g_selectable->count)
			caret_pos = g_selectable->count - 1;

		static message_id finish_menu[] = {
			MSG_NEW_GAME_EASY,
			MSG_NEW_GAME_DIFFICULT,
			MSG_NEW_GAME_FOUR_BRIDGES,
			MSG_NEW_GAME_CUSTOM,
			MSG_SEPARATOR,
			MSG_EXIT,
			MSG_NONE
		};

		if(finished()) {
			game_active = 0;
			load_map(NULL);
			clear_undo_stack();
			show_popup(&background, MSG_WIN, finish_menu, menu_handler);
		}
		else if(!pair_exists(&g_board)) {
			game_active = 0;
			load_map(NULL);
			clear_undo_stack();
			show_popup(&background, MSG_LOSE, finish_menu, menu_handler);
		}
		else {
			main_repaint();

			const int bw = r1.w / 8;
			PartialUpdate(r1.x - bw, r1.y - bw, r1.w + bw, r1.h + bw);
			PartialUpdate(r2.x - bw, r2.y - bw, r2.w + bw, r2.h + bw);
			struct rect r;
			r.x = 0;
			r.y = ScreenHeight() - HELP_HEIGHT;
			r.w = ScreenWidth();
			r.h = HELP_HEIGHT;
			PartialUpdate(r.x, r.y, r.w, r.h);
		}
	}
	else {
		int prev_selection_pos = selection_pos;

		selection_pos = caret_pos;
		main_repaint();

		if(prev_selection_pos != -1)
			PartialUpdate(r1.x, r1.y, r1.w, r1.h);
		PartialUpdate(r2.x, r2.y, r2.w, r2.h);
	}
}

static void make_hint()
{
	int i, j;

	for(;;) {
		for(i = help_index; i < g_selectable->count - 1; ++i) {
			const chip_t chip1 = board_get(&g_board, &g_selectable->positions[i]);

			for(j = i + 1 + help_offset; j < g_selectable->count; ++j) {
				const chip_t chip2 = board_get(&g_board, &g_selectable->positions[j]);

				if(fits(chip1, chip2)) {
					help_index = i;
					help_offset = j - i - 1 + 1;
					selection_pos = i;
					caret_pos = j;
					return;
				}
			}
			help_offset = 0;
		}
		if(help_index == 0)
			return; /*Should never be reached, but this avoids an endless loop in that case */
		help_index = 0;
		help_offset = 0;
	}
}

static int game_handler(int type, int par1, int par2)
{
	switch(type) {
		case EVT_SHOW:
			main_repaint();
			FullUpdate();
			break;

		case EVT_KEYPRESS:
			switch(par1) {
				case IV_KEY_MENU:
				{
					static message_id game_menu[] = {
						MSG_CONTINUE,
						MSG_HINT,
						MSG_SEPARATOR,
						MSG_NEW_GAME_EASY,
						MSG_NEW_GAME_DIFFICULT,
						MSG_NEW_GAME_FOUR_BRIDGES,
						MSG_NEW_GAME_CUSTOM,
						MSG_SEPARATOR,
						MSG_EXIT,
						MSG_NONE
					};

					static message_id game_menu_with_undo[] = {
						MSG_CONTINUE,
						MSG_HINT,
						MSG_UNDO,
						MSG_SEPARATOR,
						MSG_NEW_GAME_EASY,
						MSG_NEW_GAME_DIFFICULT,
						MSG_NEW_GAME_FOUR_BRIDGES,
						MSG_NEW_GAME_CUSTOM,
						MSG_SEPARATOR,
						MSG_EXIT,
						MSG_NONE
					};
					if(undo_stack.count != 0)
						show_popup(NULL, MSG_NONE, game_menu_with_undo, menu_handler);
					else
						show_popup(NULL, MSG_NONE, game_menu, menu_handler);
					return 1;
				}
				case IV_KEY_PREV:
					make_hint();
					main_repaint();
					FullUpdate();
					return 1;
				case IV_KEY_NEXT:
					undo();
					main_repaint();
					FullUpdate();
					return 1;
			}
			break;

		case EVT_POINTERDOWN:
		{
			int i;
			int rx, ry;

			point_change_orientation(par1, par2, GetOrientation(), &rx, &ry);

			for(i = 0; i < g_selectable->count; ++i) {
				struct rect r;
				cell_rect(&g_selectable->positions[i], &r);
				if(point_in_rect(rx, ry, &r)) {
					int prev_caret_pos = caret_pos;
					struct rect prev_r;

					caret_pos = i;

					main_repaint();
					cell_rect(&g_selectable->positions[prev_caret_pos], &prev_r);
					PartialUpdate(prev_r.x, prev_r.y, prev_r.w, prev_r.h);

					select_cell();
					break;
				}
			}
			break;
		}
	}
	return 0;
}

static message_id main_menu_wo_load[] = {
	MSG_NEW_GAME_EASY,
	MSG_NEW_GAME_DIFFICULT,
	MSG_NEW_GAME_FOUR_BRIDGES,
	MSG_NEW_GAME_CUSTOM,
	MSG_SEPARATOR,
	MSG_TOGGLE_LANGUAGE,
	MSG_CHANGE_ORIENTATION,
	MSG_SEPARATOR,
	MSG_EXIT,
	MSG_NONE
};

static message_id main_menu_w_load[] = {
	MSG_NEW_GAME_EASY,
	MSG_NEW_GAME_DIFFICULT,
	MSG_NEW_GAME_FOUR_BRIDGES,
	MSG_NEW_GAME_CUSTOM,
	MSG_SEPARATOR,
	MSG_LOAD,
	MSG_SEPARATOR,
	MSG_TOGGLE_LANGUAGE,
	MSG_CHANGE_ORIENTATION,
	MSG_SEPARATOR,
	MSG_EXIT,
	MSG_NONE
};

static message_id *main_menu;

static void menu_handler(int index)
{
	switch(index)
	{
		case MSG_CONTINUE:
			SetEventHandler(game_handler);
			break;

		case MSG_HINT:
			make_hint();
			SetEventHandler(game_handler);
			break;

		case MSG_UNDO:
			undo();
			SetEventHandler(game_handler);
			break;

		case MSG_NEW_GAME_EASY:
			init_map(&standard_map);
			SetEventHandler(game_handler);
			break;

		case MSG_NEW_GAME_DIFFICULT:
			init_map(&difficult_map);
			SetEventHandler(game_handler);
			break;

		case MSG_NEW_GAME_FOUR_BRIDGES:
			init_map(&four_bridges_map);
			SetEventHandler(game_handler);
			break;

		case MSG_NEW_GAME_CUSTOM:
			if(map_list != NULL)
				show_popup_list(&background, MSG_NONE, map_list, load_map_handler);
			break;

		case MSG_LOAD:
			if(load_game()) {
				start_game();
				SetEventHandler(game_handler);
			}
			break;

		case MSG_TOGGLE_LANGUAGE:
			if(current_language == ENGLISH)
				current_language = RUSSIAN;
			else if(current_language == RUSSIAN)
				current_language = GERMAN;
			else
				current_language = ENGLISH;
			show_popup(&background, MSG_NONE, main_menu, menu_handler);
			break;

		case MSG_CHANGE_ORIENTATION:
			if(orientation == ROTATE270)
				orientation = ROTATE90;
			else
				orientation = ROTATE270;
			SetOrientation(orientation);
			ClearScreen();
			StretchBitmap(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (ibitmap*)&background, 0);
			FullUpdate();
			show_popup(&background, MSG_NONE, main_menu, menu_handler);
			break;

		case MSG_EXIT:
			write_state();
			if(game_active)
				save_game();
			CloseApp();
			break;
	}
}

static void load_map_handler(int index)
{
	if(index >= 0 && index <= map_list_size) {
		map_t *map = load_map(map_list[index]);
		if(map != NULL) {
			init_map(map);
			SetEventHandler(game_handler);
		}
		else {
			show_popup_list(&background, MSG_LOADING_FAILED, map_list, load_map_handler);
		}
	}
	else {
		show_popup(&background, MSG_NONE, main_menu, menu_handler);
	}
}

static int main_handler(int type, int par1, int par2)
{
	switch(type) {
		case EVT_INIT:
			SetPanelType(PANEL_DISABLED);
			srand(time(NULL));
			bitmaps_init();
			read_state();
			SetOrientation(orientation);
			if(!access(SAVED_GAME_PATH, R_OK))
				main_menu = main_menu_w_load;
			else
				main_menu = main_menu_wo_load;
			scan_maps(MAPS_DIR);

			show_popup(&background, MSG_NONE, main_menu, menu_handler);
			break;

		case EVT_EXIT:
			if(game_active)
				save_game();
			break;
	}
	return 0;
}

static void read_state(void)
{
	FILE *f = fopen(STATEPATH "/pb-mahjong", "r");
	if(!f)
		return;

	regex_t reg;
	regcomp(&reg, "^([a-z]+)([ \t]*)=([ \t]*)([^ \t\r\n]+)", REG_EXTENDED);

	char line[128];
	regmatch_t pmatch[16];

	while(fgets(line, sizeof(line), f)) {
		if(regexec(&reg, line, 16, pmatch, 0))
			continue;

		const char *key = &line[pmatch[1].rm_so];
		line[pmatch[1].rm_eo] = '\0';

		const char *value = &line[pmatch[4].rm_so];
		line[pmatch[4].rm_eo] = '\0';

		if(!strcmp(key, "language")) {
			if(!strcmp(value, "en"))
				current_language = ENGLISH;
			else if(!strcmp(value, "ru"))
				current_language = RUSSIAN;
			else if(!strcmp(value, "de"))
				current_language = GERMAN;
		}
		else if(!strcmp(key, "orientation")) {
			if(!strcmp(value, "90"))
				orientation = ROTATE90;
			else if(!strcmp(value, "270"))
				orientation = ROTATE270;
		}
	}
	fclose(f);

	regfree(&reg);
}

static void write_state(void)
{
	FILE *f = fopen(STATEPATH "/pb-mahjong", "w");
	if(!f)
		return;

	if(current_language == ENGLISH)
		fprintf(f, "language = en\n");
	else if(current_language == RUSSIAN)
		fprintf(f, "language = ru\n");
	else if(current_language == GERMAN)
		fprintf(f, "language = de\n");

	if(orientation == ROTATE90)
		fprintf(f, "orientation = 90\n");
	else if(orientation == ROTATE270)
		fprintf(f, "orientation = 270\n");

	fclose(f);
}

static int load_game(void)
{
	int i, j, k;

	FILE *f = fopen(SAVED_GAME_PATH, "r");
	if(!f)
		return 0;

	fscanf(f, "%d %d\n", &row_count, &col_count);

	for(i = 0; i < MAX_ROW_COUNT; ++i) {
		for(j = 0; j < MAX_COL_COUNT; ++j) {
			for(k = 0; k < MAX_HEIGHT; ++k) {
				int chip;
				position_t pos;
				pos.y = i;
				pos.x = j;
				pos.z = k;

				fscanf(f, "%d\n", &chip);

				board_set(&g_board, &pos, chip);
				if(chip)
					++g_board.chip_count;
			}
		}
	}

	fscanf(f, "%d\n", &undo_stack.count);
	for(i = 0; i < undo_stack.count; ++i) {
		int chip, x, y, z;
		fscanf(
			f, "%d %d %d %d\n",
			&y,
			&x,
			&z,
			&chip);
		undo_stack.positions[i].y = (unsigned char) y;
		undo_stack.positions[i].x = (unsigned char) x;
		undo_stack.positions[i].z = (unsigned char) z;
		undo_stack.chips[i] = (chip_t) chip;
	}

	fclose(f);

	return 1;
}

static void save_game(void)
{
	int i, j, k;

	FILE *f = fopen(SAVED_GAME_PATH, "w");
	if(!f)
		return;

	fprintf(f, "%d %d\n", row_count, col_count);

	for(i = 0; i < MAX_ROW_COUNT; ++i) {
		for(j = 0; j < MAX_COL_COUNT; ++j) {
			for(k = 0; k < MAX_HEIGHT; ++k) {
				chip_t ch;
				position_t pos;
				pos.y = i;
				pos.x = j;
				pos.z = k;
				ch = board_get(&g_board, &pos);

				fprintf(f, "%d\n", ch);
			}
		}
	}

	fprintf(f, "%d\n", undo_stack.count);
	for(i = 0; i < undo_stack.count; ++i) {
		fprintf(
			f, "%d %d %d %d\n",
			undo_stack.positions[i].y,
			undo_stack.positions[i].x,
			undo_stack.positions[i].z,
			undo_stack.chips[i]);
	}

	fclose(f);
}

static int is_map(const struct dirent *file)
{
	int len = strlen(file->d_name) - strlen(MAPS_EXT);
	return len > 0 && strcmp(file->d_name + len, MAPS_EXT) == 0;
}

static void scan_maps(const char *directory)
{
	struct dirent **files;
	map_list_size = scandir(directory, &files, &is_map, alphasort);
	
	if(map_list_size >= 0) {
		map_list = (char **) malloc(sizeof(char *) * (map_list_size + 1));
		int index = 0;
		for(; index < map_list_size; ++index) {
			int len = strlen(files[index]->d_name) - strlen(MAPS_EXT);
			if(len < 0)
				len = 0; /* Just to be sure */
			map_list[index] = (char *) malloc(len + 1);
			strncpy(map_list[index], files[index]->d_name, len);
			map_list[index][len] = 0;
			free(files[index]);
		}
		map_list[index] = NULL;
		free(files);
	}
	else {
		map_list = NULL;
	}
}

static map_t *load_map(const char *name)
{
	static map_t *loaded_map;
	static char *loaded_name;

	if(loaded_name != NULL) {
		free(loaded_name);
		loaded_name = NULL;
	}
	if(name != NULL) {
		if(loaded_map == NULL)
			loaded_map = (map_t *) malloc(sizeof(map_t));
		memset(loaded_map, 0, sizeof(map_t));

		char path[256];
		sprintf(path, "%s/%s.map", MAPS_DIR, name);
		FILE *f = fopen(path, "r");
		if(!f)
			return NULL;

		/* Board size */
		int col_count = 32;
		int row_count = 18;
		if(
			fscanf(f, "%d %d\n", &col_count, &row_count) == EOF ||
			col_count < 0 || col_count >= MAX_COL_COUNT ||
			row_count < 0 || row_count >= MAX_ROW_COUNT)
			return NULL;
		loaded_map->col_count = (unsigned char) col_count;
		loaded_map->row_count = (unsigned char) row_count;

		/* Chip positions */
		unsigned int chip;
		int x = 0, y = 0, z = 0;
		for(chip = 0; chip < CHIP_COUNT; ++chip) {
			if(
				fscanf(f, "%d %d %d\n", &x, &y, &z) == EOF ||
				x < 0 || x >= col_count ||
				y < 0 || y >= row_count ||
				z < 0 || z >= MAX_HEIGHT)
				return NULL;

			loaded_map->chip[chip].x = (unsigned char) x;
			loaded_map->chip[chip].y = (unsigned char) y;
			loaded_map->chip[chip].z = (unsigned char) z;
			x = 0;
			y = 0;
			z = 0;
		}

		/* All following positions are blocker positions */
		while(fscanf(f, "%d %d %d\n", &x, &y, &z) != EOF) {
			if(
				x < 0 || x >= col_count ||
				y < 0 || y >= row_count ||
				z < 0 || z >= MAX_HEIGHT)
				return NULL;

			const unsigned int block = loaded_map->block_count;
			++loaded_map->block_count;
			if(loaded_map->block == NULL)
				loaded_map->block = (position_t *) malloc(sizeof(position_t));
			else
				loaded_map->block = (position_t *) realloc(loaded_map->block, sizeof(position_t) * loaded_map->block_count);

			loaded_map->block[block].x = (unsigned char) x;
			loaded_map->block[block].y = (unsigned char) y;
			loaded_map->block[block].z = (unsigned char) z;
			x = 0;
			y = 0;
			z = 0;
		}

		fclose(f);

		loaded_name = (char *) malloc(strlen(name) + 1);
		strcpy(loaded_name, name);
		loaded_map->name = loaded_name;
	}
	else {
		if(loaded_map != NULL) {
			free(loaded_map);
			loaded_map = NULL;
		}
	}

	return loaded_map;
}

int main(int argc, char **argv)
{
	InkViewMain(main_handler);
	return 0;
}
