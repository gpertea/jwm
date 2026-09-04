/**
 * @file remote.c
 * @brief Out-of-process emergency window control.
 *
 * These commands use an independent X connection. They remain available when
 * the running window manager is blocked and cannot process client messages.
 */

#include "jwm.h"
#include "remote.h"

#include <errno.h>

#define MAX_PARENT_DEPTH 32
#define MAX_CLIENTS      65536

static int remoteError;

static int RemoteErrorHandler(Display *display, XErrorEvent *event);
static Window ParseWindow(const char *value);
static Window GetFrame(Display *display, Window root, Window window);
static Window ReadWindow(Display *display, Window window, Atom property);
static unsigned long ReadCardinal(Display *display, Window window,
                                  Atom property);
static int GetStackIndex(Display *display, Window root, Window frame,
                         unsigned int *count);
static char IsAncestor(Display *display, Window ancestor, Window window);
static void PrintTitle(Display *display, Window window);
static void PrintWindow(Display *display, Window root, Window window,
                        Window active, Window focus);
static int ListWindows(Display *display, Window root);
static int ShowStatus(Display *display, Window root, Window window);
static void SendActiveRequest(Display *display, Window root, Window window);
static char ForceStack(Display *display, Window window, Window frame,
                       int stackMode, char map);
static int ControlWindow(Display *display, Window root, const char *command,
                         Window window);

/** Ignore X errors from stale client-list entries. */
static int RemoteErrorHandler(Display *display, XErrorEvent *event)
{
   (void)display;
   (void)event;
   remoteError = 1;
   return 0;
}

/** Parse an X window ID. */
static Window ParseWindow(const char *value)
{
   char *end;
   unsigned long result;

   errno = 0;
   result = strtoul(value, &end, 0);
   if(errno || end == value || *end || result == 0) {
      return None;
   }
   return (Window)result;
}

/** Find the root child that contains a client window. */
static Window GetFrame(Display *display, Window root, Window window)
{
   Window current;
   Window rootReturn;
   Window parent;
   Window *children;
   unsigned int count;
   int depth;

   current = window;
   for(depth = 0; depth < MAX_PARENT_DEPTH; depth++) {
      children = NULL;
      remoteError = 0;
      if(!JXQueryTree(display, current, &rootReturn, &parent,
                      &children, &count) || remoteError) {
         if(children) {
            JXFree(children);
         }
         return None;
      }
      if(children) {
         JXFree(children);
      }
      if(parent == root) {
         return current;
      }
      if(parent == None || current == root) {
         return None;
      }
      current = parent;
   }
   return None;
}

/** Read a single window-valued property. */
static Window ReadWindow(Display *display, Window window, Atom property)
{
   Atom actualType;
   int actualFormat;
   unsigned long count;
   unsigned long remaining;
   unsigned char *data;
   Window result;

   data = NULL;
   result = None;
   remoteError = 0;
   if(JXGetWindowProperty(display, window, property, 0, 1, False,
                          XA_WINDOW, &actualType, &actualFormat, &count,
                          &remaining, &data) == Success
      && !remoteError && actualFormat == 32 && count == 1) {
      result = *(Window*)data;
   }
   if(data) {
      JXFree(data);
   }
   return result;
}

/** Read a single cardinal property. */
static unsigned long ReadCardinal(Display *display, Window window,
                                  Atom property)
{
   Atom actualType;
   int actualFormat;
   unsigned long count;
   unsigned long remaining;
   unsigned char *data;
   unsigned long result;

   data = NULL;
   result = ~0UL;
   remoteError = 0;
   if(JXGetWindowProperty(display, window, property, 0, 1, False,
                          XA_CARDINAL, &actualType, &actualFormat, &count,
                          &remaining, &data) == Success
      && !remoteError && actualFormat == 32 && count == 1) {
      result = *(unsigned long*)data;
   }
   if(data) {
      JXFree(data);
   }
   return result;
}

/** Get the zero-based root stacking index for a frame. */
static int GetStackIndex(Display *display, Window root, Window frame,
                         unsigned int *count)
{
   Window rootReturn;
   Window parent;
   Window *children;
   unsigned int x;
   int result;

   children = NULL;
   result = -1;
   remoteError = 0;
   if(JXQueryTree(display, root, &rootReturn, &parent, &children, count)
      && !remoteError) {
      for(x = 0; x < *count; x++) {
         if(children[x] == frame) {
            result = (int)x;
            break;
         }
      }
   }
   if(children) {
      JXFree(children);
   }
   return result;
}

/** Determine if a window is equal to or contained by an ancestor. */
static char IsAncestor(Display *display, Window ancestor, Window window)
{
   Window rootReturn;
   Window parent;
   Window *children;
   unsigned int count;
   int depth;

   for(depth = 0; depth < MAX_PARENT_DEPTH && window != None; depth++) {
      if(window == ancestor) {
         return 1;
      }
      children = NULL;
      remoteError = 0;
      if(!JXQueryTree(display, window, &rootReturn, &parent,
                      &children, &count) || remoteError) {
         if(children) {
            JXFree(children);
         }
         break;
      }
      if(children) {
         JXFree(children);
      }
      window = parent;
   }
   return 0;
}

/** Print a UTF-8 or legacy window title without control characters. */
static void PrintTitle(Display *display, Window window)
{
   Atom nameAtom;
   Atom utf8Atom;
   Atom actualType;
   int actualFormat;
   unsigned long count;
   unsigned long remaining;
   unsigned long x;
   unsigned char *data;
   char *legacy;

   nameAtom = JXInternAtom(display, "_NET_WM_NAME", False);
   utf8Atom = JXInternAtom(display, "UTF8_STRING", False);
   data = NULL;
   remoteError = 0;
   if(JXGetWindowProperty(display, window, nameAtom, 0, 1024, False,
                          utf8Atom, &actualType, &actualFormat, &count,
                          &remaining, &data) == Success
      && !remoteError && actualFormat == 8 && data) {
      for(x = 0; x < count; x++) {
         if(data[x] == '\n' || data[x] == '\r' || data[x] == '\t') {
            putchar(' ');
         } else {
            putchar(data[x]);
         }
      }
      JXFree(data);
      return;
   }
   if(data) {
      JXFree(data);
   }

   legacy = NULL;
   remoteError = 0;
   if(JXFetchName(display, window, &legacy) && !remoteError && legacy) {
      for(x = 0; legacy[x]; x++) {
         if(legacy[x] == '\n' || legacy[x] == '\r' || legacy[x] == '\t') {
            putchar(' ');
         } else {
            putchar((unsigned char)legacy[x]);
         }
      }
   }
   if(legacy) {
      JXFree(legacy);
   }
}

/** Print one managed client. */
static void PrintWindow(Display *display, Window root, Window window,
                        Window active, Window focus)
{
   Atom desktopAtom;
   Window frame;
   XClassHint hint;
   unsigned long desktop;
   char activeFlag;
   char focusFlag;

   desktopAtom = JXInternAtom(display, "_NET_WM_DESKTOP", False);
   frame = GetFrame(display, root, window);
   desktop = ReadCardinal(display, window, desktopAtom);
   activeFlag = window == active ? 'A' : '-';
   focusFlag = IsAncestor(display, window, focus) ? 'F' : '-';

   memset(&hint, 0, sizeof(hint));
   remoteError = 0;
   JXGetClassHint(display, window, &hint);
   printf("%c%c 0x%08lx frame=0x%08lx desktop=", activeFlag, focusFlag,
          window, frame);
   if(desktop == ~0UL) {
      printf("all");
   } else {
      printf("%lu", desktop);
   }
   printf(" class=%s.%s title=", hint.res_name ? hint.res_name : "-",
          hint.res_class ? hint.res_class : "-");
   PrintTitle(display, window);
   putchar('\n');

   if(hint.res_name) {
      JXFree(hint.res_name);
   }
   if(hint.res_class) {
      JXFree(hint.res_class);
   }
}

/** List clients using the EWMH stacking list. */
static int ListWindows(Display *display, Window root)
{
   Atom listAtom;
   Atom activeAtom;
   Atom actualType;
   int actualFormat;
   int revert;
   unsigned long count;
   unsigned long remaining;
   unsigned long x;
   unsigned char *data;
   Window active;
   Window focus;
   Window *windows;

   listAtom = JXInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
   activeAtom = JXInternAtom(display, "_NET_ACTIVE_WINDOW", False);
   active = ReadWindow(display, root, activeAtom);
   focus = None;
   JXGetInputFocus(display, &focus, &revert);

   data = NULL;
   remoteError = 0;
   if(JXGetWindowProperty(display, root, listAtom, 0, MAX_CLIENTS, False,
                          XA_WINDOW, &actualType, &actualFormat, &count,
                          &remaining, &data) != Success
      || remoteError || actualFormat != 32 || !data) {
      if(data) {
         JXFree(data);
      }
      fprintf(stderr, "error: could not read the JWM client list\n");
      return 1;
   }

   windows = (Window*)data;
   for(x = 0; x < count; x++) {
      if(GetFrame(display, root, windows[x]) != None) {
         PrintWindow(display, root, windows[x], active, focus);
      }
   }
   JXFree(data);
   return 0;
}

/** Print direct X state for one client and its JWM frame. */
static int ShowStatus(Display *display, Window root, Window window)
{
   Atom activeAtom;
   Atom desktopAtom;
   XWindowAttributes clientAttributes;
   XWindowAttributes frameAttributes;
   Window active;
   Window focus;
   Window frame;
   unsigned long desktop;
   unsigned int stackCount;
   int revert;
   int stackIndex;

   frame = GetFrame(display, root, window);
   if(frame == None) {
      fprintf(stderr, "error: invalid or detached window 0x%lx\n", window);
      return 1;
   }

   remoteError = 0;
   if(!JXGetWindowAttributes(display, window, &clientAttributes)
      || !JXGetWindowAttributes(display, frame, &frameAttributes)
      || remoteError) {
      fprintf(stderr, "error: could not inspect window 0x%lx\n", window);
      return 1;
   }

   activeAtom = JXInternAtom(display, "_NET_ACTIVE_WINDOW", False);
   desktopAtom = JXInternAtom(display, "_NET_WM_DESKTOP", False);
   active = ReadWindow(display, root, activeAtom);
   desktop = ReadCardinal(display, window, desktopAtom);
   focus = None;
   JXGetInputFocus(display, &focus, &revert);
   stackCount = 0;
   stackIndex = GetStackIndex(display, root, frame, &stackCount);

   printf("window=0x%lx frame=0x%lx desktop=", window, frame);
   if(desktop == ~0UL) {
      printf("all");
   } else {
      printf("%lu", desktop);
   }
   printf(" client_map=%d frame_map=%d stack=%d/%u active=0x%lx "
          "focus=0x%lx focus_in_client=%d\n",
          clientAttributes.map_state, frameAttributes.map_state,
          stackIndex, stackCount, active, focus,
          IsAncestor(display, window, focus));
   return 0;
}

/** Ask the running window manager to activate a client. */
static void SendActiveRequest(Display *display, Window root, Window window)
{
   Atom activeAtom;
   XEvent event;

   activeAtom = JXInternAtom(display, "_NET_ACTIVE_WINDOW", False);
   memset(&event, 0, sizeof(event));
   event.xclient.type = ClientMessage;
   event.xclient.display = display;
   event.xclient.window = window;
   event.xclient.message_type = activeAtom;
   event.xclient.format = 32;
   event.xclient.data.l[0] = 2;
   event.xclient.data.l[1] = CurrentTime;
   JXSendEvent(display, root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
   JXFlush(display);
}

/** Bypass SubstructureRedirect long enough to stack a JWM frame. */
static char ForceStack(Display *display, Window window, Window frame,
                       int stackMode, char map)
{
   XSetWindowAttributes attributes;
   XWindowAttributes clientAttributes;
   XWindowAttributes frameAttributes;
   XWindowChanges changes;

   remoteError = 0;
   if(!JXGetWindowAttributes(display, window, &clientAttributes)
      || !JXGetWindowAttributes(display, frame, &frameAttributes)
      || remoteError) {
      return 0;
   }

   attributes.override_redirect = True;
   JXChangeWindowAttributes(display, frame, CWOverrideRedirect, &attributes);
   if(map) {
      JXChangeWindowAttributes(display, window, CWOverrideRedirect,
                               &attributes);
      JXMapWindow(display, window);
      JXMapWindow(display, frame);
   }

   memset(&changes, 0, sizeof(changes));
   changes.stack_mode = stackMode;
   JXConfigureWindow(display, frame, CWStackMode, &changes);
   JXSync(display, False);

   attributes.override_redirect = frameAttributes.override_redirect;
   JXChangeWindowAttributes(display, frame, CWOverrideRedirect, &attributes);
   if(map) {
      attributes.override_redirect = clientAttributes.override_redirect;
      JXChangeWindowAttributes(display, window, CWOverrideRedirect,
                               &attributes);
   }
   JXSync(display, False);
   return !remoteError;
}

/** Apply one direct emergency action to a client and its frame. */
static int ControlWindow(Display *display, Window root, const char *command,
                         Window window)
{
   struct timespec delay;
   Window frame;

   frame = GetFrame(display, root, window);
   if(frame == None) {
      fprintf(stderr, "error: invalid or detached window 0x%lx\n", window);
      return 1;
   }

   remoteError = 0;
   if(!strcmp(command, "activate")) {
      SendActiveRequest(display, root, window);
      delay.tv_sec = 0;
      delay.tv_nsec = 100000000;
      nanosleep(&delay, NULL);
      if(!ForceStack(display, window, frame, Above, 1)) {
         fprintf(stderr, "error: could not raise window 0x%lx\n", window);
         return 1;
      }
      JXSetInputFocus(display, window, RevertToParent, CurrentTime);
      SendActiveRequest(display, root, window);
   } else if(!strcmp(command, "raise")) {
      if(!ForceStack(display, window, frame, Above, 0)) {
         fprintf(stderr, "error: could not raise window 0x%lx\n", window);
         return 1;
      }
   } else if(!strcmp(command, "lower")) {
      if(!ForceStack(display, window, frame, Below, 0)) {
         fprintf(stderr, "error: could not lower window 0x%lx\n", window);
         return 1;
      }
   } else if(!strcmp(command, "show")) {
      if(!ForceStack(display, window, frame, Above, 1)) {
         fprintf(stderr, "error: could not show window 0x%lx\n", window);
         return 1;
      }
   } else if(!strcmp(command, "hide")) {
      JXUnmapWindow(display, frame);
   } else {
      fprintf(stderr, "error: unknown remote command: %s\n", command);
      return 1;
   }
   JXSync(display, False);
   if(remoteError) {
      fprintf(stderr, "error: X11 rejected remote command %s\n", command);
      return 1;
   }
   return ShowStatus(display, root, window);
}

/** Determine if a remote-control command requires a window argument. */
char RemoteCommandNeedsWindow(const char *command)
{
   return strcmp(command, "list") != 0;
}

/** Run an out-of-process remote-control command. */
int RunRemoteCommand(const char *command, const char *argument,
                     const char *displayName)
{
   Display *remoteDisplay;
   Window root;
   Window window;
   int result;

   remoteDisplay = JXOpenDisplay(displayName);
   if(!remoteDisplay) {
      fprintf(stderr, "error: could not open display%s%s\n",
              displayName ? " " : "", displayName ? displayName : "");
      return 1;
   }
   JXSetErrorHandler(RemoteErrorHandler);
   root = RootWindow(remoteDisplay, DefaultScreen(remoteDisplay));

   if(!strcmp(command, "list")) {
      result = ListWindows(remoteDisplay, root);
   } else {
      window = ParseWindow(argument);
      if(window == None) {
         fprintf(stderr, "error: invalid window ID: %s\n", argument);
         result = 1;
      } else if(!strcmp(command, "status")) {
         result = ShowStatus(remoteDisplay, root, window);
      } else {
         result = ControlWindow(remoteDisplay, root, command, window);
      }
   }

   JXCloseDisplay(remoteDisplay);
   return result;
}
