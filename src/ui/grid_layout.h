#ifndef GRID_LAYOUT_H
#define GRID_LAYOUT_H

/*
 * Pure geometry of the virtual emoji grid (no GTK), so it can be unit-tested.
 * Hit i sits at row i / GRID_COLS and uses pool button i % pool_size.
 */
#define GRID_COLS 8
#define GRID_ROW_GAP 5
#define GRID_MIN_CELL 40
#define GRID_SCROLLBAR_ROOM 14 /* room for the vertical scrollbar */
/* Narrowest viewport where 8 columns of GRID_MIN_CELL buttons fit. The
 * picker window is fixed at 420px wide, well above this. */
#define GRID_MIN_VIEWPORT_W (GRID_COLS * GRID_MIN_CELL + GRID_SCROLLBAR_ROOM)

/* Column pitch from the *scrolled window* width. Never the grid's own width:
 * that shrinks when the scrollbar shows, and re-placing cells for it toggled
 * the scrollbar again in a ~60 Hz resize loop. */
int grid_cell_width(int viewport_w);
/* Row pitch from the emoji button's preferred height. */
int grid_cell_height(int button_h);
/* Pool buttons needed so every row touching a viewport_h window is covered. */
int grid_pool_size(int viewport_h, int cell_h);
int grid_rows(int n_hits);
int grid_first_row(double scroll_y, int cell_h);
/* First hit index the pool covers when scrolled to scroll_y. */
int grid_first_hit(double scroll_y, int cell_h);
int grid_slot(int hit, int pool_size);
/* Top-left of hit's button, centered in its cell. */
void grid_cell_xy(int hit, int cell_w, int cell_h, int *x, int *y);

#endif
