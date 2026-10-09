JWM (Joe's Window Manager)
==============================================================================

JWM is a light-weight window manager for the X11 Window System.

Multi-monitor taskbars (this fork)
------------------------------------------------------------------------------
This fork adds independent per-monitor taskbars on top of JWM's
existing screen-bound tray placement. All new behavior is opt-in via
jwmrc attributes and costs nothing when unused; there are no new
dependencies. See the jwm(1) man page (TaskList) for details.

 - `<TaskList screen="local"/>` shows only the windows whose dominant
   (largest-overlap) Xinerama screen is the screen of the tray
   containing the task list. `screen="all"` (default) is the previous
   behavior; a number selects an explicit screen index.
 - Task buttons follow windows across monitors: when a window is
   moved to another screen, its button leaves the old screen's bar
   and is appended to the new screen's bar.
 - `<TaskList rows="2"/>` lays out task buttons in multiple rows on a
   horizontal bar (make the tray taller to match).
 - Task buttons can be reordered by dragging them with the left
   mouse button.
 - With XRandR available, trays can name screens by output:
   `<Tray screen="DP-2">` or `<Tray screen="primary">`.

Example: a bottom bar on each of two monitors, each listing only its
own windows, with a two-row bar on the second:

    <Tray screen="0" valign="bottom" height="28">
       <TaskList screen="local" maxwidth="256"/>
    </Tray>
    <Tray screen="1" valign="bottom" height="52">
       <TaskList screen="local" rows="2" maxwidth="256"/>
    </Tray>

Initial placement exclusions (this fork)
------------------------------------------------------------------------------
Reserve a monitor for manually moved windows with a top-level setting:

    <Placement exclude="DP-0"/>

The value is a RandR output name, "primary", or a zero-based Xinerama
screen index. Repeat the element to exclude more screens. New managed
windows avoid these screens even when an application restores a saved
position or a Group specifies a position. Placement uses the mouse's
screen if allowed, otherwise the primary allowed screen, otherwise the
first allowed screen. Existing windows keep their positions on restart;
dragging and later application-requested moves remain unrestricted.

Names require XRandR support. Unknown screens produce a warning and are
ignored. If every screen is excluded, JWM warns and allows the primary
screen (or screen 0) so new windows still have a destination. Overrides
such as application-owned popups are outside WM placement control.

Requirements
------------------------------------------------------------------------------
To build JWM you will need a C compiler (gcc works), X11, and the
"development headers" for X11 and Xlib.
If available and not disabled at compile time, JWM will also use
the following libraries:

 - cairo and librsvg2 for SVG icons and backgrounds.
 - pango (with the xft backend) for text layout.
 - libjpeg for JPEG icons and backgrounds.
 - libpng for PNG icons and backgrounds.
 - libXext for the shape extension.
 - libXrender for the render extension.
 - libXmu for rounded corners.
 - libXinerama for multiple head support.
 - libXpm for XPM icons and backgrounds.

Installation
------------------------------------------------------------------------------

 0. For building from the git repository, run "./autogen.sh".
 1. Run "./configure --help" for configuration options.
 2. Run "./configure [options]"
 3. Run "make" to build JWM.
 4. Run "make install" to install JWM.  Depending on where you are installing
    JWM, you may need to perform this step as root ("sudo make install").

License
------------------------------------------------------------------------------
See LICENSE for license information.

For more information see http://joewing.net/projects/jwm/
