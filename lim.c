// TODO

// X = DONE, C = CONFIGURABLE

// [X] BUG: increase buffer when necessary
// [X] BUG: call endwin() on signals
// [ ] BUG: increase pad size when necessary
// [ ] BUG: bug when line_width is bigger than screen_w <- scroll pad on x axis
// [ ] BUG: screen resize; refresh bar

// [X] CORE: open directories (by "lim <path>" or simply "lim" [like "lim ."])
// [X] CORE: select text
// [ ] CORE: copy / paste inside lim
// [ ] CORE: keep offset when moving up/down
// [ ] CORE: indentation key

// [C] FEAT: save file before closing lim or opening another file
// [ ] FEAT: fuzzy find in open dialog
// [ ] FEAT: find in file(s)
// [ ] FEAT: undo
// [ ] FEAT: second editor
// [ ] FEAT: use libtermkey or anyting better for keys (resize ?)
// [ ] FEAT: Chapters

// [ ] PERFORMANCE: e.g. just refresh single line in some cases, ...

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <signal.h>

#include "gap_buffer.c"

#define LK_TICK 39
/************************ #MISC ******************************/

#define TEXT_RED(w) wattrset(w, COLOR_PAIR(1)) 
#define TEXT_GREEN(w) wattrset(w, COLOR_PAIR(2))
#define TEXT_YELLOW(w) wattrset(w, COLOR_PAIR(3))
#define TEXT_BLUE(w) wattrset(w, COLOR_PAIR(4))
#define TEXT_PINK(w) wattrset(w, COLOR_PAIR(5))
#define TEXT_TEAL(w) wattrset(w, COLOR_PAIR(6))
#define TEXT_WHITE(w) wattrset(w, COLOR_PAIR(7))
#define PAIR_BAR COLOR_PAIR(8) // White/Blue
#define PAIR_SELECTED COLOR_PAIR(9) // Black/White


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

/************************* #EDITOR ******************************/

typedef enum {
  TEXT, OPEN
} State;

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
  bool should_refresh; // if the editor should refresh
  bool refresh_bar;
  bool refresh_text;
  bool refresh_line;
  bool dirty;
  State state;         // the state of the editor
  WINDOW *textPad;     // the area for the text
  WINDOW *linePad;     // the area for the linenumbers
  WINDOW *popupArea;   // the area for dialogs
  WINDOW *statArea;
} Editor;

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

  // INIT LINE_PAD
  e->linePad = NULL;
  e->pad_h = 100;
  e->line_pad_w = 4; // TODO
  e->linePad = newpad(e->pad_h, e->line_pad_w);
  if (!e->linePad)
    die("Could not init linePad");
  
  // INIT TEXT_PAD
  e->textPad = NULL;
  e->text_pad_w = 500;
  e->textPad = newpad(e->pad_h, e->text_pad_w);
  if (!e->textPad)
    die("Could not init textPad");
  wattrset(e->textPad, COLOR_PAIR(1));
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
  //keypad(e->popupArea, true);
  //wbkgd(e->popupArea, COLOR_PAIR(2));
  
  // INIT STAT_AREA
  e->statArea = newwin(1, e->screen_w, 0, 0);
  mvwin(e->statArea, e->screen_h - 1, 0);
  wbkgd(e->statArea, PAIR_BAR);
  //wattrset(e->statArea, COLOR_PAIR(4));
}

void ncurses_init() {
  initscr();
  start_color();
  atexit((void*)endwin);

  if (!has_colors()) {
    die("No Colors\n");
  }

  if (can_change_color()) {
    // change colors
  }

  init_pair(1, 1, 0); // RED
  init_pair(2, 2, 0); // GREEN
  init_pair(3, COLOR_YELLOW, 0); // YELLOW
  init_pair(4, COLOR_BLUE, 0); // BLUE
  init_pair(5, 5, 0); // PINK
  init_pair(6, 6, 0); // TEAL
  init_pair(7, 7, 0); // WHITE
  init_pair(8, COLOR_WHITE, COLOR_BLUE); // BAR
  init_pair(9, COLOR_BLACK, COLOR_WHITE); // SELECTED_WHITE
  init_pair(10, COLOR_RED, COLOR_WHITE); // SELECTED_RED
  init_pair(11, COLOR_BLACK, COLOR_WHITE); // SELECTED_WHITE

  raw();
  nonl(); // for LK_ENTER|13
  noecho();

  setup_signals();
  // set cursor to blinking bar
  system("echo -e -n \x1b[\x35 q");
}

u16 text_area_height(Editor *e) {
  // WHOLE SCREEN MINUS BAR
  return e->screen_h - 1;
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

void clear_selection(GapBuffer *g) {
  g->sel_start = g->sel_end = UINT32_MAX;
}

void text_move_up(Editor *e, GapBuffer *g) {
  if (gb_move_up(g)) {
  }
}

void text_move_down(Editor *e, GapBuffer *g) {
  if (gb_move_down(g)) {
  }
}

void text_move_left(Editor *e, GapBuffer *g, u32 amount) {
  gb_move_left(g, 1);
}

void text_move_right(Editor *e, GapBuffer *g, u32 amount) {
  gb_move_right(g, 1);
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
    clear_selection(g);
  }

  e->should_refresh = gb_backspace(g, amount);
}

void text_copy(Editor *e, GapBuffer *g) {
  
  if (g->sel_start == UINT32_MAX)
    return;
  
  // TODO MAKE SURE THAT P_BUFFER IS BIG ENOUGH!!
  gb_copy(g, e->p_buffer);
  
  clear_selection(g);
  e->should_refresh = true;
}

void text_cut(Editor *e, GapBuffer *g) {
 
  if (g->sel_start == UINT32_MAX)
    return;

  // TODO MAKE SURE THAT P_BUFFER IS BIG ENOUGH!!
  gb_copy(g, e->p_buffer);
  gb_backspace(g, 0);

  clear_selection(g);
  e->should_refresh = true;
}

void text_paste(Editor *e, GapBuffer *g) {
  gb_jump(g);
  u32 len = strlen(e->p_buffer);
  //die("len: %d", len);
  strncpy(g->buf + g->point, e->p_buffer, len);
  g->front += len;
  g->size += len;
  gb_move_right(g, len);
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

  // TODO Better tokenizing
  // TODO Numbers teal
  bool is_char = false;
  bool is_string = false;
  bool is_hashed = false;
  bool is_line_comment = false;
  
  for (u32 i = 0; i < g->size; i++) {
    
    char c = gb_get_char(g, i);
    bool is_selected = false;

    if (i >= g->sel_start && i < g->sel_end) {
      is_selected = true;
    }

    if (is_line_comment) {
      if (c == LK_NEWLINE) {
        is_line_comment = false;
        TEXT_RED(e->textPad);
      }
      waddch(e->textPad, c);
      continue;
    }

    if (is_char) {
      waddch(e->textPad, c);
      if (c == LK_TICK) {
        is_char = false;
        wattrset(e->textPad, COLOR_PAIR(0));
      }
      continue;
    }

    if (is_string) {
      waddch(e->textPad, c);
      if (c == '"') {
        is_string = false;
        wattrset(e->textPad, COLOR_PAIR(0));
      }
      continue;
    }
    
    if (c == '/' && (i + 1) < g->size) {
      char d = gb_get_char(g, i + 1);
      if (d  == '/') { 
        is_line_comment = true;
        TEXT_BLUE(e->textPad);
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
      wattrset(e->textPad, COLOR_PAIR(3));
      waddch(e->textPad, c);
      continue;
    }
    
    if (c == '"') {
      is_string = true;
      TEXT_TEAL(e->textPad);
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
    char *keywords[] = {"const", "return"};
    bool is_keyword = false;
    for (int i = 0; i < 2; i++) {
      if (strcmp(token, keywords[i]) == 0)
        is_keyword = true;
    }
    
    // TYPES
    char *types[] = {"u8", "u16", "u32", "int", "char", "void", "struct"};
    bool is_type = false;
    for (int i = 0; i < 7; i++) {
      if (strcmp(token, types[i]) == 0)
        is_type = true;
    }

    if (is_keyword)
      TEXT_RED(e->textPad);
    else if (is_type)
      TEXT_GREEN(e->textPad);
    else
      wattrset(e->textPad, COLOR_PAIR(0));

    waddstr(e->textPad, token);
  }
  free(token);
}

void print_normal(Editor *e, GapBuffer *g) {
  u32 sel_1 = MIN(g->sel_start, g->sel_end);
  u32 sel_2 = MAX(g->sel_start, g->sel_end);

  for (u32 i = 0; i < g->size; i++) {
    if (i >= sel_1 && i < sel_2)
      wattrset(e->textPad, COLOR_PAIR(9));
    else
      TEXT_WHITE(e->textPad);
    waddch(e->textPad, gb_get_char(g, i));
  }
}

void print_text_area(Editor *e, GapBuffer *g) {
  wmove(e->textPad, 0, 0);
  wclear(e->textPad);
  
  u16 file_len = strlen(e->filename);
  /*if (strcmp(e->filename + file_len - 2, ".c") == 0 
      || strcmp(e->filename + file_len - 2, ".h") == 0)
    print_c_file(e, g);
  else */
  print_normal(e, g);
}

void draw_line_area(Editor *e, GapBuffer *g) {
  TEXT_YELLOW(e->linePad);
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
  //wprintw(e->statArea, ", ed: (%d, %d)", g->line, g->col);
  //wprintw(e->statArea, ", point: %d", g->point);
  //wprintw(e->statArea, ", pos: %d", gb_pos(g, g->point));
  
  //wprintw(e->statArea, ", front: %d", g->front);
  //wprintw(e->statArea, ", C: %d", gb_get_current(g));
  //wprintw(e->statArea, ", size: %d", g->size);
  //wprintw(e->statArea, ", cap: %d", g->cap);
  
  // TEXT_PAD_Y
  wprintw(e->statArea, ", line: %d", g->line /* + pad_pos_y ? */);
  wprintw(e->statArea, ", t.h: %d", text_area_height(e));
  wprintw(e->statArea, ", e.pad_pos_y: %d", e->pad_pos_y);
  
  // TEXT_PAD_X
  //wprintw(e->statArea, ", e.pad_pos_x: %d", e->pad_pos_x);
  //wprintw(e->statArea, ", col: %d", g->col);
  //wprintw(e->statArea, ", TA_width: %d", text_area_width(e));

  //wprintw(e->statArea, ", sel_s: %d", g->sel_start);
  //wprintw(e->statArea, ", sel_e: %d", g->sel_end);
  //wprintw(e->statArea, ", p: %s", e->p_buffer);
  
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

void draw_editor(Editor *e, GapBuffer *g, int c) {
  
  curs_set(1);
    

  if (e->state == TEXT && e->should_refresh) {
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
    curs_set(0);
    wclear(e->popupArea);
    print_files(e);
    wrefresh(e->popupArea);
  }

  
  
}

// ---------------- #KEY HANDLING --------------------------------

void handle_open_state(Editor *e, GapBuffer *g, int c) {
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

void check_selected(Editor *e, GapBuffer *g) {
  if (g->sel_start != UINT32_MAX) {
    g->sel_end = g->point;
    e->should_refresh = true;
  }
}

void handle_text_state(Editor *e, GapBuffer *g, int c) {

  #if DEBUG_BAR
  e->refresh_bar = true;
  #endif
  
  if (c == KEY_UP || c == CTRL('i')) {
    text_move_up(e, g);
    check_selected(e, g);
  }

  else if (c == LK_DOWN || c == CTRL('k')) {
    text_move_down(e, g);
    check_selected(e, g);
  }

  else if (c == KEY_RIGHT || c == CTRL('l')) {
    text_move_right(e, g, 1);
    check_selected(e, g);
  }

  else if (c == KEY_LEFT || c == CTRL('j')) {
    text_move_left(e, g, 1);
    check_selected(e, g);
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
  
  else if (c == KEY_HOME) {
    gb_home(g);
  }

  else if (c == KEY_END) {
    gb_end(g);
  }

  else if (c >= 32 && c <= 126) {
    gb_insert_char(g, c);
    g->col += 1;
    e->should_refresh = true;
    e->dirty = true;
  }

  else if (c == CTRL('r')) {
    e->state = OPEN;
    e->should_refresh = true;
  }

  else if (c == CTRL('d')) {
    if (g->sel_start == UINT32_MAX) {
      g->sel_start = g->sel_end = g->point;
    }
    else {
      g->sel_start = g->sel_end = UINT32_MAX;
      e->should_refresh = true;
    }
  }

  else if (c == CTRL('c'))
    text_copy(e, g);

  else if (c == CTRL('x')) {
    text_cut(e, g);
  }

  else if (c == CTRL('v')) {
    text_paste(e, g);
  }
  // else if (c == 127) {
  // }
}

/************************** #MAIN ********************************/

int main(int argc, char **argv) {

  ncurses_init();
  
  Editor e;
  editor_init(&e);

  GapBuffer g;
  gb_init(&g, INIT_CAP);
  g.buf = calloc(g.cap, sizeof(char));

  get_file_system(e.path, &e.files, &e.files_len); 
  
  if (argc > 1) {
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
  } else {
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
        handle_open_state(&e, &g, c);
        break;
      case TEXT:
        handle_text_state(&e, &g, c);
        break;
    }

    draw_editor(&e, &g, c);
    e.should_refresh = false;
    e.refresh_bar = false;
    c = wgetch(e.textPad);
  } while (c != CTRL('q'));
  
  //clear();
  //endwin();
  return 0;
}
