import curses
import dataclasses
import random
import time


@dataclasses.dataclass
class Vec:
    x: int = 0
    y: int = 0

    def clear(self):
        self.x = 0
        self.y = 0

    @staticmethod
    def _as_vec(other) -> 'Vec':
        if isinstance(other, Vec):
            return other
        elif isinstance(other, tuple):
            return Vec(*other)
        else:
            raise ValueError(f"cannot operate on non Vec/tuple type: {type(other)}")

    def __add__(self, other):
        other = self._as_vec(other)
        self.x += other.x
        self.y += other.y
        return self

    def __sub__(self, other):
        other = self._as_vec(other)
        self.x -= other.x
        self.y -= other.y
        return self

    def __truediv__(self, other):
        other = self._as_vec(other)
        self.x /= other.x
        self.y /= other.y
        return self

    def __eq__(self, other) -> bool:
        other = self._as_vec(other)
        return self.x == other.x and self.y == other.y

    def __ge__(self, other) -> bool:
        if isinstance(other, int):
            return self.x >= other and self.y >= other
        other = self._as_vec(other)
        return self.x >= other.x and self.y >= other.y

    def __abs__(self):
        return Vec(abs(self.x), abs(self.y))


@dataclasses.dataclass
class Proximity:
    bit: int
    attr: int
    distance: int

    def check(self, p: Vec) -> bool:
        return p.x <= self.distance and p.y <= self.distance


@dataclasses.dataclass
class Score:
    value: int = 0
    points: int = 0
    mines_defused: int = 0
    total_mines: int = 0
    steps: int = 0
    cur_level: int = 0
    field_area: int = 0
    text: str = "score[{0}] mines[{1}:{2}] steps[{3}] lvl[{4}]"

    def reset(self):
        self.value = 0
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

    def update_score(self):
        self.value += self.points
        self.points = 0

    def as_text(self) -> str:
        return self.text.format(
                self.value, self.mines_defused, self.total_mines, self.steps, self.cur_level
            )


logo = (
    "  //| //||  ||  ||\\\\  |  ||---||"
    " // |// ||  ||  || \\\\ |  ||-||  "
    "//      ||  ||  ||  \\\\|  ||---||"
)

logo_w = 32
logo_h = 3
logo_p = logo_w * 2 + logo_h * 2


def play_splashscreen(screen: "curses.window"):
    c = Vec(int(curses.COLS / 2 - logo_w / 2), int(curses.LINES / 3 - logo_h / 2))
    j = len(logo)

    for i in range(int(len(logo) / 2)):
        j -= 1
        a = Vec(i % logo_w, int(i / logo_w))
        b = Vec(j % logo_w, int(j / logo_w))
        screen.addch(c.y + a.y, c.x + a.x, logo[i])
        screen.addch(c.y + b.y, c.x + b.x, logo[j])

        time.sleep(0.05)
        screen.refresh()

    screen.move(0, 0)
    screen.getch()


UP = 0
DOWN = 1
LEFT = 2
RIGHT = 3

PLYR_CHAR = "X"


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
    pos = Vec(int((curses.COLS / 2) - (cols / 2)), int((curses.LINES / 2) - (lines / 2)))

    # create the window and set its box chars
    win = curses.newwin(lines, cols, pos.y, pos.x)
    win.box("|", "-")

    # write the string at (1, 1) from the box's origin
    win.addstr(1, 1, msg)

    # update the screen
    win.refresh()
    win.getch()


class GameObject:
    def __init__(self, x_pos: int = 0, y_pos: int = 0):
        self.pos = Vec(x_pos, y_pos)

    @property
    def x(self):
        return self.pos.x

    @property
    def y(self):
        return self.pos.y


class Mine(GameObject):
    def __init__(self, field_dim: Vec):
        random.seed()
        super(Mine, self).__init__(random.randrange(1, field_dim.x - 2),
                                   random.randrange(1, field_dim.y - 2))
        self.defused = False

    def check(self, pos: Vec) -> int:
        if self.defused:
            return 0

        for prox in ALL_PRX:
            if prox.check(abs(pos - self.pos)):
                return prox.bit

        return 0

    def try_defusing(self, p: Vec) -> bool:
        self.defused = p == self.pos
        return self.defused


class Player(GameObject):
    def __init__(self):
        super(Player, self).__init__()
        self.trail = ALL_PRX[-1].attr
        self.steps = 0

    def reset(self):
        self.pos.clear()
        self.trail = ALL_PRX[-1].attr

    def set_position(self, xy: Vec):
        self.pos = xy

    def move(self, xy: Vec):
        self.pos += xy
        self.steps += abs(xy.x) + abs(xy.y)

    def update(self, field: "curses.window"):
        field.move(self.pos.y, self.pos.x)

    def late_update(self, dst: int):
        """ set the trail to the closest proximity bit """
        for prox in ALL_PRX:
            # if distance integer matches proximity bit, switch the trail char
            if dst & prox.bit:
                self.trail = prox.attr
                break

    def draw(self, field: "curses.window"):
        field.addch(PLYR_CHAR, self.trail)


class MineField:
    def __init__(self, field_dim: Vec):
        self.dim = field_dim
        self.center = self.dim / 2
        self.field = curses.newwin(self.dim.y, self.dim.x, 2, 1)
        self.hide_target = True
        self.total_mines = 0
        self.num_mines = 0
        self.mines: list["Mine"] = []
        self.player = Player()

    @property
    def area(self) -> int:
        return self.dim.x * self.dim.y

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
        self.mines = [Mine(self.dim) for _ in range(self.total_mines)]
        self.reset_player()

    def reset_player(self):
        self.player.set_position(self.center)
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

    def fire(self):
        for mine in self.mines:
            if mine.try_defusing(self.player.pos):
                self.num_mines += 1
                curses.flash()

    def toggle_target(self):
        self.hide_target = not self.hide_target

        if not self.hide_target:
            for mine in self.mines:
                self.field.addch(mine.y, mine.x, curses.ACS_DIAMOND, curses.A_DIM)
        else:
            for mine in self.mines:
                self.field.addch(mine.y, mine.x, " ")
        self.set_cursor()

    def process_input(self, in_chr: chr):
        if in_chr == "w" and self.player.y > 1:
            self.player.move(Vec(0, -1))
        elif in_chr == "s" and self.player.y < self.dim.y - 2:
            self.player.move(Vec(0, 1))
        elif in_chr == "a" and self.player.x > 1:
            self.player.move(Vec(-1, 0))
        elif in_chr == "d" and self.player.x < self.dim.x - 2:
            self.player.move(Vec(1, 0))
        elif in_chr == "f":
            self.fire()
        elif in_chr == "c":
            self.toggle_target()
        elif in_chr == "r":
            self.reset_player()
            self.clear_tracks()
            self.draw_border()

    def update(self):
        self.player.update(self.field)
        m_dist = 0
        for mine in self.mines:
            m_dist |= mine.check(self.player.pos)
        self.player.late_update(m_dist)

    def draw(self):
        self.player.draw(self.field)
        self.field.refresh()


class ScoreBanner:
    def __init__(self, area: int):
        self.score = Score(field_area=area)
        self.win = curses.newwin(1, curses.COLS - 2, 1, 1)
        self.text = "score[{0}] mines[{1}:{2}] steps[{3}] lvl[{4}]"

    def new_level(self, level_num: int, total_mines: int):
        self.score.cur_level = level_num
        self.score.total_mines = total_mines
        self.score.steps = 0
        self.score.mines_defused = 0

        # update the score on generating a new level
        self.score.update_score()

        self.win.clear()

    def draw(self):
        self.win.addstr(0, 0, self.score.as_text())
        self.win.refresh()


class HUD:
    def __init__(self):
        self.text = "[wasd] move | [f]ire | [q]uit | [r]eset | [c]heat"
        self.win = curses.newwin(1, curses.COLS - 2, curses.LINES - 2, 1)

    def draw(self):
        self.win.addstr(0, 0, self.text)
        self.win.refresh()


class MinesGame:
    def __init__(self, screen: "curses.window"):
        self.screen = screen
        self.steps = 0
        self.num_mines = 0
        self.total_mines = 0
        self.hud = HUD()
        self.mine_field = MineField(Vec(curses.COLS - 2,  curses.LINES - 4))
        self.banner = ScoreBanner(self.mine_field.area)
        self.frames = 0
        self.running = False

    def play(self) -> int:
        self.screen.refresh()
        self.new_level(0)

        self.running = True
        self.frames = 0

        while self.running:
            self.mine_field.set_cursor()
            self.frames += 1

            raw_chr = self.screen.getch()
            in_chr = 0

            if raw_chr < 256:
                in_chr = chr(raw_chr)

            if isinstance(chr, in_chr):
                self.handle_input(in_chr)
            self.update()
            self.draw()

            if self.num_mines == self.total_mines:
                msg_box("Field Clear")
                self.new_level(self.banner.score.cur_level + 1)

        return int(self.banner.score)

    def new_game(self):
        self.screen.border(0, 0, 0, 0, 0, 0, 0, 0)
        self.screen.move(0, int(curses.COLS / 2))
        self.screen.addstr(":|mines|:")

    def new_level(self, level_num: int):
        self.mine_field.reset()
        self.mine_field.create()
        self.steps = 0
        self.num_mines, self.total_mines = self.mine_field.get_status()
        self.banner.new_level(level_num, self.total_mines)
        self.mine_field.draw_border()
        self.mine_field.reset_player()
        self.banner.draw()
        self.hud.draw()
        self.screen.refresh()

    def handle_input(self, in_chr: chr):
        if in_chr == "q":
            self.running = False
        else:
            self.mine_field.process_input(in_chr)

    def update(self):
        self.mine_field.update()

        self.num_mines, self.total_mines = self.mine_field.get_status()
        self.banner.update_mines(self.num_mines, self.total_mines)
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
