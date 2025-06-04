// TODO
//
// [X] scroll linePad 
// [ ] move cursor to textPad at start
// [ ] dont show cursor in popup
// [ ] make popup nicer
// [ ] change controls
// [ ] create readme
// [ ] make statusLine optional
// [ ] cleanup
//
// [ ] increase buffer when necessary
// [ ] increase pad size when necessary
// 

#include "gap_buffer.c"

/************************* #EDITOR ******************************/

typedef struct {
  u16 screen_y, screen_x;  // size in cols and rows of window
  u8 screen_line; // line of the point on the screen
  u32 pad_pos; // offset from top of pad to top of screen
} Editor;

int read_file(GapBuffer *g, char* filename) {

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

void print_text_area(WINDOW *textArea, GapBuffer *g) {
  wmove(textArea, 0, 0);
  wclear(textArea);
  u32 point = g->point;
  for (g->point = 0; g->point < g->size; g->point++) {
    waddch(textArea, gb_get_current(g));
  }
  g->point = point;
}

void draw_line_area(GapBuffer *g, WINDOW *lineArea) {
  wclear(lineArea);
  for (int i = 0; i < g->maxlines; i++) {
    mvwprintw(lineArea, i - 1, 0, "%d", i);
  }
  wrefresh(lineArea);
}

int print_status_line(WINDOW *statArea, GapBuffer *g, Editor *e, int c, u8 chosen_file) {
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
  //wprintw(statArea, "cf: %d, ", chosen_file);
  //wprintw(statArea, "prev: %d, ", gb_prev_line_width(g));
  wprintw(statArea, "\t\t\t");
}

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

void print_files(WINDOW *popupArea, char **files, u16 files_len, u8 chosen_file) {
  int line = 2;
  for (int i = 0; i < files_len; i++, line++) {
    if (i == chosen_file)
      wattrset(popupArea, COLOR_PAIR(4));
    else
      wattrset(popupArea, COLOR_PAIR(2));

    mvwprintw(popupArea, line, 3, "%s", files[i]);
  }
}

void open_move_up(u8 *chosen_file, u16 files_len, bool *changed) {
  *chosen_file = (*chosen_file + files_len - 1) % files_len;
  *changed = true;
}

void open_move_down(u8 *chosen_file, u16 files_len, bool *changed) {
  *chosen_file = (*chosen_file + 1) % files_len;
  *changed = true;
}

void open_open_file(GapBuffer *g, char **files, u8 chosen_file) {
  gb_clear_buffer(g);
  read_file(g, files[chosen_file]);
}

typedef enum {
  TEXT, OPEN
} State;


/************************** #MAIN ********************************/

int main(int argc, char **argv) {

  initscr();
  start_color();
  atexit((void*)endwin);

  struct timeval tp;
  u16 millis = 1000;
  u16 old_millis = 1000;
  u16 delta = 1000;
  
  char *path = get_path();
  char **files = NULL;
  int files_len;
  u8 chosen_file = 0;

  get_file_system(path, &files, &files_len);

  WINDOW *lineArea;
  WINDOW *textPad;
  WINDOW *statArea;
  WINDOW *popupArea;

  Editor e;
  State state = TEXT;
  GapBuffer g;
  gb_init(&g, INIT_CAP);
 
  g.buf = calloc(g.cap, sizeof(char));
  getmaxyx(stdscr, e.screen_y, e.screen_x);
  e.screen_line = 0;
  lineArea = newpad(1000, 4);
  textPad = newpad(1000, e.screen_x - 4);
  statArea = newwin(1, e.screen_x, 0, 0);
  popupArea = newwin(5, 30, 10, 10);
  e.pad_pos = 0;
  
  wresize(popupArea, 30, 60);
  box(popupArea, ACS_VLINE, ACS_HLINE);
  mvwin(statArea, e.screen_y - 1, 0);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_WHITE, COLOR_RED);
  wattrset(textPad, COLOR_PAIR(1));
  wattrset(statArea, COLOR_PAIR(4));
  wbkgd(popupArea, COLOR_PAIR(2));

  raw();
  keypad(textPad, TRUE);
  noecho();
  
  if (argc > 1) {
    read_file(&g, argv[1]);
    draw_line_area(&g, lineArea);
    print_text_area(textPad, &g);
    refresh();
    prefresh(lineArea, e.pad_pos, 0, 0, 0, e.screen_y - 2, 4);
  }

  int c = - 1;
  bool changed;
  do {
    changed = false;
    // if (c == KEY_UP || c == CTRL('i')) {
    if (c == KEY_UP) {
      if (state == TEXT && gb_move_up(&g)) {
        if (e.screen_line <= 8 && e.pad_pos > 0)
          e.pad_pos--;
        else
          e.screen_line--;
      }
      else
        open_move_up(&chosen_file, files_len, &changed);
    }
    
    else if (c == LK_DOWN) {
      if (state == TEXT && gb_move_down(&g)) {
        if (e.screen_line >= e.screen_y - 8)
          e.pad_pos++;
        else
          e.screen_line++;
      }
      else
        open_move_down(&chosen_file, files_len, &changed);
    }

    else if (c == KEY_RIGHT) {
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
    
    else if (c == KEY_LEFT) {
      gb_move_left(&g);
      if (gb_get_current(&g) == LK_ENTER) {
        if (e.screen_line <= 8 && e.pad_pos > 0)
          e.pad_pos--;
        else
          e.screen_line--;
      }
    }

    else if (c == 263) {
      changed = gb_backspace(&g);
      draw_line_area(&g, lineArea);
    }

    else if (c == CTRL('s')) {
      gb_write_to_file(&g);
    }
    
    else if (c == LK_ENTER) {
      if (state == TEXT) {
        gb_jump(&g);
        g.buf[g.front] = '\n';
        g.size++;
        g.front++;
        g.point++;
        g.lin += 1;
        g.col = 0;
        g.maxlines++;
        gb_refresh_line_width(&g);
      } else {
        open_open_file(&g, files, chosen_file);
        e.screen_line = 0;
        state = TEXT;
      }
     	
      draw_line_area(&g, lineArea);
      changed = true;
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(&g);
      g.buf[g.front] = c;
      g.size++;
      g.front++;
      g.point++;
      g.col += 1;
      gb_refresh_line_width(&g);
      changed = true;
    }

    else if (c == CTRL('r')) {
      if (state == TEXT) {
        state = OPEN;
        changed = true;
      }
      else if (state == OPEN) {
        state = TEXT;
        changed = true;
      }
    }

    // else if (c == 127) {
    // }

    print_status_line(statArea, &g, &e, c, chosen_file);
    wrefresh(statArea);
    if (state == TEXT && changed) {
      print_text_area(textPad, &g);
    }
    refresh();
    wmove(textPad, g.lin, g.col);
    prefresh(lineArea, e.pad_pos, 0, 0, 0, e.screen_y - 2, 4);
    prefresh(textPad, e.pad_pos, 0, 0, 4, e.screen_y - 2, e.screen_x - 1);

    if (state == OPEN && changed) {
      wclear(popupArea);
      print_files(popupArea, files, files_len, chosen_file);
      wrefresh(popupArea);
    }
  } while ((c = wgetch(textPad)) != STR_Q);
  
  clear();
  endwin();
  return 0;
}
