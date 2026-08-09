#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef VERSION
#define VERSION 0
#endif

#include <windows.h>
#include <stdio.h>

#define global_variable static
#define local_persist static
#define internal static

global_variable unsigned char g_has_console = 0;
global_variable unsigned char g_silent = 0;
global_variable const char* g_line_ending = "\n";

typedef struct {
  DWORD windows_error_code;
  unsigned short line;
  char error_ocurred;
  char should_quit;
} TrackerError;

#define had_error(error)                                                       \
  error.line = __LINE__;                                                       \
  error.error_ocurred = 1;                                                     \
  error.windows_error_code = GetLastError();

internal void output(char* string) {
  if (g_has_console) {
    puts(string);
  } else {
    if (!g_silent) {
      MessageBoxA(NULL, string, "tracker", MB_OK);
    }
  }
}

internal unsigned char is_null_terminated(char* string, unsigned int size) {
  for (unsigned int i = 0; i < size; i++) {
    if (*string == '\0') {
      return 1;
    }
    string++;
  }
  return 0;
}

internal void show_error_and_quit_message(TrackerError error, const char* extra_message) {
  // NOTE: if the same error happens too many times, quit
  // TODO: there is no knowlage in the current implementation on how much time between each failure.
  // it isn't ideal to reset the aplication if the errors are years apart
  local_persist unsigned short last_error_line = 0;
  local_persist unsigned short last_error_count = 0;

  if (last_error_line != error.line) {
    last_error_line = error.line;
    last_error_count = 1;
  } else {
    last_error_count++;
  }

  if (last_error_count >= 30) {
    error.should_quit = 1;
    extra_message = "Error repeated more than 30 times, exiting\n";
  }

  if (extra_message == 0) {
    extra_message = "Error in tracker\n";
  }

  if (error.should_quit) {
    g_silent = 0;
  }

	LPVOID lpMsgBuf;
	DWORD dw = error.windows_error_code; 

	if (FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &lpMsgBuf,
		0, NULL) == 0) {
    // NOTE: FormatMessage failed
		MessageBox(NULL, TEXT("FormatMessage failed"), TEXT("Error"), MB_OK);
		ExitProcess(dw);
	}

  char buffer[512];
  snprintf(buffer, sizeof(buffer), "%sline: %d\nWindows message: %s", extra_message, error.line, (char*)lpMsgBuf);
  output(buffer);

	LocalFree(lpMsgBuf);
  if (error.should_quit) {
    ExitProcess(dw);
  }
}

internal void show_error_and_quit(TrackerError error) {
  show_error_and_quit_message(error, 0);
}

