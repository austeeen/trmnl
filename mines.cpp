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
    "  //| //||  ||  ||\\\\  |  ||---||  \\\\---||"
    " // |// ||  ||  || \\\\ |  ||-||       \\\\  "
    "//      ||  ||  ||  \\\\|  ||---||  ||---\\\\";
const int logo_w = 41;
const int logo_h = 3;
const int logo_p = logo_w * 2 + logo_h * 2;
const int DIST_NEAR = 10;
const int DIST_CLOSE = 5;

const int IS_FAR = 0x0;
const int IS_NEAR = 0x1;
const int IS_CLOSE = 0x2;


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

        if (abs(px - x) < DIST_CLOSE && abs(py - y) < DIST_CLOSE) {
            return IS_CLOSE;
        }
        else if (abs(px - x) < DIST_NEAR && abs(py - y) < DIST_NEAR) {
            return IS_NEAR;
        }
        return IS_FAR;
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


class MineField
{
public:
    MineField(): hide_target(true), fw(COLS - 2), fh(LINES - 3)
    {
        fld = newwin(fh, fw, 1, 1);
        sx = fw / 2;
        sy = fh / 2;
        mx = sx;
        my = sy;
        mchr = A_NORMAL;
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
    }

    void resetplayer() {
        mx = sx;
        my = sy;
        mchr = A_NORMAL;
        setplayer();
    }

    void setplayer() {
        wmove(fld, my, mx);
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

    void update() {
        waddch(fld, ACS_BLOCK | mchr);
        setplayer();
        int mdist = 0;
        for (int i = 0; i < tmines; i ++) {
            mdist |= mines[i].check(mx, my);
        }
        if (mdist & IS_CLOSE) {
            mchr = A_BLINK;
        } else if (mdist & IS_NEAR) {
            mchr = A_BOLD;
        } else {
            mchr = A_NORMAL;
        }
    }

    int move_u() {
        if (my > 1) {
            my --;
            update();
            return 1;
        }
        return 0;
    }

    int move_d() {
        if (my < fh - 2) {
            my ++;
            update();
            return 1;
        }
        return 0;
    }

    int move_l() {
        if (mx > 1) {
            mx --;
            update();
            return 1;
        }
        return 0;
    }

    int move_r() {
        if (mx < fw - 2) {
            mx ++;
            update();
            return 1;
        }
        return 0;
    }

    bool fire() {
        bool hit = false;
        for (int i = 0; i < tmines; i ++) {
            if (mines[i].trydefuse(mx, my)) {
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
        setplayer();
    }

private:
    WINDOW * fld;
    bool hide_target;
    int nmines, tmines, sx, sy, mx, my, mchr, fw, fh;
    mine * mines;
};


struct hud
{
    hud(): score(0), steps(0), mines_defused(0), total_mines(0)
    {
        hd = newwin(1, COLS - 2, LINES - 2, 1);
    }

    ~hud() {
        delwin(hd);
    }

    void setscore() {
        if (steps) {
            score += (mines_defused * 250) / steps;
        }
    }

    void update(int _steps, int _defused, int _total)
    {
        steps = _steps;
        mines_defused = _defused;
        total_mines = _total;
    }

    void reset() {
        wclear(hd);
    }

    void draw()
    {
        mvwprintw(hd, 0, 0, hud_txt, score, mines_defused, total_mines, steps);
        wrefresh(hd);
    }

    int x, y, w, h, score, steps, mines_defused, total_mines;
    const char* hud_txt = "score[%d] (%d:%d) %d";
    WINDOW *hd;
};

class MinesGame
{
public:
    MinesGame(): score(0), steps(0), nmines(0), tmines(0)
    {}

    ~MinesGame() {
        delete mf;
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

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
            mf->setplayer();
            switch (getch()) {
                case 'w': { steps += mf->move_u(); break; }
                case 's': { steps += mf->move_d(); break; }
                case 'a': { steps += mf->move_l(); break; }
                case 'd': { steps += mf->move_r(); break; }
                case 'f': {
                    if(mf->fire()) {
                        mf->status(nmines, tmines);
                        hd->update(steps, nmines, tmines);
                        hd->setscore();
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
            hd->update(steps, nmines, tmines);
            hd->draw();
            refresh();
            if (nmines == tmines) {
                nextlevel();
            }
        }
    }

    void newgame() {
        border(0, 0, 0, 0, 0, 0, 0, 0);
        move(0, (int) COLS / 3);
        addstr(":|mines|:");
        hd = new hud();
    }

    void nextlevel() {
        msgbox mb("field clear");
        getch();
        delete mf;
        newlevel();
    }

    void newlevel() {
        mf = new MineField();
        mf->create();
        steps = 0;
        mf->status(nmines, tmines);
        hd->update(steps, nmines, tmines);
        hd->reset();
        hd->draw();
        mf->drawborder();
        mf->setplayer();
        refresh();
    }

private:
    int score, steps, nmines, tmines;
    MineField *mf;
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
