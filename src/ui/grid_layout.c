#include "ui/grid_layout.h"

int grid_cell_width(int viewport_w) {
    int w = (viewport_w - GRID_SCROLLBAR_ROOM) / GRID_COLS;
    return w > 0 ? w : 1;
}

int grid_cell_height(int button_h) {
    return (button_h > GRID_MIN_CELL ? button_h : GRID_MIN_CELL) + GRID_ROW_GAP;
}

int grid_pool_size(int viewport_h, int cell_h) {
    if (cell_h <= 0)
        return 0;
    /* Rows fully inside, plus one partly scrolled in at each edge. */
    return ((viewport_h + cell_h - 1) / cell_h + 1) * GRID_COLS;
}

int grid_rows(int n_hits) {
    return n_hits > 0 ? (n_hits + GRID_COLS - 1) / GRID_COLS : 0;
}

int grid_first_row(double scroll_y, int cell_h) {
    if (cell_h <= 0 || scroll_y <= 0)
        return 0;
    return (int)(scroll_y / cell_h);
}

int grid_first_hit(double scroll_y, int cell_h) {
    return grid_first_row(scroll_y, cell_h) * GRID_COLS;
}

int grid_slot(int hit, int pool_size) {
    return pool_size > 0 ? hit % pool_size : 0;
}

void grid_cell_xy(int hit, int cell_w, int cell_h, int *x, int *y) {
    int button_w = cell_h - GRID_ROW_GAP;
    *x = (hit % GRID_COLS) * cell_w + (cell_w - button_w) / 2;
    *y = (hit / GRID_COLS) * cell_h;
}
