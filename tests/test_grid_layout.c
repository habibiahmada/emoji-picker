#include "ui/grid_layout.h"
#include "test_harness.h"

#include <string.h>

/* Picker geometry as measured on Lubuntu: 400px viewport, 232px tall grid,
 * 13px classic scrollbar, 40px buttons (+5 gap). */
#define VIEW_W 400
#define VIEW_H 232
#define SCROLLBAR_W 13
#define ALL_HITS 1914

static void test_cell_height_has_floor_and_gap(void) {
    TEST_ASSERT(grid_cell_height(40) == 45);
    TEST_ASSERT(grid_cell_height(10) == GRID_MIN_CELL + GRID_ROW_GAP);
    TEST_ASSERT(grid_cell_height(48) == 53);
}

/* With the scrollbar shown the grid is VIEW_W - SCROLLBAR_W wide; every
 * column must still fit, or the grid would ask for width and toggle the
 * scrollbar (the 60 Hz resize loop). */
static void test_columns_fit_beside_scrollbar(void) {
    TEST_ASSERT(VIEW_W >= GRID_MIN_VIEWPORT_W);
    for (int vw = GRID_MIN_VIEWPORT_W; vw <= 900; vw++) {
        int cw = grid_cell_width(vw);
        int ch = grid_cell_height(40);
        int x = 0, y = 0;
        grid_cell_xy(GRID_COLS - 1, cw, ch, &x, &y);
        int right = x + (ch - GRID_ROW_GAP);
        TEST_ASSERT(right <= vw - SCROLLBAR_W);
        if (right > vw - SCROLLBAR_W) {
            fprintf(stderr, "  viewport %d: last column ends at %d\n", vw, right);
            break;
        }
    }
}

static void test_cell_xy_centers_button(void) {
    int cw = grid_cell_width(VIEW_W), ch = grid_cell_height(40);
    for (int i = 0; i < 3 * GRID_COLS; i++) {
        int x = 0, y = 0;
        grid_cell_xy(i, cw, ch, &x, &y);
        int col = i % GRID_COLS, row = i / GRID_COLS;
        TEST_ASSERT(x >= col * cw && x + (ch - GRID_ROW_GAP) <= (col + 1) * cw);
        TEST_ASSERT(y == row * ch);
    }
}

static void test_rows_and_first_row_edges(void) {
    TEST_ASSERT(grid_rows(0) == 0);
    TEST_ASSERT(grid_rows(1) == 1);
    TEST_ASSERT(grid_rows(GRID_COLS) == 1);
    TEST_ASSERT(grid_rows(GRID_COLS + 1) == 2);
    TEST_ASSERT(grid_first_row(-5.0, 45) == 0);
    TEST_ASSERT(grid_first_row(44.9, 45) == 0);
    TEST_ASSERT(grid_first_row(45.0, 45) == 1);
    TEST_ASSERT(grid_first_row(100.0, 0) == 0);
    TEST_ASSERT(grid_pool_size(VIEW_H, 0) == 0);
}

/* At every pixel offset, every hit that is even partly visible must be one
 * the pool is rendering — otherwise a blank cell shows while scrolling. */
static void test_pool_covers_viewport_at_every_offset(void) {
    for (int ch = 41; ch <= 60; ch++) {
        int pool = grid_pool_size(VIEW_H, ch);
        TEST_ASSERT(pool % GRID_COLS == 0);
        int max_scroll = grid_rows(ALL_HITS) * ch - VIEW_H;
        for (int s = 0; s <= max_scroll; s++) {
            int start = grid_first_hit(s, ch);
            int top_row = s / ch, bottom_row = (s + VIEW_H - 1) / ch;
            int first_vis = top_row * GRID_COLS;
            int last_vis = (bottom_row + 1) * GRID_COLS - 1;
            if (last_vis >= ALL_HITS)
                last_vis = ALL_HITS - 1;
            if (first_vis < start || last_vis >= start + pool) {
                fprintf(stderr, "  cell_h %d scroll %d: visible %d..%d, pool %d..%d\n",
                        ch, s, first_vis, last_vis, start, start + pool - 1);
                TEST_ASSERT(0);
                return;
            }
        }
    }
}

/* Two hits rendered together never share a button. */
static void test_slots_unique_within_pool(void) {
    int pool = grid_pool_size(VIEW_H, 45);
    for (int start = 0; start < ALL_HITS; start += GRID_COLS) {
        char used[256];
        memset(used, 0, sizeof(used));
        for (int i = start; i < start + pool; i++) {
            int slot = grid_slot(i, pool);
            TEST_ASSERT(slot >= 0 && slot < pool && !used[slot]);
            used[slot] = 1;
        }
    }
}

/* Ring buffer: scrolling one row relabels exactly one row of buttons. */
static void test_one_row_scroll_relabels_one_row(void) {
    int ch = 45, pool = grid_pool_size(VIEW_H, ch);
    int slot_hit[256];
    for (int k = 0; k < pool; k++)
        slot_hit[k] = -1;
    int prev_first = -1;
    for (int s = 0; s <= 40 * ch; s += 7) {
        int first = grid_first_row(s, ch);
        int start = grid_first_hit(s, ch);
        int relabeled = 0;
        for (int i = start; i < start + pool; i++) {
            int slot = grid_slot(i, pool);
            if (slot_hit[slot] != i) {
                slot_hit[slot] = i;
                relabeled++;
            }
        }
        if (prev_first >= 0 && first == prev_first + 1)
            TEST_ASSERT(relabeled == GRID_COLS);
        if (prev_first >= 0 && first == prev_first)
            TEST_ASSERT(relabeled == 0);
        prev_first = first;
    }
}

int main(void) {
    TEST_RUN(test_cell_height_has_floor_and_gap);
    TEST_RUN(test_columns_fit_beside_scrollbar);
    TEST_RUN(test_cell_xy_centers_button);
    TEST_RUN(test_rows_and_first_row_edges);
    TEST_RUN(test_pool_covers_viewport_at_every_offset);
    TEST_RUN(test_slots_unique_within_pool);
    TEST_RUN(test_one_row_scroll_relabels_one_row);
    return test_finish();
}
