// TODO
//
// [ ] show different bar in debug mode
// [ ] indicate dirty file in bar
// [ ] disable autosave
// [ ] improve bar
//
// [X] BUG: start of file -> enter left left right|down
// [ ] BUG: bug when line_width is bigger than screen_w
// [X] BUG: save in correct file
// [ ] BUG: increase buffer when necessary
// [ ] BUG: increase pad size when necessary
// [ ] BUG: screen resize
// [ ] CORE: select text
// [ ] CORE: copy / paste inside lim
// [ ] CORE: open directories (by "lim <path>" or simply "lim" [like "lim ."])
// [ ] FEAT?: save file before closing lim or opening another file
// [ ] FEAT: second editor
// [ ] FEAT: indicate saved file

#include "gap_buffer.c"

#define LK_TICK 39
/************************ #MISC ******************************/

#define TEXT_RED(w) wattrset(w, COLOR_PAIR(1)) 
#define TEXT_GREEN(w) wattrset(w, COLOR_PAIR(2))
#define TEXT_ORANGE(w) wattrset(w, COLOR_PAIR(3))
#define TEXT_BLUE(w) wattrset(w, COLOR_PAIR(4))
#define TEXT_PINK(w) wattrset(w, COLOR_PAIR(5))
#define TEXT_TEAL(w) wattrset(w, COLOR_PAIR(6))
#define TEXT_WHITE(w) wattrset(w, COLOR_PAIR(7))

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

/************************* #EDITOR ******************************/


typedef enum {
  TEXT, OPEN
} State;

typedef struct {
  u16 screen_h;        // height of window in rows
  u16 screen_w;        // width of window in cols
  u8 screen_line;      // line of the point on the screen
  u32 pad_pos;         // offset from top of pad to top of screen
  char *path;          // basepath of the current Project
  char **files;        // files below basepath
  char *filename;      // name of the file that is currently edited
  u32 chosen_file;     // selected file in open_file menu
  u32 files_len;       // amount of files found in path
  bool should_refresh; // if the editor should refresh
  bool dirty;
  State state;         // the state of the editor
  WINDOW *textPad;     // the area for the text
  WINDOW *linePad;     // the area for the linenumbers
  WINDOW *popupArea;   // the area for dialogs
  #if SHOW_BAR
  WINDOW *statArea;
  #endif
} Editor;

void editor_init(Editor *e) {
  e->screen_h = 0;
  e->screen_w = 0;
  e->screen_line = 0;
  e->pad_pos = 0;
  e->chosen_file = 0;
  e->files = NULL;
  e->files_len = 0;
  e->filename = NULL;
  e->should_refresh = false;
  e->dirty = 0;
  e->path = get_path();
  e->state = TEXT;
  
  // INIT TEXT_PAD
  e->textPad = NULL;
  getmaxyx(stdscr, e->screen_h, e->screen_w);
  e->textPad = newpad(1000, e->screen_w - 4);
  if (!e->textPad)
    die("Could not init textPad");
  wattrset(e->textPad, COLOR_PAIR(1));
  keypad(e->textPad, TRUE);
  
  // INIT LINE_PAD
  e->linePad = NULL;
  e->linePad = newpad(1000, 4);
  if (!e->linePad)
    die("Could not init linePad");
  
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
  #if SHOW_BAR
  e->statArea = newwin(1, e->screen_w, 0, 0);
  mvwin(e->statArea, e->screen_h - 1, 0);
  wattrset(e->statArea, COLOR_PAIR(4));
  #endif //SHOW_BAR
}



void ncurses_init() {
  initscr();
  start_color();
  atexit((void*)endwin);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, 1, 0); // RED
  init_pair(2, 2, 0); // GREEN
  init_pair(3, 3, 0); // ORANGE
  init_pair(4, 4, 0); // BLUE
  init_pair(5, 5, 0); // PINK
  init_pair(6, 6, 0); // TEAL
  init_pair(7, 7, 0); // WHITE
  raw();
  nonl(); // for LK_ENTER|13
  noecho();
}

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
  e->screen_line = 0;
  e->pad_pos = 0;
  e->state = TEXT;
  //draw_line_area(e, g);
}

// ---------------- #TEXT FUNCTIONS -----------------------------

void text_move_up(Editor *e, GapBuffer *g) {
  if (gb_move_up(g)) {
    if (e->screen_line <= 8 && e->pad_pos > 0)
      e->pad_pos--;
    else
      e->screen_line--;
  }
}

void text_move_down(Editor *e, GapBuffer *g) {
  if (gb_move_down(g)) {
    if (e->screen_line >= e->screen_h - 8)
      e->pad_pos++;
    else
      e->screen_line++;
  }
}

void text_move_left(Editor *e, GapBuffer *g) {
  if (gb_move_left(g) && gb_get_current(g) == LK_NEWLINE) {
    if (e->screen_line <= 8 && e->pad_pos > 0)
      e->pad_pos--;
    else
      e->screen_line--;
  }
}

void text_move_right(Editor *e, GapBuffer *g) {
  if (g->line < g->maxlines - 2 && gb_get_current(g) == LK_NEWLINE) {
    if (e->screen_line >= e->screen_h - 8)
      e->pad_pos++;
    else
      e->screen_line++;
  }
  gb_move_right(g);
}

// TODO delete more than one character
void text_backspace(Editor *e, GapBuffer *g) {
  u32 maxlines = g->maxlines;
  e->should_refresh = gb_backspace(g);
  if (maxlines > g->maxlines) {
    if (e->screen_line <= 8 && e->pad_pos > 0)
      e->pad_pos--;
    else
      e->screen_line--;
  }
}

void text_enter(Editor *e, GapBuffer *g) {
  gb_enter(g);
  e->should_refresh = true;
  if (e->screen_line >= e->screen_h - 8)
    e->pad_pos++;
  else
    e->screen_line++;
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
  TEXT_BLUE(e->textPad);
  for (u32 i = 0; i < g->size; i++) {
    waddch(e->textPad, gb_get_char(g, i));
  }
}

void print_text_area(Editor *e, GapBuffer *g) {
  // wattrset(e->textPad, COLOR_PAIR(1));
  wmove(e->textPad, 0, 0);
  wclear(e->textPad);
  
  u16 file_len = strlen(e->filename);
  //die("[lim.c: print_text_area] file ending: %s, %d", e->filename + file_len - 2, file_len);
  if (strcmp(e->filename + file_len - 2, ".c") == 0)
    print_c_file(e, g);
  else
    print_normal(e, g);
  
  if (!e->dirty) {
    wattrset(e->textPad, COLOR_PAIR(2));
    mvwaddch(e->textPad, e->pad_pos + e->screen_h - 2 - SHOW_BAR, e->screen_w - 6, 'S');
  }
}

void draw_line_area(Editor *e, GapBuffer *g) {
  wclear(e->linePad);
  for (int i = 0; i < g->maxlines; i++) {
    mvwprintw(e->linePad, i - 1, 0, "%d", i);
  }
  wrefresh(e->linePad);
}

#if SHOW_BAR
int print_status_line(WINDOW *statArea, GapBuffer *g, Editor *e, int c) {
  wmove(statArea, 0, 0);
  wprintw(statArea, "last: %d", c);
  //wprintw(statArea, ", fn: %s", e->filename);
  //wprintw(statArea, ", ed: (%d, %d)", g->line, g->col);
  wprintw(statArea, ", point: %d", g->point);
  wprintw(statArea, ", pos: %d", gb_pos(g, g->point));
  //wprintw(statArea, ", front: %d", g->front);
  //wprintw(statArea, ", C: %d", gb_get_current(g));
  wprintw(statArea, ", size: %d", g->size);
  wprintw(statArea, ", e.line: %d", e->screen_line);
  wprintw(statArea, ", e.pad_pos: %d", e->pad_pos);
  //wprintw(statArea, ", maxl: %d", g->maxlines);
  wprintw(statArea, ", wl: %d", gb_width_left(g));
  wprintw(statArea, ", wr: %d", gb_width_right(g));
  //wprintw(statArea, ", cf: %d", e->chosen_file);
  //wprintw(statArea, ", fl: %d", e->files_len);
  //wprintw(statArea, ", prev: %d", gb_prev_line_width(g));
  wprintw(statArea, "\t\t\t");
}
#endif //SHOW_BAR

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
  u8 last_line = 0;
  #if SHOW_BAR
  print_status_line(e->statArea, g, e, c);
  wrefresh(e->statArea);
  last_line = 1;
  #endif // SHOW_BAR
  if (e->state == TEXT && e->should_refresh) {
    print_text_area(e, g);
    draw_line_area(e, g); // maybe separate bool for this ?
  }
  refresh();
  wmove(e->textPad, g->line, g->col);
  prefresh(e->linePad, e->pad_pos, 0, 0, 0, e->screen_h - 1 - last_line, 4);
  prefresh(e->textPad, e->pad_pos, 0, 0, 4, e->screen_h - 1 - last_line, e->screen_w - 1);

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
    gb_write_to_file(g, e->filename);
    open_open_file(e, g);
  }
}

void handle_text_state(Editor *e, GapBuffer *g, int c) {

  if (c == KEY_UP || c == CTRL('i')) {
    text_move_up(e, g);
  }

  else if (c == LK_DOWN || c == CTRL('k')) {
    text_move_down(e, g);
  }

  else if (c == KEY_RIGHT || c == CTRL('l')) {
    text_move_right(e, g);
  }

  else if (c == KEY_LEFT || c == CTRL('j')) {
    text_move_left(e, g);
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
    gb_jump(g);
    g->buf[g->front] = c;
    g->size++;
    g->front++;
    g->point++;
    g->col += 1;
    e->should_refresh = true;
    e->dirty = true;
  }

  else if (c == CTRL('r')) {
    e->state = OPEN;
    e->should_refresh = true;
  }

  // else if (c == 127) {
  // }
}

/************************** #MAIN ********************************/

int main(int argc, char **argv) {

  ncurses_init();

  // TODO maybe for refreshing in TTY
  // struct timeval tp;
  // u16 millis = 1000;
  // u16 old_millis = 1000;
  // u16 delta = 1000;
  
  Editor e;
  editor_init(&e);

  GapBuffer g;
  gb_init(&g, INIT_CAP);
  g.buf = calloc(g.cap, sizeof(char));

  get_file_system(e.path, &e.files, &e.files_len); 
  
  if (argc > 1) {
    int len = MIN(strlen(argv[1]), 100);
    e.filename = malloc(len);
    if (!e.filename)
      die("could not allocate mem");
    strncpy(e.filename, argv[1], len + 1);
    gb_read_file(&g, e.filename);
    e.should_refresh = true;
  }

  int c = - 1;
  do {
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
    c = wgetch(e.textPad);
  } while (c != CTRL('q') && c != CTRL('b'));
  
  //if (c == CTRL('q'))
    //gb_write_to_file(&g, e.filename);  
  clear();
  endwin();
  return 0;
}
