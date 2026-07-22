# AGENTS.md - working on JWM in this repo

JWM (Joe's Window Manager) - lightweight X11 window manager in plain C.
Upstream: https://github.com/joewing/jwm (this fork: branch `devel`).
See CODEMAP.md for the module map and architecture conventions.

## Current focus

Adding proper multi-monitor support. Today JWM only snapshots Xinerama
geometry once at StartupScreens() (src/screen.c); there is no XRandR
awareness, no hotplug/layout-change handling, and no per-monitor
identity (names, primary output). The owner tests changes live on a
real multi-display desktop running this JWM.

## Build

    ./autogen.sh          # only after fresh clone or configure.ac changes
    ./configure           # add --enable-debug for Debug()/memory checks
    make -j$(nproc)       # binary: src/jwm

- Autoconf only (no automake): `Makefile.in` is hand-maintained; adding
  a new src/foo.c means editing src/Makefile.in by hand.
- New optional X libraries follow the configure.ac pattern: check lib,
  add `AC_DEFINE(USE_FOO)`, guard code with `#ifdef USE_FOO`, include
  the header in src/jwm.h, and report in the configure summary.
- `./src/jwm -p` parses a config and exits: use it to validate jwmrc
  changes. `-f <file>` selects an alternate config.

## Testing

No unit tests. Verification ladder:

1. Compile clean (fix all warnings; the codebase builds warning-free).
2. `./src/jwm -p` for config parsing changes.
3. Xephyr sandbox - this emulates multi-head safely and is the default
   way to test multi-monitor logic without touching the live session:

       Xephyr :2 +xinerama -screen 1280x800 -screen 1024x768 &
       DISPLAY=:2 ./src/jwm -f example.jwmrc

4. Live hot-swap on the real desktop: a plain `jwm -restart` re-runs
   startup in the SAME process and does NOT load a new binary. To swap
   in a fresh build without ending the X session, use an Exit action
   whose command is the new binary - JWM execl()s it in place on exit:

       <Exit label="Swap JWM" confirm="false">/data/gpertea/work/jwm/src/jwm</Exit>

   (Add that to the root menu in ~/.jwmrc, then Exit via the menu.)
   Keep a fallback terminal or `ssh` session open when testing live.

## Code style (upstream is strict; PRs are expected to match)

- 3-space indentation, no tabs; lines <= 79 columns.
- C89-style: declarations at the top of the block, `/* */` comments.
- Doxygen `/** */` headers on every file and public function.
- Xlib calls go through the `JX*` macros from src/jxlib.h, never raw
  `XFoo()`.
- Memory via `Allocate`/`Release` (src/debug.h), never malloc/free.
- Module lifecycle: Initialize/Startup/Shutdown/Destroy - see CODEMAP.md.
  Order matters; register new modules in main.c in all four places.
- User-visible strings wrapped in `_()` for gettext (po/).
- Optional features always compile-gated with `#ifdef USE_*` so JWM
  still builds with the feature's library absent.
- ASCII only in code and comments.

## Git

- Branch `devel`; upstream PRs go to joewing/jwm master, squashed into
  logical commits, buildable at every commit (CONTRIBUTING.md).
- No Co-Authored-By/AI attribution trailers in commit messages.

## Code navigation

The repo is indexed in the codebase-memory graph (project
`data-gpertea-work-jwm`): use search_graph / trace_path /
get_code_snippet for symbol lookup and call-chain tracing before
falling back to grep.
