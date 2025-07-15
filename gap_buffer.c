#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#if 1 // for later implementation of editors without ncurses
#include <ncurses.h>
#endif

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

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? (-a) : (a))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// ********************* #MISC *********************************/

#define die(format, ...) __die(__FILE__, __LINE__, format __VA_OPT__(,) __VA_ARGS__)

void __die(const char* file, int line, const char *format, ...) {
  endwin();
  printf("\n\033[91m[%s: %d]\033[39m ", file, line);
  va_list args;
  va_start(args, format);
    vprintf(format, args);
  va_end(args);
  printf("\n\n");
  exit(0);
}

void signal_handler(int sig) {
  endwin();
  fprintf(stderr, "\n\033[91m[%s: %d] Signal %d caught. Exiting.\n\n\033[39m",
      __FILE__, __LINE__, sig);
  signal(sig, SIG_DFL);
  raise(sig);
}

void setup_signals(void) {
  int signals[] = { SIGINT, SIGTERM, SIGSEGV, SIGABRT, SIGHUP };
  for (size_t i = 0; i < sizeof(signals)/sizeof(signals[0]); i++) {
    signal(signals[i], signal_handler);
  }
}
/******************** #GAPBUFFER *******************************/

#define INIT_CAP 1000

typedef struct {
  char *buf;
  u32 cap;       // maximum capacity of gapbuffer, should be increased when needed
  u32 size;      // size of written chars (frontbuffer + backbuffer)
  u32 front;     // end of frontbuffer
  u32 point;     // relative position of cursor inside the buffer (get absolute pos with gb_pos)
  u16 line, col; // number of lines and cols the cursor is at
  u16 maxlines;  // number of maxlines of current buffer
  u32 sel_start;
  u32 sel_end;
} GapBuffer;


void gb_init(GapBuffer *g, u32 init_cap) {
  g->cap = init_cap;
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->maxlines = 1;
  g->sel_start = UINT32_MAX;
  g->sel_end = UINT32_MAX;
}

u32 gb_gap(GapBuffer *g) {
  return g->cap - g->size;
}

u32 gb_back_start(GapBuffer *g) {
  return g->front + gb_gap(g);
}

u32 gb_back_len(GapBuffer *g) {
  return g->size - gb_back_start(g);
}

u32 gb_pos(GapBuffer *g, u32 point) {
  return point + (point >= g->front) * gb_gap(g);
}

char gb_get_char(GapBuffer *g, u32 point) {
  return g->buf[gb_pos(g, point)];
}

char gb_get_current(GapBuffer *g) {
  return gb_get_char(g, g->point);
}

void gb_check_increase(GapBuffer *g, u32 amount) {
  
  if (g->size + amount < g->cap)
    return;

  u32 old_back = gb_back_start(g);
  u32 old_back_len = g->cap - old_back;

  while (g->size + amount >= g->cap) {
    g->cap += INIT_CAP;
  }

  char *tmp = realloc(g->buf, g->cap);
  if (!tmp)
    die("Not enough RAM?");
  
  g->buf = tmp;
  
  if (g->size == 0)
    return;

  // move backbuffer to end
  memmove(g->buf + gb_back_start(g), g->buf + old_back, old_back_len);

  int debug = 1;
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

u32 gb_move_left(GapBuffer *g, u32 amount) {
  
  //if (g->point == 0) {
  //  return 0;
  //}

  if (g->point < amount) {
    amount = g->point;
  }

  for (int i = 0; i < amount; i++) {
    g->point--;
    g->col--;
    if (gb_get_current(g) == LK_NEWLINE) {
      g->line--;
      g->col = gb_width_left(g);
    }
  }
  return amount;
}

u32 gb_move_right(GapBuffer *g, u32 amount) {
  //if (g->point == g->size - 1) {
  //  return 0;
  //}
  if (g->point + amount >= g->size)
    amount = g->size - g->point;

  for (int i = 0; i < amount; i++) {
    if (gb_get_current(g) == LK_NEWLINE) {
      g->line++;
      g->col = 0;
    }
    else {
      g->col++;
    }
    g->point++;
  }
  return amount;
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
  gb_check_increase(g, 1);
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

bool gb_has_selection(GapBuffer *g) {
  return g->sel_start != UINT32_MAX; 
}

u32 gb_backspace(GapBuffer *g, u32 amount) {
 
  amount = MIN(amount, g->point);

  if (gb_has_selection(g)) {
    u32 sel_left = MIN(g->sel_start, g->sel_end);
    u32 sel_right = MAX(g->sel_start, g->sel_end);
    amount = sel_right - sel_left;
    g->point = sel_right;
  }
  gb_jump(g); // correct, here?

  for (int i = 0; i < amount; i++) { 
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
  }
  return amount;
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

// TODO check cap before !!!
void gb_copy(GapBuffer *g, char* p_buffer) {
  u32 sel_left = MIN(g->sel_start, g->sel_end);
  u32 sel_right = MAX(g->sel_start, g->sel_end);
  u32 len = sel_right - sel_left;

  g->point = sel_right; // to move all of the string to frontbuffer
  gb_jump(g);
 
  strncpy(p_buffer, g->buf + sel_left, len);
  p_buffer[len + 1] = 0;
}

void gb_cut(GapBuffer *g, char* p_buffer) {
  gb_copy(g, p_buffer);
}

// TODO
// void gb_trim_trailing()  <- trim trailing whitespaces

// TODO refactor and write in 2 steps (front, back)
void gb_write_to_file(GapBuffer *g, char* filename) {
  if (filename == NULL)
    return;
  FILE *file = fopen(filename, "w");
  if (!file) {
    die("[gap_buffer.c] Can't write to file: %s", filename);
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
 
  u32 size = ftell(file);
  fseek(file, 0, SEEK_SET);

  gb_check_increase(g, size);
  fread(g->buf, sizeof(char), size, file);
  g->size = size;
  g->front = size;
  g->buf[g->size + 1] = 0;
  gb_count_maxlines(g);
  fclose(file);
  return 0;
}
