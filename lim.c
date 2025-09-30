#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <signal.h>

#include "gap_buffer.c"

#define LK_TICK 39
#define LK_PGDN 338
#define LK_PGUP 339
#define GOTO_MAX 10 

/************************ #MISC ******************************/

char *get_path() {
  #define PATH_MAX 4096
  char buffer[PATH_MAX];
  if (getcwd(buffer, PATH_MAX) == NULL) {
    perror("getcwd");
    return NULL;
  }
  u16 cwd_len = strlen(buffer) + 1;
  char *path = malloc(cwd_len);
  strcpy(path, buffer);
  return path;
}

bool is_directory(char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    die("stat dieded: %s", path);
  return S_ISDIR(statbuf.st_mode);
}

typedef enum {
  BLINKING_BLOCK = 1,
  STEADY_BLOCK,
  BLINKING_UNDERLINE,
  STEADY_UNDERLINE,
  BLINKING_BAR,
  STEADY_BAR,
} CursorShape;

void set_cursor_shape(int code) {
  printf("\033[%d q", code);
  fflush(stdout); // Ensure it gets sent to terminal
}

/************************* #EDITOR ******************************/

typedef enum {
  TEXT, OPEN, SEARCH, GOTO,
} State;

typedef struct {
  u16 text;
  u16 type;
  u16 keyword;
  u16 comment;
  u16 lines;
  u16 bar;
  u16 selected;
  u16 string;
} Mode;

typedef struct {
  u16 screen_h;        // height of window in rows
  u16 screen_w;        // width of window in cols
  u32 pad_pos_y;       // offset from top of pad to top of screen 
  u32 pad_pos_x;       // ---- "" --- left to left of screen 
  u16 text_pad_w;      // total width of textpad
  u16 line_pad_w;      // total width of linepad
  u32 pad_h;           // total height of pads
  char *path;          // basepath of the current Project
  char **files;        // files below basepath
  char *filename;      // name of the file that is currently edited
  u32 chosen_file;     // selected file in open_file menu
  u32 files_len;       // amount of files found in path
  u32 p_buffer_cap;
  char *p_buffer;
  u16 search_point;
  u32 search_line;
  u32 search_col;
  char *search_string;
  char goto_string[GOTO_MAX];
  u8 goto_index;
  bool should_refresh; // if the editor should refresh
  bool refresh_bar;
  bool refresh_text;
  bool refresh_line;
  bool dirty;
  Mode mode;
  State state;         // the state of the editor
  WINDOW *textPad;     // the area for the text
  WINDOW *linePad;     // the area for the linenumbers
  WINDOW *popupArea;   // the area for dialogs
  WINDOW *statArea;
} Editor;

void set_dark_mode(Editor *e) {
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);
  init_pair(5, COLOR_YELLOW, COLOR_BLACK);
  init_pair(6, COLOR_WHITE, COLOR_BLUE);
  init_pair(7, COLOR_BLACK, COLOR_WHITE);
  init_pair(8, COLOR_CYAN, COLOR_BLACK);

  e->mode.text = COLOR_PAIR(1); 
  e->mode.type = COLOR_PAIR(2); 
  e->mode.keyword = COLOR_PAIR(3); 
  e->mode.comment = COLOR_PAIR(4); 
  e->mode.lines = COLOR_PAIR(5); 
  e->mode.bar = COLOR_PAIR(6);
  e->mode.selected = COLOR_PAIR(7);
  e->mode.string = COLOR_PAIR(8);

  wbkgd(e->linePad, e->mode.lines);
  wbkgd(e->textPad, e->mode.text);
  wbkgd(e->statArea, e->mode.bar);
}

void set_light_mode(Editor *e) {
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_GREEN, COLOR_WHITE);
  init_pair(3, COLOR_RED, COLOR_WHITE);
  init_pair(4, COLOR_BLUE, COLOR_WHITE);
  init_pair(5, COLOR_YELLOW, COLOR_WHITE);
  init_pair(6, COLOR_WHITE, COLOR_BLUE); // BAR
  init_pair(7, COLOR_WHITE, COLOR_BLACK); // SELECTED_WHITE
  
  e->mode.text = COLOR_PAIR(1); 
  e->mode.type = COLOR_PAIR(2); 
  e->mode.keyword = COLOR_PAIR(3); 
  e->mode.comment = COLOR_PAIR(4); 
  e->mode.lines = COLOR_PAIR(5); 
  e->mode.bar = COLOR_PAIR(6);
  e->mode.selected = COLOR_PAIR(7);

  bkgd(e->mode.text);  
  wbkgd(e->linePad, e->mode.lines);
  wbkgd(e->textPad, e->mode.text);
  wbkgd(e->statArea, e->mode.bar);
}

void editor_init(Editor *e) {
  e->screen_h = 0;
  e->screen_w = 0;
  e->pad_pos_y = 0;
  e->pad_pos_x = 0;
  e->chosen_file = 0;
  e->files = NULL;
  e->files_len = 0;
  e->p_buffer_cap = 512;
  e->p_buffer = malloc(e->p_buffer_cap);
  if (!e->p_buffer)
    die ("cant alloc");
  e->search_point = 0;
  e->search_string = malloc(1024); //TODO
  e->search_line = 0;
  e->search_col = 0;
  if (!e->search_string) 
    die ("cant alloc search string");
  e->goto_index = 0;
  e->goto_string[0] = 0;
  e->p_buffer[0] = 0;
  e->filename = NULL;
  e->should_refresh = false;
  e->refresh_bar = true;
  e->refresh_text = false;
  e->refresh_line = false;
  e->dirty = 0;
  e->path = get_path();
  e->state = TEXT;
  getmaxyx(stdscr, e->screen_h, e->screen_w);
  
  
  e->pad_h = 100;
  
  // INIT LINE_PAD
  e->linePad = NULL;
  e->line_pad_w = 4; // TODO
  e->linePad = newpad(e->pad_h, e->line_pad_w);
  if (!e->linePad)
    die("Could not init linePad");
  
  // INIT TEXT_PAD
  e->textPad = NULL;
  e->text_pad_w = 100;
  e->textPad = newpad(e->pad_h, e->text_pad_w);
  if (!e->textPad)
    die("Could not init textPad");
  //wattrset(e->textPad, e->mode.text);
  keypad(e->textPad, TRUE);
 
  // INIT POPUP_AREA
  e->popupArea = NULL;
  u16 height = 40;
  u16 width = 80;
  u16 line = (e->screen_h - height) / 2;
  u16 col = (e->screen_w - width) / 2;
  e->popupArea = newwin(height, width, line, col);
  if (!e->popupArea)
    die("Could not init popupArea");
  box(e->popupArea, '|', '-');

  // INIT STAT_AREA
  e->statArea = newwin(1, e->screen_w, 0, 0);
  mvwin(e->statArea, e->screen_h - 1, 0);
}

void ncurses_init() {
  
  
  initscr();
  noecho();
  
  curs_set(1);
  set_cursor_shape(BLINKING_BAR);

  start_color();
  atexit((void*)endwin);

  if (!has_colors()) {
    die("No Colors\n");
  }

  if (can_change_color()) {
    // change colors
  }
  
  raw();
  nonl(); // for LK_ENTER|13
  
  setup_signals();
}

u16 text_area_height(Editor *e) {
  // WHOLE SCREEN MINUS BAR
  return e->screen_h - 1;
}

//TODO loga(maxlines)
u16 line_area_width(Editor *e, GapBuffer *g) {
  return 4;
}

u16 text_area_width(Editor *e) {
  return e->screen_w - e->line_pad_w;
}

// ---------------- #MENU FUNCTIONS -----------------------------

void open_move_up(Editor *e) {
  e->chosen_file = (e->chosen_file + e->files_len - 1) % e->files_len;
}

void open_move_down(Editor *e) {
  e->chosen_file = (e->chosen_file + 1) % e->files_len;
}

void open_open_file(Editor *e, GapBuffer *g) {
  free(e->filename);
  // TODO unsafe?
  int len = strlen(e->files[e->chosen_file]);
  e->filename = malloc(len + 1);
  strcpy(e->filename, e->files[e->chosen_file]);
  gb_read_file(g, e->filename);
  g->line = 0;
  e->pad_pos_y = 0;
  e->state = TEXT;
  e->refresh_bar = true;
  //draw_line_area(e, g);
}

// ---------------- #TEXT FUNCTIONS -----------------------------

void update_cursor(Editor *e, GapBuffer *g) {
  u16 text_area_h = text_area_height(e);
  u16 text_area_w = text_area_width(e);
  i32 diff = 0;
  // DOWN
  if (g->line > e->pad_pos_y + text_area_h - 3)
    e->pad_pos_y = g->line - text_area_h + 3;
  // UP
  else if (g->line < e->pad_pos_y + 2) {
    diff = g->line - 2;
    e->pad_pos_y = MAX(diff, 0);
  }
  // RIGHT X
  if (g->col > e->pad_pos_x + text_area_w - 5)
    e->pad_pos_x = g->col - text_area_w + 5;
  // LEFT
  else if (g->col < e->pad_pos_x + 5) {
    diff = g->col - 5;
    e->pad_pos_x = MAX(0, diff);
  }
  wmove(e->textPad, g->line, g->col);
}

void text_enter(Editor *e, GapBuffer *g) {
  gb_enter(g);
  e->should_refresh = true;
  /*if (e->screen_line >= e->screen_h - 8)
    e->pad_pos_y++;
  else
    e->screen_line++;
  */
}

void text_backspace(Editor *e, GapBuffer *g) {
  u32 maxlines = g->maxlines;
  i32 amount = 1;
  if (g->sel_start != UINT32_MAX) {
    amount = g->sel_end - g->sel_start;
    amount = ABS(amount);
    if (g->sel_end < g->sel_start)
      gb_move_right(g, amount);
    e->should_refresh = true;
    gb_clear_selection(g);
  }
  e->should_refresh = gb_backspace(g, amount);
}

void text_copy(Editor *e, GapBuffer *g) {
  
  if (!gb_has_selection(g)) {
    
  }
  
  // TODO MAKE SURE THAT P_BUFFER IS BIG ENOUGH!!
  //gb_copy(g, e->p_buffer);
  
  //gb_clear_selection(g);
  //e->should_refresh = true;
}

void text_cut(Editor *e, GapBuffer *g) {

  if (g->sel_start == UINT32_MAX)
    return;

  // TODO MAKE SURE THAT P_BUFFER IS BIG ENOUGH!!
  gb_copy(g, e->p_buffer, e->p_buffer_cap);
  gb_backspace(g, 0);

  gb_clear_selection(g);
  e->should_refresh = true;
}

void text_paste(Editor *e, GapBuffer *g) {
  gb_jump(g);
  u32 len = strlen(e->p_buffer);
  gb_check_increase(g, len);
  //die("len: %d", len);
  strncpy(g->buf + g->point, e->p_buffer, len);
  g->front += len;
  g->size += len;
  gb_move_right(g, len);
  gb_count_limits(g);
  e->should_refresh = true;
}

// -------------------------------- #DRAW STUFF ------------------------------------------

bool is_char_in(char c, char f, ...) {
  va_list args;
  va_start(args, f);
    //if (c == args)
      //return true;
  va_end(args);
  return false;
}

void print_c_file(Editor *e, GapBuffer *g) {
  
  char *token = malloc(1024); // TODO errors, size
  u32 sel_1 = MIN(g->sel_start, g->sel_end);
  u32 sel_2 = MAX(g->sel_start, g->sel_end);

  // TODO Better tokenizing
  // TODO Numbers teal
  bool is_char = false;
  bool is_string = false;
  bool is_hashed = false;
  bool is_line_comment = false;
  
  for (u32 i = 0; i < g->size; i++) {
    char c = gb_get_char(g, i);
    if (i >= sel_1 && i < sel_2) {
      wattrset(e->textPad, e->mode.selected);
      waddch(e->textPad, c);
      continue;
    }

    if (is_line_comment) {
      if (c == LK_NEWLINE) {
        is_line_comment = false;
        wattrset(e->textPad, e->mode.text); 
      }
      waddch(e->textPad, c);
      continue;
    }

    if (is_char) {
      waddch(e->textPad, c);
      if (c == LK_TICK) {
        is_char = false;
        wattrset(e->textPad, e->mode.text);
      }
      continue;
    }

    if (is_string) {
      waddch(e->textPad, c);
      if (c == '"') {
        is_string = false;
        wattrset(e->textPad, e->mode.text);
      }
      continue;
    }

    if (c == '/' && (i + 1) < g->size) {
      char d = gb_get_char(g, i + 1);
      if (d  == '/') { 
        is_line_comment = true;
        wattrset(e->textPad, e->mode.comment);
        waddch(e->textPad, c);
        waddch(e->textPad, d);
        i += 1;
        continue;
      }
    }

    if (c == ' ' || c == LK_NEWLINE) {
      waddch(e->textPad, c);
      continue;
    }

    if (c == LK_TICK) {
      is_char = true;
      wattrset(e->textPad, e->mode.string);
      waddch(e->textPad, c);
      continue;
    }
    
    if (c == '"') {
      is_string = true;
      wattrset(e->textPad, e->mode.string);
      waddch(e->textPad, c);
      continue;
    }

    u32 j = 0;
    for (; j < g->size; j++) {
      c = gb_get_char(g, i + j);
      if (c == ' ' || c == LK_NEWLINE || c == '"')
        break;
      token[j] = c;
    }
    i += (j - 1);
    token[j] = 0;
    
    // KEYWORDS
    char *keywords[] = {"break", "continue", "return"};
    bool is_keyword = false;
    for (int i = 0; i < 2; i++) {
      if (strcmp(token, keywords[i]) == 0)
        is_keyword = true;
    }
    
    // TYPES
    char *types[] = {"u8", "u16", "u32", "int", "char", "bool", "void", "struct"};
    bool is_type = false;
    for (int i = 0; i < 7; i++) {
      if (strcmp(token, types[i]) == 0)
        is_type = true;
    }

    if (is_keyword)
      wattrset(e->textPad, e->mode.keyword);
    else if (is_type)
      wattrset(e->textPad, e->mode.type);
    else
      wattrset(e->textPad, e->mode.text);

    waddstr(e->textPad, token);
  }
  free(token);
}

void print_normal(Editor *e, GapBuffer *g) {
  u32 sel_1 = MIN(g->sel_start, g->sel_end);
  u32 sel_2 = MAX(g->sel_start, g->sel_end);

  for (u32 i = 0; i < g->size; i++) {
    if (i >= sel_1 && i < sel_2)
      wattrset(e->textPad, e->mode.selected);
    else
      wattrset(e->textPad, e->mode.text);
    waddch(e->textPad, gb_get_char(g, i));
  }
}

void print_text_area(Editor *e, GapBuffer *g) {
  wmove(e->textPad, 0, 0);
  wattrset(e->textPad, e->mode.text);
  //die("mode: %d", e->mode.text);
  wclear(e->textPad);

  u16 file_len = strlen(e->filename);
  if (strcmp(e->filename + file_len - 2, ".c") == 0 
      || strcmp(e->filename + file_len - 2, ".h") == 0)
    print_c_file(e, g);
  else
    print_normal(e, g);
}

void draw_line_area(Editor *e, GapBuffer *g) {
  wattrset(e->linePad, e->mode.lines);
  wclear(e->linePad);
  for (int i = 0; i <= g->maxlines; i++) {
    mvwprintw(e->linePad, i - 1, 0, "%*d\n", 3, i); //TODO loga(maxlines)
  }
  wrefresh(e->linePad);
}

int print_status_line(GapBuffer *g, Editor *e, int c) {
  wclear(e->statArea);
  wmove(e->statArea, 0, 0);
  #if DEBUG_BAR
  wprintw(e->statArea, "last: %d", c);
  //wprintw(e->statArea, ", fn: %s", e->filename);
  wprintw(e->statArea, ", ed: (%d, %d)", g->line+1, g->col+1);
  int line, col;
  gb_get_line_col(g, &line, &col, g->point);
  wprintw(e->statArea, ", ed2: (%d, %d)", line+1, col+1);
  
  wprintw(e->statArea, ", point: %d", g->point);
  //wprintw(e->statArea, ", pos: %d", gb_pos(g, g->point));
  
  wprintw(e->statArea, ", front: %d", g->front);
  //wprintw(e->statArea, ", C: %d", gb_get_current(g));
  wprintw(e->statArea, ", size: %d", g->size);
  //wprintw(e->statArea, ", cap: %d", g->cap);
  
  // TEXT_PAD_Y
  //wprintw(e->statArea, ", line: %d", g->line /* + pad_pos_y ? */);
  //wprintw(e->statArea, ", t.h: %d", text_area_height(e));
  //wprintw(e->statArea, ", pad_pos_y: %d", e->pad_pos_y);
  //wprintw(e->statArea, ", pad_h: %d", e->pad_h);
  
  // TEXT_PAD_X
  //wprintw(e->statArea, ", e.pad_pos_x: %d", e->pad_pos_x);
  //wprintw(e->statArea, ", col: %d", g->col);
  //wprintw(e->statArea, ", TA_width: %d", text_area_width(e));
  //wprintw(e->statArea, ", TP_width: %d", e->text_pad_w);
  //wprintw(e->statArea, ", maxcols: %d", g->maxcols);


  //wprintw(e->statArea, ", sel_s: %d", g->sel_start);
  //wprintw(e->statArea, ", sel_e: %d", g->sel_end);
  wprintw(e->statArea, ", p: %s", e->p_buffer);
  
  //wprintw(e->statArea, ", maxl: %d", g->maxlines);
  
  //wprintw(e->statArea, ", wl: %d", gb_width_left(g));
  //wprintw(e->statArea, ", wr: %d", gb_width_right(g));
  //wprintw(e->statArea, ", cf: %d", e->chosen_file);
  //wprintw(e->statArea, ", fl: %d", e->files_len);
  //wprintw(e->statArea, ", prev: %d", gb_prev_line_width(g));
  wprintw(e->statArea, "\t\t\t");
  #else
  wprintw(e->statArea, "%*d", 3, g->maxlines);
  mvwprintw(e->statArea, 0, 10, "%s/%s", e->path, e->filename);
  #endif
  if (e->dirty)
    mvwprintw(e->statArea, 0, e->screen_w - 2, "%c", 'M');
  wrefresh(e->statArea);
}

void print_files(Editor *e) {
  wattrset(e->popupArea, COLOR_PAIR(1));
  box(e->popupArea, ACS_VLINE, ACS_HLINE);
  wattrset(e->popupArea, COLOR_PAIR(0));
  mvwprintw(e->popupArea, 2, 4, "PATH: %s", e->path);
  int line = 4;
  for (int i = 0; i < e->files_len; i++, line++) {
    if (i == e->chosen_file)
      wattrset(e->popupArea, COLOR_PAIR(4));
    else
      wattrset(e->popupArea, COLOR_PAIR(1));

    mvwprintw(e->popupArea, line, 4, "%s", e->files[i]);
  }
}

void print_search_window(Editor *e) {
  wattrset(e->popupArea, COLOR_PAIR(1));
  box(e->popupArea, ACS_VLINE, ACS_HLINE);
  wattrset(e->popupArea, COLOR_PAIR(0));
  mvwprintw(e->popupArea, 1, 1, "%s", e->search_string);
}

void print_goto_window(Editor *e) {
  wattrset(e->popupArea, COLOR_PAIR(1));
  box(e->popupArea, ACS_VLINE, ACS_HLINE);
  wattrset(e->popupArea, COLOR_PAIR(0));
  mvwprintw(e->popupArea, 1, 1, "goto: %s", e->goto_string);
}

void check_pad_sizes(Editor *e, GapBuffer *g) {
  while (e->pad_h < g->maxlines + 10)
    e->pad_h += 20;
  while (e->text_pad_w < g->maxcols + 10)
    e->text_pad_w += 10;
  e->text_pad_w = MAX(e->text_pad_w, e->screen_w - e->line_pad_w);
  wresize(e->linePad, e->pad_h, e->line_pad_w);
  wresize(e->textPad, e->pad_h, e->text_pad_w);
}

void draw_editor(Editor *e, GapBuffer *g, int c) {

  if (e->state == TEXT && e->should_refresh) {
    check_pad_sizes(e, g);
    draw_line_area(e, g); // maybe separate bool for this ?
    print_text_area(e, g);
  }

  update_cursor(e, g);

  if (e->refresh_bar) {
    print_status_line(g, e, c);
    // MOVE CURSOR OUT OF STATUS_BAR
    wmove(e->textPad, g->line, g->col);
  }

  prefresh(e->linePad, e->pad_pos_y, 0, 0, 0, e->screen_h - 2, 4);
  prefresh(e->textPad, e->pad_pos_y, e->pad_pos_x, 0, 4, e->screen_h - 2, e->screen_w - 1);

  if (e->state == OPEN && e->should_refresh) {
    wclear(e->popupArea);
    wresize(e->popupArea, 40, 80);
    print_files(e);
    wrefresh(e->popupArea);
  }

  if (e->state == SEARCH && e->should_refresh) {
    wclear(e->popupArea);
    wresize(e->popupArea, 3, 40);
    print_search_window(e);
    wrefresh(e->popupArea);
  }

  if (e->state == GOTO && e->should_refresh) {
    wclear(e->popupArea);
    wresize(e->popupArea, 3, 40);
    print_goto_window(e);
    wrefresh(e->popupArea);
  }
}

// UNFINISHED
void get_functions(GapBuffer *g) {
  u8 num = 100;
  char **buffer = malloc(num);
  if (!buffer)
    die("no buffer");
  
  for (u32 i = 0; i < num; i++) {
    buffer[i] = malloc(100);
    //sprintf(buffer[i], "void my_function_%d", i);
  }

  for (u32 i = 0; i < g->size; i++) {
    
  }
  endwin();
  int j = 0;
  for (int i = 0; i < num; i++) {
    printf("%s\n", buffer[i]);
  }
  exit(0);
}

// ---------------- #KEY HANDLING --------------------------------

void handle_open_state_keys(Editor *e, GapBuffer *g, int c) {
  e->should_refresh = true;
  if (c == KEY_UP || c == CTRL('i'))
    open_move_up(e);
  else if (c == LK_DOWN || c == CTRL('k'))
    open_move_down(e);
  else if (c == CTRL('r'))
    e->state = TEXT;
  else if (c == CTRL('o') || c == LK_ENTER) {
    // Save before loading a different file
    // gb_write_to_file(g, e->filename);
    open_open_file(e, g);
  }
}

void handle_search_state_keys(Editor *e, GapBuffer *g, int c) {
  if (c >= 32 && c <= 126) {
    e->search_string[e->search_point++] = c;
    if (gb_search(g, e->search_string, 0, &e->search_line, &e->search_col)) {
      die("found at (%d, %d)", e->search_line, e->search_col);
      g->line = e->search_line;
      g->col = e->search_col;
    }
  }
  else if (c == CTRL('o') || c == LK_ENTER) {
    e->state = TEXT;
  }
  e->should_refresh = true;
}

void handle_goto_state_keys(Editor *e, GapBuffer *g, int c) {
  /* GOTO_MAX can hold 9 numbers plus the '\0'-bit */
  if (c >= 48 && c <= 57 && e->goto_index < GOTO_MAX - 1) {
    e->goto_string[e->goto_index++] = c;
  }
  else if (c == KEY_BACKSPACE && e->goto_index > 0) {
    e->goto_string[--e->goto_index] = 0;
  }
  else if (c == CTRL('o') || c == LK_ENTER) {
    u32 line = strtol(e->goto_string, NULL, 10);
    if (line > 0 && line <= g->maxlines)
      gb_goto_line(g, line);
    e->state = TEXT;
  }
  e->should_refresh = true;
}

void check_selected(Editor *e, GapBuffer *g) {
  if (g->sel_start != UINT32_MAX) {
    g->sel_end = g->point;
    e->should_refresh = true;
  }
}

void handle_text_state_keys(Editor *e, GapBuffer *g, int c) {
  e->refresh_bar = true;
  if (c == KEY_UP || c == CTRL('i')) {
    gb_move_up(g, 1);
    check_selected(e, g);
  }
  else if (c == LK_DOWN || c == CTRL('k')) {
    gb_move_down(g, 1);
    check_selected(e, g);
  }
  else if (c == KEY_RIGHT || c == CTRL('l')) {
    gb_move_right(g, 1);
    check_selected(e, g);
  }
  else if (c == KEY_LEFT || c == CTRL('j')) {
    gb_move_left(g, 1);
    check_selected(e, g);
  }
  else if (c == LK_PGDN) {
    gb_move_down(g, 50);
  }
  else if (c == LK_PGUP) {
    gb_move_up(g, 50);
  }
  else if (c == KEY_BACKSPACE || c == CTRL('u')) {
    text_backspace(e, g);
    e->dirty = true;
  }
  else if (c == CTRL('s')) {
    gb_write_to_file(g, e->filename);
    e->dirty = false;
    e->should_refresh = true;
  }
  else if (c == CTRL('o') || c == LK_ENTER) {
    text_enter(e, g);
    e->dirty = true;
  }
  else if (c == CTRL('p')) {
    gb_tab(g, 2);
    e->should_refresh = true;
    e->dirty = true;
  }
  else if (c == KEY_HOME) {
    gb_home(g);
  }
  else if (c == KEY_END) {
    gb_end(g);
  }
  else if (c >= 32 && c <= 126) {
    gb_insert_char(g, c);
    g->col += 1;
    g->maxcols = MAX(g->maxcols, g->col);
    e->should_refresh = true;
    e->dirty = true;
  }
  else if (c == CTRL('r')) {
    e->state = OPEN;
    e->should_refresh = true;
  }
  else if (c == CTRL('f')) {
    memset(e->search_string, 0, 1024);
    e->search_point = 0;
    e->state = SEARCH;
    e->should_refresh = true;
  }
  else if (c == CTRL('g')) {
    memset(e->goto_string, 0, GOTO_MAX);
    e->goto_index = 0;
    e->state = GOTO;
    e->should_refresh = true;
  } 
  else if (c == CTRL('d')) {
    if (g->sel_start == UINT32_MAX) {
      g->sel_start = g->sel_end = g->point;
      set_cursor_shape(STEADY_BLOCK);
    }
    else {
      g->sel_start = g->sel_end = UINT32_MAX;
      set_cursor_shape(BLINKING_BAR);
      e->should_refresh = true;
    }
  }
  else if (c == CTRL('c')) {
    e->should_refresh = gb_copy(g, e->p_buffer, e->p_buffer_cap);
    set_cursor_shape(BLINKING_BAR);
  }
  else if (c == CTRL('x')) {
    //text_cut(e, g);
  }
  else if (c == CTRL('v')) {
    text_paste(e, g);
  }
}

/************************** #MAIN ********************************/

int main(int argc, char **argv) {

  ncurses_init();
  
  Editor e;
  editor_init(&e);
  set_dark_mode(&e);
  
  GapBuffer g;
  gb_init(&g, INIT_CAP);
  g.buf = calloc(g.cap, sizeof(char));

  get_file_system(e.path, &e.files, &e.files_len); 
  
  for (int i = 1; i < argc; i++) {
    /* if (strcmp(argv[i], "-l") == 0) {
      set_light_mode(&e);
      continue;
    } */
    int len = MIN(strlen(argv[1]), 100);
    e.filename = calloc(len, 1);
    if (!e.filename)
      die("could not allocate mem");
    strncpy(e.filename, argv[1], len);
    
    if (is_directory(e.filename)) {
      e.state = OPEN;
    } else {
      gb_read_file(&g, e.filename);
      e.should_refresh = true;
    }
  } 
  
  if (argc == 1) {
    e.state = OPEN;
  }

  int c = -1;
  do {
    if (c == KEY_RESIZE) {
      getmaxyx(stdscr, e.screen_h, e.screen_w);
      //e.text_pad_w = e.screen_w - e.line_pad_w;
      wresize(e.textPad, e.pad_h, e.text_pad_w);
      e.should_refresh = true;
      e.refresh_bar = true;
    }
    switch (e.state) {
      case OPEN:
        handle_open_state_keys(&e, &g, c);
        break;
      case GOTO:
        handle_goto_state_keys(&e, &g, c);
        break;
      case SEARCH:
        handle_search_state_keys(&e, &g, c);
        break;
      case TEXT:
        handle_text_state_keys(&e, &g, c);
        break;
      default:
        die("no state");
    }
    //get_functions(&g);
    draw_editor(&e, &g, c);
    e.should_refresh = false;
    e.refresh_bar = false;
    c = wgetch(e.textPad);
  } while (c != CTRL('q'));
  return 0;
}
