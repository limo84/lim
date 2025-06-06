// TODO
//
// [X] scroll linePad 
// [X] move cursor to textPad at start
// [ ] dont show cursor in popup
// [ ] make popup nicer
// [X] change controls
// [X] create readme
// [X] make statusLine optional
// [ ] cleanup
// [ ] save in correct file
// [ ] select text
// [ ] copy / paste from clipboard
//
// [ ] increase buffer when necessary
// [ ] increase pad size when necessary
// 

#include "gap_buffer.c"

/************************* #EDITOR ******************************/

#define SHOW_BAR 1

typedef struct {
  u16 screen_y;        // height of window in rows
  u16 screen_x;        // width of window in cols
  u8 screen_line;      // line of the point on the screen
  u32 pad_pos;         // offset from top of pad to top of screen
  u32 chosen_file;     // selected file in open_file menu
  u32 files_len;       // amount of files found in path
  char **files;        // files
  bool should_refresh; // if the editor should refresh
  WINDOW *textPad;     // the area for the text
  WINDOW *linePad;     // the area for the linenumbers
  WINDOW *popupArea;   // the area for dialogs
} Editor;

void editor_init(Editor *e) {
  e->screen_y = 0;
  e->screen_x = 0;
  e->pad_pos = 0;
  e->chosen_file = 0;
  e->files = NULL;
  e->files_len = 0;
  e->should_refresh = false;
  
  // INIT TEXT_PAD
  e->textPad = NULL;
  getmaxyx(stdscr, e->screen_y, e->screen_x);
  e->textPad = newpad(1000, e->screen_x - 4);
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
  e->popupArea = newwin(5, 30, 10, 10);
  if (!e->popupArea)
    die("Could not init popupArea");
  wresize(e->popupArea, 30, 60);
  wbkgd(e->popupArea, COLOR_PAIR(2));

}

void print_text_area(Editor *e, GapBuffer *g) {
  wmove(e->textPad, 0, 0);
  wclear(e->textPad);
  u32 point = g->point;
  for (g->point = 0; g->point < g->size; g->point++) {
    waddch(e->textPad, gb_get_current(g));
  }
  g->point = point;
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
  //wprintw(statArea, "last: %d, ", c);
  wprintw(statArea, "ed: (%d, %d), ", g->lin + 1, g->col + 1);
  //wprintw(statArea, "width: %d, ", g->line_width);
  wprintw(statArea, "point: %d, ", g->point);
  wprintw(statArea, "pos: %d, ", gb_pos(g));
  //wprintw(statArea, "front: %d, ", g->front);
  //wprintw(statArea, "C: %d, ", gb_get_current(g));
  wprintw(statArea, "size: %d, ", g->size);
  wprintw(statArea, "e.line: %d, ", e->screen_line);
  wprintw(statArea, "e.pad_pos: %d, ", e->pad_pos);
  //wprintw(statArea, "lstart: %d, ", g->line_start);
  //wprintw(statArea, "lend: %d, ", g->line_end);
  wprintw(statArea, "maxl: %d, ", g->maxlines);
  //wprintw(statArea, "wl: %d, ", gb_width_left(g));
  //wprintw(statArea, "wr: %d, ", gb_width_right(g));
  wprintw(statArea, "cf: %d, ", e->chosen_file);
  wprintw(statArea, "fl: %d, ", e->files_len);
  //wprintw(statArea, "prev: %d, ", gb_prev_line_width(g));
  wprintw(statArea, "\t\t\t");
}
#endif //SHOW_BAR

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

void print_files(Editor *e) {
  int line = 2;
  for (int i = 0; i < e->files_len; i++, line++) {
    if (i == e->chosen_file)
      wattrset(e->popupArea, COLOR_PAIR(4));
    else
      wattrset(e->popupArea, COLOR_PAIR(2));

    mvwprintw(e->popupArea, line, 3, "%s", e->files[i]);
  }
}

void open_move_up(Editor *e) {
  e->chosen_file = (e->chosen_file + e->files_len - 1) % e->files_len;
  e->should_refresh = true;
}

void open_move_down(Editor *e) {
  e->chosen_file = (e->chosen_file + 1) % e->files_len;
  e->should_refresh = true;
}

void open_open_file(GapBuffer *g, Editor *e) {
  gb_clear_buffer(g);
  gb_read_file(g, e->files[e->chosen_file]);
}

typedef enum {
  TEXT, OPEN
} State;


/************************** #MAIN ********************************/

int main(int argc, char **argv) {

  initscr();
  start_color();
  atexit((void*)endwin);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_WHITE, COLOR_RED);

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


  char *path = get_path();

  get_file_system(path, &(e.files), &(e.files_len));


  State state = TEXT;
  
  
  #if SHOW_BAR
  WINDOW *statArea;
  statArea = newwin(1, e.screen_x, 0, 0);
  mvwin(statArea, e.screen_y - 1, 0);
  wattrset(statArea, COLOR_PAIR(4));
  #endif //SHOW_BAR

  
  raw();
  noecho();
  
  if (argc > 1) {
    gb_read_file(&g, argv[1]);
    draw_line_area(&e, &g);
    print_text_area(&e, &g);
    refresh();
    prefresh(e.linePad, e.pad_pos, 0, 0, 0, e.screen_y - 2, 4);
  }

  int c = - 1;
  do {
    e.should_refresh = false;
    if (c == KEY_UP || c == CTRL('i')) {
      if (state == TEXT && gb_move_up(&g)) {
        if (e.screen_line <= 8 && e.pad_pos > 0)
          e.pad_pos--;
        else
          e.screen_line--;
      }
      else
        open_move_up(&e);
    }
    
    else if (c == LK_DOWN || c == CTRL('k')) {
      if (state == TEXT && gb_move_down(&g)) {
        if (e.screen_line >= e.screen_y - 8)
          e.pad_pos++;
        else
          e.screen_line++;
      }
      else
        open_move_down(&e);
    }

    else if (c == KEY_RIGHT || c == CTRL('l')) {
      if (state == TEXT) {
        if (g.lin < g.maxlines - 2 && gb_get_current(&g) == LK_ENTER) {
          if (e.screen_line >= e.screen_y - 8)
            e.pad_pos++;
          else
            e.screen_line++;
        }
      }
      gb_move_right(&g);
    }
    
    else if (c == KEY_LEFT || c == CTRL('j')) {
      gb_move_left(&g);
      if (gb_get_current(&g) == LK_ENTER) {
        if (e.screen_line <= 8 && e.pad_pos > 0)
          e.pad_pos--;
        else
          e.screen_line--;
      }
    }

    else if (c == 263 || c == CTRL('u')) {
      e.should_refresh = gb_backspace(&g);
      draw_line_area(&e,&g);
    }

    else if (c == CTRL('s')) {
      gb_write_to_file(&g);
    }
    
    else if (c == CTRL('o')) {
      if (state == TEXT) {
        gb_jump(&g);
        g.buf[g.front] = '\n';
        g.size++;
        g.front++;
        g.point++;
        if (e.screen_line >= e.screen_y - 8)
          e.pad_pos++;
        else
          e.screen_line++;
        g.lin += 1;
        g.col = 0;
        g.maxlines++;
        gb_refresh_line_width(&g);
      } else {
        open_open_file(&g, &e);
        e.screen_line = 0;
        state = TEXT;
      }
     	
      draw_line_area(&e, &g);
      e.should_refresh = true;
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(&g);
      g.buf[g.front] = c;
      g.size++;
      g.front++;
      g.point++;
      g.col += 1;
      gb_refresh_line_width(&g);
      e.should_refresh = true;
    }

    else if (c == CTRL('r')) {
      if (state == TEXT) {
        state = OPEN;
        e.should_refresh = true;
      }
      else if (state == OPEN) {
        state = TEXT;
        e.should_refresh = true;
      }
    }

    // else if (c == 127) {
    // }
    #if SHOW_BAR
    print_status_line(statArea, &g, &e, c);
    wrefresh(statArea);
    #endif // SHOW_BAR
    if (state == TEXT && e.should_refresh) {
      print_text_area(&e, &g);
    }
    refresh();
    wmove(e.textPad, g.lin, g.col);
    prefresh(e.linePad, e.pad_pos, 0, 0, 0, e.screen_y - 2, 4);
    prefresh(e.textPad, e.pad_pos, 0, 0, 4, e.screen_y - 2, e.screen_x - 1);

    if (state == OPEN && e.should_refresh) {
      wclear(e.popupArea);
      print_files(&e);
      wrefresh(e.popupArea);
    }
  } while ((c = wgetch(e.textPad)) != STR_Q);
  
  clear();
  endwin();
  return 0;
}
