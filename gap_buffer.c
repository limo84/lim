#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "files.h"

#define ASSERT(c) assert(c)
#define MY_ASSERT(c, s, p) if (!(c)) { printf(s, p); exit(1); } 


#define CTRL(c) ((c) & 037)
#define STR_Q 17
#define LK_NEWLINE 10
#define LK_ENTER 13
#define LK_UP 65
#define LK_DOWN 258
#define LK_RIGHT 67
#define LK_LEFT 68

#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// ********************* #MISC *********************************/

void die(const char *format, ...) {
  va_list args;
  va_start(args, format);
    vprintf(format, args);
  va_end(args);
 
  exit(0);
}

/******************** #GAPBUFFER *******************************/

#define INIT_CAP 100000

typedef struct {
  char *buf;
  u32 cap;       // maximum capacity of gapbuffer, should be increased when needed
  u32 size;      // size of written chars (frontbuffer + backbuffer)
  u32 front;     // end of frontbuffer
  u32 point;     // relative position of cursor inside the buffer (get absolute pos with gb_pos)
  u16 line, col; // number of lines and cols the cursor is at
  u16 maxlines;  // number of maxlines of current buffer
} GapBuffer;


void gb_init(GapBuffer *g, u32 init_cap) {
  g->cap = init_cap;
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->maxlines = 1;
}

u32 gb_gap(GapBuffer *g) {
  return g->cap - g->size;
}

u32 gb_pos(GapBuffer *g, u32 point) {
  return point + (point >= g->front) * gb_gap(g);
}

char gb_get_char(GapBuffer *g, u32 point) {
  return g->buf[gb_pos(g, point)];
}

char gb_get_current(GapBuffer *g) {
  return gb_get_char(g, g->point);;
}

void gb_count_maxlines(GapBuffer *g) {
  g->maxlines = 1;
  for (int i = 0; i < g->size; i++)
    if (gb_get_char(g, i) == LK_NEWLINE)
      g->maxlines += 1;
}

void gb_jump(GapBuffer *g) {
  // MOVE DATA TO END
  if (g->point < g->front) {
    u32 n = g->front - g->point;
    memmove(g->buf + g->point + gb_gap(g), g->buf + g->point, n);
    g->front = g->point;
  }
  // MOVE DATA TO FRONT 
  else if (g->point > g->front) {
    u32 n = g->point - g->front;
    memmove(g->buf + g->front, g->buf + gb_gap(g) + g->front, n);
    g->front = g->point;
  }
}

u16 gb_width_left(GapBuffer *g) {
  for (int i = 0; i < 10000; i++) {
    if (g->point - i == 0)
      return i;
    if (gb_get_char(g, g->point - i - 1) == LK_NEWLINE)
      return i;
  }
  die("should never be reached (left)");
}

u16 gb_width_right(GapBuffer *g) {
  for (int i = 0; i < 10000; i++) {
    if (g->point + i == g->size - 1)
      return i;
    if (gb_get_char(g, g->point + i) == LK_NEWLINE)
      return i;
  }
  die("should never be reached (right)");
}

// --------------- MOVE -----------------------------

bool gb_move_left(GapBuffer *g) {
  if (g->point == 0) {
    return false;
  }
  g->point--;
  g->col--;
  if (gb_get_current(g) == LK_NEWLINE) {
    g->line--;
    g->col = gb_width_left(g);
  }
  return true;
}

bool gb_move_right(GapBuffer *g) {
  if (g->point == g->size - 1) {
    return false;
  }
  if (gb_get_current(g) == LK_NEWLINE) {
    g->point++;
    g->line++;
    g->col = 0;
  }
  else {
    g->point++;
    g->col++;
  }
  return true;
}

bool gb_move_down(GapBuffer *g) {
  if (g->line == g->maxlines - 2) {
    return false;
  }
  // move to start of next line
  u16 offset = gb_width_left(g);
  g->point += gb_width_right(g) + 1;
  // move further right to offset
  u16 new_width = gb_width_right(g);
  offset = MIN(offset, new_width);
  g->point += offset;
  // adjust line and col
  g->line += 1;
  g->col = offset;
  return true;
}

bool gb_move_up(GapBuffer *g) {
  if (g->line == 0) {
    return false;
  }
  // move to end of previous line
  u16 offset = gb_width_left(g);
  g->point -= (offset + 1);
  // move to start of line
  u16 new_width = gb_width_left(g);
  g->point -= new_width;
  // move to offset
  offset = MIN(offset, new_width);
  g->point += offset;
  //adjust line and col
  g->line -= 1;
  g->col = offset;
  return true;
}

// TODO refactor from here

void gb_insert_char(GapBuffer *g, u8 c) {
  gb_jump(g);
  g->buf[g->front] = c;
  g->size++;
  g->front++;
  g->point++;
}

void gb_enter(GapBuffer *g) {
  gb_insert_char(g, LK_NEWLINE);
  g->line += 1;
  g->col = 0;
  g->maxlines++;
}

bool gb_backspace(GapBuffer *g) {
  
  if (g->point == 0) {
    return false;
  }

  gb_jump(g);
  if (g->col > 0) {
    g->col--;
    g->point--;
  } 
  else {
    g->line--;
    g->maxlines--;
    g->point--;
    g->col = gb_width_left(g);
  }
  g->size--;
  g->front--;
  return true;
}

void gb_home(GapBuffer *g) {
  g->point -= gb_width_left(g);
  g->col = 0;
}

void gb_end(GapBuffer *g) {
  u16 width_right = gb_width_right(g);
  g->col += width_right;
  g->point += width_right;
}

// TODO
// void gb_trim_trailing()  <- trim trailing whitespaces

// TODO refactor and write in 2 steps (front, back)
void gb_write_to_file(GapBuffer *g, char* filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    die("Can't write to file: %s", filename);
  }
  u32 old_point = g->point;
  for (g->point = 0; g->point < g->size; g->point++) {
    char c = gb_get_current(g);
    putc(c, file);
  }
  g->point = old_point;
  fclose(file);
}

int gb_read_file(GapBuffer *g, char* filename) {

  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->line = 0;
  g->col = 0;
  g->maxlines = 1;
 
  FILE *file = fopen(filename, "r");
  if (!file) die("File not found\n");
  if (fseek(file, 0, SEEK_END) != 0)
    die("fseek SEEK_END");
 
  g->size = ftell(file);
  fseek(file, 0, SEEK_SET);

  fread(g->buf + g->cap - g->size, g->size, 1, file);
  gb_count_maxlines(g);
  fclose(file);
  return 0;
}
