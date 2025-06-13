// TODO coredumps

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
#define LK_ENTER 10
#define MY_KEY_ENTER 13
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
  uint32_t cap;         // maximum capacity of gapbuffer, should be increased when needed
  uint32_t size;        // size of written chars (frontbuffer + backbuffer)
  uint32_t front;       // end of frontbuffer
  uint32_t point;       // relative position of cursor inside the buffer (get absolute pos with gb_pos)
  uint32_t line_start;  // position of next \n (or pos 0) to the left of cursor
  uint32_t line_end;    //         ""          (or pos cap) to the right
  uint16_t line_width;  // width of current line
  uint16_t lin, col;    // number of lines and cols the cursor is at
  uint16_t maxlines;    // number of maxlines of current buffer
} GapBuffer;


void gb_init(GapBuffer *g, uint32_t init_cap) {
  g->cap = init_cap;
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->line_width = 0;
  g->maxlines = 1;
}

uint32_t gb_gap(GapBuffer *g) {
  return g->cap - g->size;
}

uint32_t gb_pos(GapBuffer *g, u32 point) {
  return point + (point >= g->front) * gb_gap(g);
}

u32 gb_pos_offset(GapBuffer *g, i32 offset) {
  if (offset < 0 && g->point < -offset)
    return 0;
  if (offset > 0 && g->point + offset > g->size)
    return 0;
  u32 n = g->point + offset;
  return n + (n >= g->front) * gb_gap(g);
}

char gb_get_current(GapBuffer *g) {
  u32 pos = gb_pos(g, g->point);
  MY_ASSERT(pos < g->cap, "g->cap: %d", g->cap);
  return g->buf[pos];
}

char gb_get_char(GapBuffer *g, u32 point) {
	u32 pos = gb_pos(g, point);
	return g->buf[pos];
}

char gb_get_offset(GapBuffer *g, i32 offset) {
  u32 pos = gb_pos_offset(g, offset);
  return g->buf[pos];
}

void gb_count_maxlines(GapBuffer *g) {
  u32 old_point = g->point;
  g->maxlines = 1;
  for (g->point = 0; g->point < g->size; g->point++)
    if (gb_get_current(g) == LK_ENTER)
      g->maxlines += 1;
  g->point = old_point;
}

int gb_jump(GapBuffer *g) {
  // MOVE DATA TO END
  if (g->point < g->front) {
    size_t n = g->front - g->point;
    memmove(g->buf + g->point + gb_gap(g), g->buf + g->point, n);
    g->front = g->point;
  }
  // MOVE DATA TO FRONT 
  else if (g->point > g->front) {
    size_t n = g->point - g->front;
    memmove(g->buf + g->front, g->buf + gb_gap(g) + g->front, n);
    g->front = g->point;
  }
}

void gb_refresh_line_width(GapBuffer *g) {
  uint32_t old_point = g->point;
  uint32_t point_right = 0;
  uint32_t point_left = 0;
  
  // MOVE RIGHT
  for (; g->point < g->size; g->point++) {
    if (gb_get_current(g) == 10) {
      point_right = g->point;
      break;
    }
  }

  // MOVE LEFT
  for (g->point = point_right - 1; g->point > 0; g->point--) {
    if (gb_get_current(g) == 10) {
      point_left = g->point;
      break;
    }
  }

  g->line_start = point_left + (point_left == 0 ? 0 : 1);
  g->line_end = point_right;    
  g->line_width = g->line_end - g->line_start + 1;
  g->point = old_point;
}


u16 gb_width_left(GapBuffer *g) {
  for (int i = 0; i < 10000; i++) {
    if (g->point - i == 0)
      return i;
		if (i > 0 && gb_get_char(g, g->point - i) == LK_ENTER)
      return i - 1;
  }
	die("should never be reached (left)");
}

u16 gb_width_right(GapBuffer *g) {
  for (int i = 0; i < 10000; i++) {
    if (g->point + i == g->size - 1)
      return i;
    if (gb_get_offset(g, i) == LK_ENTER)
      return i;
  }
  die("should never be reached (right)");
}

void gb_enter(GapBuffer *g) {
  gb_jump(g);
  g->buf[g->front] = '\n';
  g->size++;
  g->front++;
  g->point++;
  g->lin += 1;
  g->col = 0;
  g->maxlines++;
  //gb_refresh_line_width(g);
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
    g->lin--;
    g->maxlines--;
    g->point--;
    g->col = gb_width_left(g);
  }
  g->size--;
  g->front--;
  //gb_refresh_line_width(g);
  return true;
}

bool gb_move_right(GapBuffer *g) {
  if (g->point >= g->size - 1) {
    return false;
  }
  if (gb_get_current(g) == 10) {
    g->point++;
    g->lin++;
    g->col = 0;
    //gb_refresh_line_width(g);
  }
  else {
    g->point++;
    g->col++;
  }
  return true;
}

bool gb_move_left(GapBuffer *g) {
  if (g->point <= 0) {
    return false;
  }
  g->point--;
  if (gb_get_current(g) == 10) {
    gb_refresh_line_width(g);
    g->lin--;
    g->col = g->line_width - 1;
  }
  else {
    g->col--;
  }
  return true;
}

bool gb_home(GapBuffer *g) {
  if (g->col == 0 || g->point <= 0) {
     return false;
   }

  g->point -= gb_width_left(g);
  g->col = 0;
  return true;
}

bool gb_end(GapBuffer *g) {
  if (gb_get_current(g) == 10 || g->point >= g->size -1) {
    return false;
  }

  g->point += gb_width_right(g);
  g->col = g->line_width - 1;
  return true;
}

bool gb_move_up(GapBuffer *g) {
  if (g->lin == 0) {
    return false;
  }
  g->point = g->line_start - 1;
  gb_refresh_line_width(g);
  g->lin--;
  g->col = MIN(g->col, g->line_width - 1);
  g->point -= (g->col < g->line_width - 1) ? 
  g->line_width - g->col - 1 : 0; 
  return true;
}

bool gb_move_down(GapBuffer *g) {
  if (g->lin >= g->maxlines - 2) {
    return false;
  }
  g->point = g->line_end + 1;
  gb_refresh_line_width(g);
  g->lin++;
  g->col = MIN(g->col, g->line_width - 1);
  g->point += g->col;
  return true;
}

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
  g->line_width = 0;
  g->lin = 0;
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
  gb_refresh_line_width(g);
  fclose(file);
  return 0;
}
