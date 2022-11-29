#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <ncurses.h>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

// g++ mines.cpp -lncurses

const std::string logo =
    "  //| //||  ||  ||\\\\  |  ||---||"
    " // |// ||  ||  || \\\\ |  ||-||  "
    "//      ||  ||  ||  \\\\|  ||---||";

const int logo_w = 32;
const int logo_h = 3;
const int logo_p = logo_w * 2 + logo_h * 2;

const int UP = 0;
const int DOWN = 1;
const int LEFT = 2;
const int RIGHT = 3;

const char PLYR_CHAR = 'X';

struct proximity {
    const int bit, chr, dist;
    bool check(const int dx, const int dy) const {
        return dist ? dx <= dist && dy <= dist : true;
    }
};

// not all of these are supported by terminals... regular chars may be better
// 4 proximity types
const int NUM_PRX = 4;
const proximity ALL_PRX[NUM_PRX] = {
    {0x1 << 1, A_BLINK,    5},
    {0x1 << 2, A_STANDOUT, 10},
    {0x1 << 3, A_BOLD,     20},
    {0x1 << 0, A_NORMAL,   0},
};
const int NOPRX = 3;

// points per mine
const int PPM = 25;
// point modifier
const int PT_MOD = 10;

class msgbox
{
public:
    msgbox(const char* msg_): msg(msg_)
    {
        w = strlen(msg_) + 2;
        h = 3;
        x = (COLS / 2) - (w / 2);
        y = (LINES / 2) - (h / 2);
        wn = newwin(h, w, y, x);
        box(wn, ACS_VLINE, ACS_HLINE);
        wmove(wn, 1, 1);
        waddstr(wn, msg);
        wrefresh(wn);
    }

    ~msgbox() {
        delwin(wn);
    }

private:
    WINDOW *wn;
    int x, y, w, h;
    const char *msg;
};

struct mine
{
    int x, y;
    bool isdefused;

    mine(): isdefused(false)
    {}

    int create(int seed, int fw, int fh)
    {
        srand(seed);
        x = rand() % (fw - 2) + 1;
        y = rand() % (fh - 2) + 1;
        return seed * x * y;
    }

    int check(int px, int py)
    {
        if (isdefused) {
            return 0;
        }
        for (int i = 0; i < NUM_PRX; i ++) {
            if (ALL_PRX[i].check(abs(px - x), abs(py - y))) {
                return ALL_PRX[i].bit;
            }
        }
        return 0;
    }

    bool trydefuse(int px, int py)
    {
        if (isdefused) {
            return false;
        }
        isdefused = (px == x && py == y);
        return isdefused;
    }
};

struct player
{
    int x, y, trail;

    player(): trail(ALL_PRX[NOPRX].chr) {}

    void reset(int _x, int _y) {
        x = _x;
        y = _y;
        trail = ALL_PRX[NOPRX].chr;
    }

    void update(WINDOW * fld) {
        wmove(fld, y, x);
    }

    void lateupdate(int mdst) {
        for (int i = 0; i < NUM_PRX; i ++) {
            if (mdst & ALL_PRX[i].bit) {
                trail = ALL_PRX[i].chr;
                break;
            }
        }
    }

    void draw(WINDOW * fld) {
        waddch(fld, PLYR_CHAR | trail);
    }
};

class MineField
{
public:
    MineField(int fw, int fh): hide_target(true), fw(fw), fh(fh)
    {
        fld = newwin(fh, fw, 2, 1);
        sx = fw / 2;
        sy = fh / 2;
    }

    ~MineField() {
        delete[] mines;
        delwin(fld);
    }

    void create()
    {
        int seed = time(NULL);
        srand(seed);
        tmines = rand() % 4 + 1;
        seed *= tmines;
        mines = new mine[tmines];
        for (int i = 0; i < tmines; i ++) {
            seed = mines[i].create(seed, fw, fh);
        }
        resetplayer();
    }

    void resetplayer() {
        plyr.reset(sx, sy);
        setcursor();
    }

    void setcursor() {
        wmove(fld, plyr.y, plyr.x);
        wrefresh(fld);
    }

    void cleartracks() {
        wclear(fld);
    }

    void drawborder() {
        box(fld, ACS_VLINE, ACS_HLINE);
    }

    void status(int &nmines_, int &tmines_) {
        nmines_ = nmines;
        tmines_ = tmines;
    }

    int move(int dir)
    {
        switch(dir) {
            case UP: if (plyr.y > 1) plyr.y --; break;
            case DOWN: if (plyr.y < fh - 2) plyr.y ++; break;
            case LEFT: if (plyr.x > 1) plyr.x --; break;
            case RIGHT:  if (plyr.x < fw - 2) plyr.x ++; break;
            default: return 0;
        }
        update();
        return 1;
    }

    bool fire() {
        bool hit = false;
        for (int i = 0; i < tmines; i ++) {
            if (mines[i].trydefuse(plyr.x, plyr.y)) {
                nmines ++;
                hit = true;
                flash();
            }
        }
        return hit;
    }

    void toggle_target() {
        hide_target = !hide_target;
        if (!hide_target) {
            for (int i = 0; i < tmines; i ++) {
                mvwaddch(fld, mines[i].y, mines[i].x, ACS_DIAMOND | A_DIM);
            }
        } else {
            for (int i = 0; i < tmines; i ++) {
                mvwaddch(fld, mines[i].y, mines[i].x, ' ');
            }
        }
        setcursor();
    }

    void update() {
        plyr.update(fld);
        int mdist = 0;
        for (int i = 0; i < tmines; i ++) {
            mdist |= mines[i].check(plyr.x, plyr.y);
        }
        plyr.lateupdate(mdist);
    }

    void draw() {
        plyr.draw(fld);
        wrefresh(fld);
    }

private:
    WINDOW * fld;
    bool hide_target;
    int nmines, tmines, sx, sy, fw, fh;
    player plyr;
    mine * mines;
};

struct banner
{
    double score, pts;
    int fld_area;
    int mines_defused, total_mines;
    int steps, cur_lvl;

    const char* bnr_txt = "score[%d] mines[%d:%d] steps[%d] lvl[%d]";
    WINDOW *bnr;

    banner(int area):
        score(0.0),
        pts(0.0),
        fld_area(area),
        mines_defused(0),
        total_mines(0),
        steps(0),
        cur_lvl(0)
    {
        bnr = newwin(1, COLS - 2, 1, 1);
    }

    ~banner() {
        delwin(bnr);
    }

    void updatepts() {
        if (steps) {
            double r = fld_area / steps;
            int ppm = mines_defused * (PPM + (PT_MOD * cur_lvl));
            pts = ppm * r;
        }
    }

    void updatemines(int _defused, int _total) {
        mines_defused = _defused;
        total_mines = _total;
        updatepts();
    }

    void updatesteps(int _steps) {
        steps = _steps;
    }

    void newlevel(int _lvl, int _tmines) {
        cur_lvl = _lvl;
        total_mines = _tmines;
        steps = 0;
        mines_defused = 0;
        score += pts;
        pts = 0;
        wclear(bnr);
    }

    void draw() {
        int rscore = (int) score + pts;
        mvwprintw(bnr, 0, 0, bnr_txt, rscore, mines_defused, total_mines, steps, cur_lvl);
        wrefresh(bnr);
    }
};

struct hud
{
    int fld_area;

    const char* hud_txt = "[wasd] move | [f]ire | [q]uit | [r]eset | [c]heat";
    WINDOW *hd;

    hud() { hd = newwin(1, COLS - 2, LINES - 2, 1); }
    ~hud() { delwin(hd); }

    void draw() {
        mvwaddstr(hd, 0, 0, hud_txt);
        wrefresh(hd);
    }
};

class MinesGame
{
public:
    MinesGame():
        fldw(COLS - 2),
        fldh(LINES - 4),
        flda(fldw * fldh),
        lvl(0),
        steps(0),
        nmines(0),
        tmines(0),
        hd(new hud())
    {}

    ~MinesGame() {
        delete mf;
        delete bnr;
        delete hd;
    }

    void splash() {
        int sx = COLS / 2 - logo_w / 2;
        int sy = LINES / 3 - logo_h / 2;
        int j = logo.size();
        int x1, x2, y1, y2;
        // printw("border: %d, area: %d, loops: %d", logo_p, logo.size(), logo.size() / 2);
        for (int i = 0; i <= logo.size() / 2; i ++) {
            j--;

            x1 = i % logo_w;
            y1 = i / logo_w;
            mvaddch(sy + y1, sx + x1, logo[i]);

            x2 = j % logo_w;
            y2 = j / logo_w;
            mvaddch(sy + y2, sx + x2, logo[j]);

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            refresh();
        }
        move(0, 0);
        getch();
    }

    void play() {
        refresh();
        newlevel();
        bool running = true;
        while (running)
        {
            mf->setcursor();
            switch (getch()) {
                case 'w': { steps += mf->move(UP); break; }
                case 's': { steps += mf->move(DOWN); break; }
                case 'a': { steps += mf->move(LEFT); break; }
                case 'd': { steps += mf->move(RIGHT); break; }
                case 'f': {
                    if(mf->fire()) {
                        mf->status(nmines, tmines);
                        bnr->updatemines(nmines, tmines);
                    }
                    break;
                }
                case 'c': { mf->toggle_target(); break; }
                case 'r': {
                    mf->resetplayer();
                    mf->cleartracks();
                    mf->drawborder();
                    break;
                }
                case 'q': { running = false; break; }
            }
            update();
            draw();
            if (nmines == tmines) {
                nextlevel();
            }
        }
    }

    void newgame() {
        border(0, 0, 0, 0, 0, 0, 0, 0);
        move(0, (int) COLS / 2);
        addstr(":|mines|:");
        bnr = new banner(flda);
        hd->draw();
    }

    void nextlevel() {
        msgbox mb("field clear");
        getch();
        delete mf;
        lvl++;
        newlevel();
    }

    void newlevel() {
        mf = new MineField(fldw, fldh);
        mf->create();
        steps = 0;
        mf->status(nmines, tmines);
        bnr->newlevel(lvl, tmines);
        bnr->draw();
        mf->drawborder();
        mf->resetplayer();
        refresh();
    }

    void update() {
        mf->update();
        bnr->updatesteps(steps);
        bnr->updatepts();
    }

    void draw() {
        mf->draw();
        hd->draw();
        bnr->draw();
        refresh();
    }

private:
    int fldw, fldh, flda;
    int lvl, steps, nmines, tmines;
    MineField *mf;
    banner* bnr;
    hud* hd;
};

int main()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    MinesGame game;
    game.splash();
    game.newgame();
    game.play();

    endwin();
}
