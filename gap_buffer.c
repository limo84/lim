#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define LSH
#include "lsh_logger.h"
#include "lsh_array.h"

#if 1 // for later implementation of editors without ncurses
#include <ncurses.h>
#endif

#include "files.h"

#ifdef LOGGER
#define LOG_DEBUG(fmt, ...) logger_log(fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

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

// ******************** #GAPBUFFER *******************************/

#define INIT_CAP 1000

typedef struct {
  char *buf;        // the actual gapbuffer
  u32 cap;          // maximum capacity of gapbuffer, should be increased when needed
  u32 size;         // size of written chars (frontbuffer + backbuffer)
  u32 front;        // end of frontbuffer
  u32 point;        // relative position of cursor inside the buffer (get absolute pos with gb_pos)
  u16 line, col;      // number of lines and cols the cursor is at
  u16 maxlines;       // number of maxlines of current buffer
  u16 maxcols;        // maximum width (in cols) the textpad needs
  u16 wanted_offset;  // the offset tried to be restored when moving up or down
  u32 sel_start;      // selection point 1
  u32 sel_end;        // selection point 2
  //u32 search_point;
  Array sps;
  u32 sps_index;
} GapBuffer;


void gb_init(GapBuffer *g, u32 init_cap) {
  g->cap = init_cap;
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->maxlines = 1;
  g->maxcols = 20;
  g->wanted_offset = 0;
  g->sel_start = UINT32_MAX;
  g->sel_end = UINT32_MAX;
  //g->search_point = 0;
  if (array_init(&g->sps, sizeof(u32), 50, 10) < 0)
    die("no memory");
  g->sps_index = 0;
}

// ********************* #DEFINITIONS **************************

void gb_get_line_col(GapBuffer *g, u16 *line, u16 *col, u32 pos);
void gb_get_point(GapBuffer *g, u16 line, u16 col);

// ********************* #MOVE **************************

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

bool gb_has_selection(GapBuffer *g) {
  return g->sel_start != UINT32_MAX; 
}

void gb_clear_selection(GapBuffer *g) {
  g->sel_start = g->sel_end = UINT32_MAX;
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

void gb_count_limits(GapBuffer *g) {
  g->maxcols = 20;
  g->maxlines = 1;
  u16 cols = 0;
  for (u32 i = 0; i < g->size; i++) {
    cols++;
    if (gb_get_char(g, i) == LK_NEWLINE) {
      g->maxlines += 1;
      g->maxcols = MAX(g->maxcols, cols);
      cols = 0;
    }
  }
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
    if (g->point + i >= g->size)
      return i + g->point -g->size;
    if (gb_get_char(g, g->point + i) == LK_NEWLINE)
      return i;
  }
  die("should never be reached (right)");
}

// --------------- MOVE -----------------------------

u32 gb_move_left(GapBuffer *g, u32 amount) {
  if (g->point < amount)
    amount = g->point;
  gb_get_line_col(g, &g->line, &g->col, (g->point -= amount)); 
  g->wanted_offset = g->col;
  return amount;
}

u32 gb_move_right(GapBuffer *g, u32 amount) {
  if (g->point + amount >= g->size)
    amount = g->size - g->point;
  gb_get_line_col(g, &g->line, &g->col, (g->point += amount)); 
  g->wanted_offset = g->col;
  return amount;
}

u32 gb_move_down(GapBuffer *g, u32 amount) {  
  if (g->line + amount >= g->maxlines - 1)
    amount = g->maxlines - g->line - 1; 
  for (u32 i = 0; i < amount; i++) {
    // move to start of next line
    g->point += gb_width_right(g) + 1;
  }
  // move further right to offset
  u16 new_width = gb_width_right(g);
  u16 offset = MIN(g->wanted_offset, new_width);
  g->point += offset;
  // adjust line and col
  g->line += amount;
  g->col = offset;
  return amount;
}

u32 gb_move_up(GapBuffer *g, u32 amount) {  
  amount = MIN(g->line, amount);  
  for (u32 i = 0; i < amount; i++) {
    // move to end of previous line
    g->point -= (gb_width_left(g) + 1);
  }
  // move to start of line
  u16 new_width = gb_width_left(g);
  g->point -= new_width;
  // move to offset
  u16 offset = MIN(g->wanted_offset, new_width);
  g->point += offset;
  //adjust line and col
  g->line -= amount;
  g->col = offset;
  return amount;
}

u32 gb_find_point_by_line(GapBuffer *g, u32 line) {
  if (line == 0)
    return 0;
  for (int i = 0; i < g->size;) {
    line -= (gb_get_char(g, i++) == LK_NEWLINE) ? 1 : 0;
    if (line == 0)
      return i;
  }
}

// Takes line, starting at 1
void gb_goto_line(GapBuffer *g, u32 line) {
  g->point = gb_find_point_by_line(g, line - 1);
  g->line = line - 1;
  g->col = 0;
}

// TODO refactor from here
void gb_goto_position(GapBuffer *g, u8 pos, u32 *line, u32 *col) {
  if (pos >= g->size)
    return;
}

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

void gb_tab(GapBuffer *g, u8 tabsize) {
  for (int i = 0; i < tabsize; i++) {
    gb_insert_char(g, ' ');
  }
  g->col += tabsize;
  g->maxcols = MAX(g->maxcols, g->col);
}

// TODO
u32 gb_backspace(GapBuffer *g) {
  u32 amount = MIN(1, g->point);
  if (gb_has_selection(g)) {
    u32 sel_left = g->sel_start;
    u32 sel_right = g->sel_end + 1;
    if (g->sel_start > g->sel_end) {
      sel_left = g->sel_end;
      sel_right = g->sel_start;
    }
    amount = sel_right - sel_left;
    g->point = MIN(sel_right, g->size);
  }
  g->point -= amount;
  gb_jump(g);
  g->size -= amount;
  gb_get_line_col(g, &g->line, &g->col, g->point);
  return amount;
}

// TODO check cap before !!!
void gb_copy(GapBuffer *g, char* p_buffer, u32 cap) {
  u32 sel_left;
  u32 sel_right;
  memset(p_buffer, 0, cap);
  if (!gb_has_selection(g)) {
    // copy line
    sel_left = g->point - gb_width_left(g);
    sel_right = g->point + gb_width_right(g) + 1;
  } else {
    sel_left = MIN(g->sel_start, g->sel_end);
    sel_right = MAX(g->sel_start, g->sel_end) + 1;
  }
  u32 len = sel_right - sel_left;
  gb_move_right(g, sel_right - g->point); // to move all of the string to frontbuffer
  gb_jump(g);
  strncpy(p_buffer, g->buf + sel_left, len);
  p_buffer[len + 1] = 0;
}

void gb_cut(GapBuffer *g, char* p_buffer, u32 cap) {
  u32 sel_left;
  u32 sel_right;
  memset(p_buffer, 0, cap);
  if (!gb_has_selection(g)) {
    // copy line
    sel_left = g->point - gb_width_left(g);
    sel_right = g->point + gb_width_right(g) + 1;
  } else {
    sel_left = MIN(g->sel_start, g->sel_end);
    sel_right = MAX(g->sel_start, g->sel_end) + 1;
  }
  u32 len = sel_right - sel_left;
  gb_move_right(g, sel_right - g->point); // to move all of the string to frontbuffer
  gb_jump(g);
  strncpy(p_buffer, g->buf + sel_left, len);
  p_buffer[len + 1] = 0;
  gb_backspace(g);
}

// is this performant enough?
void gb_get_line_col(GapBuffer *g, u16 *line, u16 *col, u32 pos) {
  *line = 0;
  *col = 0;
  for (int i = 0; i < pos; i++) {
    *col += 1;
    if (gb_get_char(g, i) == LK_NEWLINE) {
      *line += 1;
      *col = 0;
    }
  }
}

bool __compare__(GapBuffer *g, u32 offset, char *needle) {
  for (int i = 0; needle[i]; i++)
    if (gb_get_char(g, offset + i) != needle[i])
      return false;
  return true;
}

u32 gb_search(GapBuffer *g, char *s, u32 start, u16 *line, u16 *col) {
  if (!s || !s[0])
    return false;
  if (strlen(s) <= 2)
    return false;
  g->sps.length = 0;
  g->sps_index = 0;
  for (u32 i = 0; i < g->size; i++) {
    if (__compare__(g, i, s)) {
      array_add(&g->sps, 1, &i); 
    }
  }
  if (g->sps.length) {
    u32 *first = array_get(&g->sps, 0);
    for (int k = 0; k < g->sps.length; k++) {
      u32 *tmp = array_get(&g->sps, k);
      LOG_DEBUG("%d\n", *tmp);
    }
    gb_get_line_col(g, line, col, *first);
  }
  return g->sps.length;
}

// TODO
// void gb_trim_trailing()  <- trim trailing whitespaces

#define LINE_SIZE 1024

FILE* __get_recently_opened(char *attr) {
  char config_path[LINE_SIZE];
  if (getenv("XDG_CONFIG_HOME") != NULL)
    sprintf(config_path, "%s/%s", getenv("XDG_CONFIG_HOME"), "lim/recent");
  else 
    die("XDG_CONFIG_HOME not set");
  return fopen(config_path, attr);
}

void gb_store_position(GapBuffer *g, char *path, char *current_file) {
  FILE *file = __get_recently_opened("r");
  char *buffer = NULL;
  char full_path[LINE_SIZE];
  snprintf(full_path, LINE_SIZE, "%s/%s", path, current_file);
  int len_path = strlen(full_path);
  if (file) {
    if (fseek(file, 0, SEEK_END) != 0)
      die("fseek SEEK_END");
    u32 size = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = malloc(size);
    if (!buffer)
      die("can't create buffer");
    buffer[0] = 0;
    char line[LINE_SIZE];
    while (fgets(line, LINE_SIZE, file)) {
      if (strncmp(full_path, line, len_path) != 0)
        sprintf(buffer, "%s%s", buffer, line);
    }
    fclose(file);
  }
  // --- WRITE FILE ---------------
  file = __get_recently_opened("w");
  fprintf(file, "%s:%d\n%s", full_path, g->line, buffer ? buffer : "");
  fclose(file);
  free(buffer);
}

void gb_restore_position(GapBuffer *g, char *path, char *current_file) {
  FILE *file = __get_recently_opened("r");
  if (!file) {
    return;
  }
  char filename[LINE_SIZE];
  snprintf(filename, LINE_SIZE, "%s/%s", path, current_file);
  u8 len = strlen(filename);
  char line[LINE_SIZE];
  while (fgets(line, LINE_SIZE, file)) {
    if (strncmp(line, filename, len) == 0) {
      u16 l = atoi(line + len + 1);
      gb_goto_line(g, l + 1);
      break;
    }
  }
  fclose(file);
}

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

int gb_read_file(GapBuffer *g, char *path, char *filename) {
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->line = 0;
  g->col = 0;
  g->maxcols = 1;
  g->wanted_offset = 0;
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
  gb_count_limits(g);
  fclose(file);
  return 0;
}
