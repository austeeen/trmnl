#include <ncurses.h>
#include <iostream>

// g++ -lncurses game.cpp

int main()
{
    initscr();

    int x = 0, y = 0;
    move(x, y); // move cursor on the screen, not shown until refreshed
    refresh(); // erase everything on screen
    // ^^ equivalent to wrefresh(stdscr);

    int c = getch(); // read character from stdscr

    noecho(); // does not print the character to screen
    cbreak(); // get one char at a time -- otherwise return all chars following a new line break
    nodelay(stdscr, TRUE); // non-blocking, without this it will wait until a char is given
    keypad(stdscr, TRUE); // allow special keys like arrows/delete/ctrl/shift/etc

    chtype new_char; // asci value of the character and some video stuff like colors
    addch(new_char); // output a character

    const char *ch;
    addstr(ch); // output a character string
    const char *fmt;
    printw(fmt, ch); // same as printf

    /*
    A_NORMAL        Normal display (no highlight)
    A_STANDOUT      Best highlighting mode of the terminal.
    A_UNDERLINE     Underlining
    A_REVERSE       Reverse video
    A_BLINK         Blinking
    A_DIM           Half bright
    A_BOLD          Extra bright or bold
    A_PROTECT       Protected mode
    A_INVIS         Invisible or blank mode
    A_ALTCHARSET    Alternate character set
    A_CHARTEXT      Bit-mask to extract a character
    COLOR_PAIR(n)   Color-pair number n
    */

    attron(A_STANDOUT); // set attribute on for all subsequent outputs
    addch('A' | A_UNDERLINE | A_BLINK); // or together video display options

    start_color(); // must use to use colors
    // color values are stored in COLORS and COLOR_PAIRS

    /*
    COLOR_BLACK
    COLOR_RED
    COLOR_GREEN
    COLOR_YELLOW
    COLOR_BLUE
    COLOR_MAGENTA
    COLOR_CYAN
    COLOR_WHITE
    */

    // a color pair is a numeric id, foreground color, and background color:
    init_pair(1, 2, 0); // color id 1 has fg color 2 and bg color 0

    /*  line graphics that can be used in addch and addstr
    ACS_BLOCK               solid square block
    ACS_BOARD               board of squares
    ACS_BTEE                bottom tee
    ACS_BULLET              bullet
    ACS_CKBOARD             checker board (stipple)
    ACS_DARROW              arrow pointing down
    ACS_DEGREE              degree symbol
    ACS_DIAMOND             diamond
    ACS_GEQUAL              greater-than-or-equal-to
    ACS_HLINE               horizontal line
    ACS_LANTERN             lantern symbol
    ACS_LARROW              arrow pointing left
    ACS_LEQUAL              less-than-or-equal-to
    ACS_LLCORNER            lower left-hand corner
    ACS_LRCORNER            lower right-hand corner
    ACS_LTEE                left tee
    ACS_NEQUAL              not-equal
    ACS_PI                  greek pi
    ACS_PLMINUS             plus/minus
    ACS_PLUS                plus
    ACS_RARROW              arrow pointing right
    ACS_RTEE                right tee
    ACS_S1                  scan line 1
    ACS_S3                  scan line 3
    ACS_S7                  scan line 7
    ACS_S9                  scan line 9
    ACS_STERLING            pound-sterling symbol
    ACS_TTEE                top tee
    ACS_UARROW              arrow pointing up
    ACS_ULCORNER            upper left-hand corner
    ACS_URCORNER            upper right-hand corner
    ACS_VLINE               vertical line
    */

    border(0, 0, 0, 0, 0, 0, 0, 0); // add basic borders to stdscr window

    int ncolumns = 10;
    hline(ACS_HLINE, ncolumns); // add horizontal line across 10 columns

    endwin(); // must be called to restore terminal settings
}
