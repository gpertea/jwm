/**
 * @file remote.h
 * @brief Out-of-process emergency window control.
 */

#ifndef REMOTE_H
#define REMOTE_H

/** Run a remote-control command and return its exit status. */
int RunRemoteCommand(const char *command, const char *argument,
                     const char *displayName);

/** Determine if a remote-control command requires a window argument. */
char RemoteCommandNeedsWindow(const char *command);

#endif /* REMOTE_H */
