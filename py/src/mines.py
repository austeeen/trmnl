import curses
import dataclasses
import random
import time


logo = \
    "  //| //||  ||  ||\\\\  |  ||---||" \
    " // |// ||  ||  || \\\\ |  ||-||  " \
    "//      ||  ||  ||  \\\\|  ||---||"

logo_w = 32
logo_h = 3
logo_p = logo_w * 2 + logo_h * 2


def play_splashscreen(screen: 'curses.window'):
    cx = int(curses.COLS / 2 - logo_w / 2)
    cy = int(curses.LINES / 3 - logo_h / 2)
    j = len(logo)

    for i in range(int(len(logo) / 2)):
        j -= 1

        x1 = i % logo_w
        y1 = i / logo_w

        x2 = j % logo_w
        y2 = j / logo_w

        screen.addch(int(cy + y1), int(cx + x1), logo[i])
        screen.addch(int(cy + y2), int(cx + x2), logo[j])

        time.sleep(0.05)
        screen.refresh()

    screen.move(0, 0)
    screen.getch()


UP = 0
DOWN = 1
LEFT = 2
RIGHT = 3

PLYR_CHAR = 'X'


@dataclasses.dataclass
class Proximity:
    bit: int
    attr: int
    distance: int

    def check(self, dx: int, dy: int) -> bool:
        return dx <= self.distance and dy <= self.distance


ALL_PRX = [
    Proximity(1 << 1, curses.A_BLINK, 5),
    Proximity(1 << 2, curses.A_STANDOUT, 10),
    Proximity(1 << 3, curses.A_BOLD, 20),
    Proximity(1 << 0, curses.A_NORMAL, 0),
]

# points per mine
PPM = 3

# point modifier
PT_MOD = 10


def msg_box(msg: str):
    # centered
    cols = len(msg) + 2
    lines = 3
    x = int((curses.COLS / 2) - (cols / 2))
    y = int((curses.LINES / 2) - (lines / 2))

    # create the window and set its box chars
    win = curses.newwin(lines, cols, y, x)
    win.box('|', '-')

    # write the string at (1, 1) from the box's origin
    win.addstr(1, 1, msg)

    # update the screen
    win.refresh()
    win.getch()


class Mine:
    def __init__(self, field_w: int, field_h: int):
        random.seed()
        self.x = random.randrange(1, field_w - 2)
        self.y = random.randrange(1, field_h - 2)
        self.defused = False

    def check(self, px: int, py: int) -> int:
        if self.defused:
            return 0

        for prox in ALL_PRX:
            if prox.check(abs(px - self.x), abs(py - self.y)):
                return prox.bit

        return 0

    def try_defusing(self, px: int, py: int) -> bool:
        self.defused = px == self.x and py == self.y
        return self.defused


class Player:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.trail = ALL_PRX[-1].attr

    def reset(self):
        self.x = 0
        self.y = 0
        self.trail = ALL_PRX[-1].attr

    def set_position(self, x: int, y: int):
        self.x = x
        self.y = y

    def update(self, field: 'curses.window'):
        field.move(self.y, self.x)

    def late_update(self, dst: int):
        for prox in ALL_PRX:
            # if distance integer matches proximity bit, switch the trail char
            if dst & prox.bit:
                self.trail = prox.attr
                break

    def draw(self, field: 'curses.window'):
        field.addch(PLYR_CHAR, self.trail)


class MineField:
    def __init__(self, field_w: int, field_h: int):
        self.field_w = field_w
        self.field_h = field_h
        self.hide_target = True
        self.field = curses.newwin(self.field_h, self.field_w, 2, 1)
        self.center_x = int(self.field_w / 2)
        self.center_y = int(self.field_h / 2)
        self.total_mines = 0
        self.num_mines = 0
        self.mines: list['Mine'] = []
        self.player = Player()

    def reset(self):
        self.hide_target = True
        self.total_mines = 0
        self.num_mines = 0
        self.mines.clear()
        self.player.reset()
        self.field.erase()

    def create(self):
        random.seed()
        self.total_mines = random.randrange(1, 4)
        self.mines = [Mine(self.field_w, self.field_h) for _ in range(self.total_mines)]
        self.reset_player()

    def reset_player(self):
        self.player.set_position(self.center_x, self.center_y)
        self.set_cursor()

    def set_cursor(self):
        self.field.move(self.player.y, self.player.x)
        self.field.refresh()

    def clear_tracks(self):
        self.field.clear()

    def draw_border(self):
        self.field.box("|", "-")

    def get_status(self) -> tuple[int, int]:
        return self.num_mines, self.total_mines

    def move_player(self, d: int) -> int:
        if d == UP and self.player.y > 1:
            self.player.y -= 1
        elif d == DOWN and self.player.y < self.field_h - 2:
            self.player.y += 1
        elif d == LEFT and self.player.x > 1:
            self.player.x -= 1
        elif d == RIGHT and self.player.x < self.field_w - 2:
            self.player.x += 1
        else:
            return 0
        self.update()
        return 1

    def fire(self) -> bool:
        hit = False
        for mine in self.mines:
            if mine.try_defusing(self.player.x, self.player.y):
                self.num_mines += 1
                hit = True
                curses.flash()
        return hit

    def toggle_target(self):
        self.hide_target = not self.hide_target

        if not self.hide_target:
            for mine in self.mines:
                self.field.addch(mine.y, mine.x, curses.ACS_DIAMOND, curses.A_DIM)
        else:
            for mine in self.mines:
                self.field.addch(mine.y, mine.x, ' ')
        self.set_cursor()

    def update(self):
        self.player.update(self.field)
        m_dist = 0
        for mine in self.mines:
            m_dist |= mine.check(self.player.x, self.player.y)
        self.player.late_update(m_dist)

    def draw(self):
        self.player.draw(self.field)
        self.field.refresh()


class Banner:
    def __init__(self, area: int):
        self.score = 0
        self.points = 0
        self.field_area = area
        self.mines_defused = 0
        self.total_mines = 0
        self.steps = 0
        self.cur_level = 0
        self.win = curses.newwin(1, curses.COLS - 2, 1, 1)
        self.text = "score[{0}] mines[{1}:{2}] steps[{3}] lvl[{4}]"

    def reset(self):
        self.score = 0
        self.points = 0
        self.mines_defused = 0
        self.total_mines = 0
        self.steps = 0
        self.cur_level = 0

    def update_mines(self, defused: int, total: int):
        self.mines_defused = defused
        self.total_mines = total
        self.update_points()

    def update_points(self):
        if self.steps:
            r = float(self.field_area / self.steps)
            ppm = float(self.mines_defused * PPM + (PT_MOD * self.cur_level))
            self.points = int(r * ppm)

    def new_level(self, level_num: int, total_mines: int):
        self.cur_level = level_num
        self.total_mines = total_mines
        self.steps = 0
        self.mines_defused = 0

        # update the score on generating a new level
        self.score += self.points
        self.points = 0

        self.win.clear()

    def draw(self):
        self.win.addstr(0, 0, self.text.format(
            self.score,
            self.mines_defused,
            self.total_mines,
            self.steps,
            self.cur_level))
        self.win.refresh()


class HUD:
    def __init__(self):
        self.text = "[wasd] move | [f]ire | [q]uit | [r]eset | [c]heat"
        self.win = curses.newwin(1, curses.COLS - 2, curses.LINES - 2, 1)

    def draw(self):
        self.win.addstr(0, 0, self.text)
        self.win.refresh()


class MinesGame:
    def __init__(self, screen: 'curses.window'):
        self.screen = screen
        self.field_w = curses.COLS - 2
        self.field_h = curses.LINES - 4
        self.field_a = self.field_h * self.field_w
        self.level = 0
        self.steps = 0
        self.num_mines = 0
        self.total_mines = 0
        self.hud = HUD()
        self.mine_field = MineField(self.field_w, self.field_h)
        self.banner = Banner(self.field_a)
        self.frames = 0

    def play(self) -> int:
        self.screen.refresh()
        self.new_level()

        running = True
        self.frames = 0

        while running:
            self.mine_field.set_cursor()
            self.frames += 1

            raw_chr = self.screen.getch()
            in_chr = 0

            if raw_chr < 256:
                in_chr = chr(raw_chr)

            if in_chr == 'w':
                self.steps += self.mine_field.move_player(UP)
            elif in_chr == 's':
                self.steps += self.mine_field.move_player(DOWN)
            elif in_chr == 'a':
                self.steps += self.mine_field.move_player(LEFT)
            elif in_chr == 'd':
                self.steps += self.mine_field.move_player(RIGHT)
            elif in_chr == 'f':
                if self.mine_field.fire():
                    self.num_mines, self.total_mines = \
                        self.mine_field.get_status()
                    self.banner.update_mines(self.num_mines, self.total_mines)
            elif in_chr == 'c':
                self.mine_field.toggle_target()
            elif in_chr == 'r':
                self.mine_field.reset_player()
                self.mine_field.clear_tracks()
                self.mine_field.draw_border()
            elif in_chr == 'q':
                running = False

            self.update()
            self.draw()

            if self.num_mines == self.total_mines:
                self.next_level()

        return int(self.banner.score)

    def new_game(self):
        self.screen.border(0, 0, 0, 0, 0, 0, 0, 0)
        self.screen.move(0, int(curses.COLS / 2))
        self.screen.addstr(":|mines|:")

    def next_level(self):
        msg_box("Field Clear")
        self.level += 1
        self.new_level()

    def new_level(self):
        self.mine_field.reset()
        self.mine_field.create()
        self.steps = 0
        self.num_mines, self.total_mines = self.mine_field.get_status()
        self.banner.new_level(self.level, self.total_mines)
        self.mine_field.draw_border()
        self.mine_field.reset_player()
        self.banner.draw()
        self.hud.draw()
        self.screen.refresh()

    def update(self):
        self.mine_field.update()
        self.banner.steps = self.steps
        self.banner.update_points()

    def draw(self):
        self.mine_field.draw()
        self.hud.draw()
        self.banner.draw()
        self.screen.refresh()


def exit_main(exit_str: str, err_str: str = ""):
    curses.endwin()
    print(exit_str)
    if err_str:
        print(err_str)


def main():
    import traceback

    screen = curses.initscr()
    curses.cbreak()
    curses.noecho()
    screen.keypad(True)

    game = MinesGame(screen)

    exit_str = ""
    err_str = ""
    end_score = 0
    try:
        play_splashscreen(screen)
        game.new_game()
        end_score = game.play()
    except Exception as e:
        exit_str = f"Caught exception: {str(e)}"
        err_str = traceback.format_exc()
    finally:
        curses.endwin()

    if not exit_str:
        if not end_score:
            end_score = int(game.banner.score)
        exit_str = f"Thanks for playing, your score was: {end_score}"

    exit_main(exit_str, err_str)
