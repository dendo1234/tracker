#include "tracker_common.h"
#include "tracker_icon.h"

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <assert.h>

global_variable wchar_t* g_output_path;
global_variable int g_process_priority = BELOW_NORMAL_PRIORITY_CLASS;
global_variable char g_running = 1;
global_variable UINT g_timer_period_ms = 60000;

typedef VOID WINAPI time_func(_Out_ LPSYSTEMTIME lpSystemTime);
global_variable time_func *g_user_time_func = GetSystemTime;

void set_timezone_designator(char buffer[7], unsigned buffer_size) {
  assert(buffer_size == 7);
  if (g_user_time_func == GetLocalTime) {
    TIME_ZONE_INFORMATION timezone;
    GetTimeZoneInformation(&timezone);
    int hours = timezone.Bias / 60;
    unsigned minutes = abs(timezone.Bias % 60);

    assert(abs(hours) < 100);

    snprintf(buffer, buffer_size, "%+03d:%02u", hours, minutes);

  } else {
    buffer[0] = 'Z';
    buffer[1] = '\0';
  }
}
TrackerError get_write_data(void) {
  SetLastError(0);
  TrackerError error = {};

  char title[256];
  title[0] = '\0';

  char executable_path[256];
  executable_path[0] = '\0';
 
	HWND window = GetForegroundWindow();
	if (window) {
    // DESC: get executable path:
    {
      DWORD process_id = 0;
      GetWindowThreadProcessId(window, &process_id);
      if (process_id) {
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, process_id);
        if (process) {
          WCHAR executable_path_w[256];
          DWORD size_w = sizeof(executable_path_w) / sizeof(WCHAR);
          int size = 0;
          if (QueryFullProcessImageNameW(process, 0, executable_path_w, &size_w)) {
            size = WideCharToMultiByte(CP_UTF8, 0, executable_path_w, size_w, executable_path, sizeof(executable_path), NULL, NULL);
            if (!size) {
              // NOTE: don't think this will ever fail, but anyways
              had_error(error);
            }
            executable_path[size] = '\0';
          } else {
            had_error(error);
          }

          CloseHandle(process);
        } else {
          // NOTE:: managed to fail OpenProcess while on RAMMap.exe when it was busy.
          // maybe there is a better way to get executable path that doesn't involve OpenProcess?
          // NOTE: there might be an alternative using WMI, but it seams too much of an hassle

          OutputDebugStringA("Failed to OpenProcess\n");

          // TODO: should we track this failure?
          //
          // had_error(error);
        }
      } else {
        had_error(error);
      }
    }

    // DESC: get window title:
    // TODO: might want to change this to a WM_GETTEXT message sent
    {
      WCHAR title_w[256];
      DWORD size_w = GetWindowTextW(window, title_w, 200);
      int size = 0;
      if (size_w) {
        size = WideCharToMultiByte(CP_UTF8, 0, title_w, size_w, title, sizeof(title), NULL, NULL);
        if (!size) {
          // NOTE: don't think this will ever fail, but anyways
          had_error(error);
        }
        title[size] = '\0';
      } else if (!GetLastError()) {
        // NOTE: error, but no code - assuming empty string
        OutputDebugStringA("GetWindowTextW error, but nothing on GetLastError... maybe window has no text??\n");

      } else {
        had_error(error);

      }
    }
  } else {
    // BUG: no foreground window, ON WINDOWS ????????
    // NOTE: it seams that this might happen randomly, so we don't want to show a message

    OutputDebugStringA("GetForegroundWindow failed\n");
    // TODO: should we track this failure?
    //
    // had_error(error);
  }

  // DESC: output to file:
  {
    char output_buffer[512];
    int char_to_write;

    SYSTEMTIME time;
    g_user_time_func(&time);

    assert(is_null_terminated(executable_path, sizeof(executable_path)));
    assert(is_null_terminated(title, sizeof(title)));

    char timezone_designator[7];
    set_timezone_designator(timezone_designator, sizeof(timezone_designator));

    // YYYY-MM-DDThh:mmTZD
    int needed_size = snprintf(output_buffer, sizeof(output_buffer), "%04d-%02d-%02dT%02d:%02d%s,%s,%s%s", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, timezone_designator, executable_path, title, g_line_ending);
    if (needed_size < 0) {
      char_to_write = 0;
      OutputDebugStringA("Enconding error on snprintf\n");
      had_error(error);
    } else if ((unsigned)needed_size > sizeof(output_buffer)) {
      // DESC: text truncated, manualy add g_line_ending
      assert(strcmp(g_line_ending, "\n") || strcmp(g_line_ending, "\r\n"));
      if (*g_line_ending == '\r') {
        output_buffer[sizeof(output_buffer)-2] = '\r';
      }
      output_buffer[sizeof(output_buffer)-1] = '\n';

      char_to_write = sizeof(output_buffer);
      OutputDebugStringA("output text truncated\n");
      had_error(error);
    } else {
      char_to_write = needed_size;
    }

    if (char_to_write) {
      wchar_t path[MAX_PATH];

      size_t size_w = sizeof(path) / sizeof(wchar_t);
      // WARN: although the _TRUNCATE, we don't care about the output if it got truncated, we just doesn't want windows CRT exception
      int count = _snwprintf_s(path, size_w, _TRUNCATE, L"%ws\\%04d-%02d-%02d.csv", g_output_path, time.wYear, time.wMonth, time.wDay);
      if (count > 0) {
        // TODO: investigate FILE_SHARE_WRITE and FILE_SHARE_DELETE
        HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (file == INVALID_HANDLE_VALUE) {
          OutputDebugStringA("Error opening file\n");
          had_error(error);
        } else {
          // NOTE: Windows 7: This parameter can not be NULL.
          DWORD bytesWritten;

          WriteFile(file, output_buffer, char_to_write, &bytesWritten, 0);

          CloseHandle(file);
        }
      } else {
        // NOTE: startup check makes this impossible (assuming g_output_path doesn't change)
        assert(0);
      }
    }
  }

  return error;
}

UINT_PTR create_timer(UINT period_ms) {
  // NOTE: is UOI_TIMERPROC_EXCEPTION_SUPPRESSION really necessary?
  // It's recomended on SetTimer documentation but we don't use TIMERPROC
  // NOTE: this fails on win7, would require more code to support it
 
  // BOOL timerproc_exception_suppression = 0;
  // if (SetUserObjectInformationA(GetCurrentProcess(),UOI_TIMERPROC_EXCEPTION_SUPPRESSION,&timerproc_exception_suppression,sizeof(timerproc_exception_suppression))) {
  //   // DESC: success
  //
  // } else {
  //   OutputDebugStringA("Couldn't disable timerproc exception suppresssion\n");
  //   show_error_and_quit_message((TrackerError){ .should_quit = 0, .error_ocurred = 1, .line = __LINE__}, "Warning: couldn't disable timerproc exception");
  // }

  UINT_PTR timer_identifier = SetTimer(0, 0, period_ms, 0);
  if (timer_identifier == 0) {
    OutputDebugStringA("Failed to create timer\n");
    show_error_and_quit((TrackerError){ .should_quit = 1, .error_ocurred = 1, .line = __LINE__});
  }
  return timer_identifier;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  (void)hInstance;
  (void)hPrevInstance;
  (void)nCmdShow;

  // NOTE: on windows, it seams hard to consolidate command line execution and
  // execution from the explorer. This AttachConsole will make the output look
  // like this:
  // w:\>tracker.exe
  //
  // w:\>[TRACKER OUTPUT HERE]
  //
  // compiling with subsytem console would solve this, but might make a console
  // window pop up when running thought explorer. Proper fix requires creating
  // two binaries, one with substyem console and the other with window, which
  // I'm not too interested

  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    FILE *file;
    errno_t error = freopen_s(&file, "CONOUT$", "w", stdout);
    error |= freopen_s(&file, "CONOUT$", "w", stderr);
    if (error) {
      OutputDebugStringA("Couldn't rebind stdout/stderr to CONOUT% \n");
    } else {
      g_has_console = 1;
      OutputDebugStringA("Got a console");
    }

  } else {
    // DESC: no console

  }

  if (*lpCmdLine == '\0') {
    char text_buffer[200];
    snprintf(text_buffer, sizeof(text_buffer),"tracker version v%u\nUsage: tracker [OPTION]... [DIRECTORY]\n\nConsider using shortcuts or running through the command line to pass the arguments", VERSION);
    output(text_buffer);
    ExitProcess(0);
  }

  // DESC: command line arguments parsing:
  wchar_t *c = lpCmdLine;
  while (*c) {
    if (*c == L'-') {
      // DESC: parsing options
      c++;

      while (*c != L' ' && *c != L'\0') {
        switch (*c++) {
          case L'h': {
            output(
              "tracks foreground window and logs it on a csv file on a specified directory\n"
              "Usage: [OPTION]... [DIRECTORY]\n\n"
              "Running though a command line will redirect all of the output to it\n"
              "\n"
              "Available options:\n"
              "\t-h: shows this\n"
              "\t-s: don't show message boxes. Note: they show up in case of an error/warning and pause execution till dismissed. The message boxes for fatal errors are still shown\n"
              "\t-b: set process priority to PROCESS_MODE_BACKGROUND_BEGIN instead of BELOW_NORMAL_PRIORITY_CLASS. Note: This will lower the process priority by a lot, but may be buggy, specially if the system is overloaded. Use with caution.\n"
              "\t-c: use CRLF line endings. Note: Use it if your text reader reads every output on the same line\n"
              "\t-l: logs the time in 'local time' instead of UTC. Note: timezone information is appended to the timestamp\n"
              "\t-t[MILISECONDS]: how much milliseconds between each track. Default: 60000\n"
            );
          } break;
          case L's': {
            g_silent = 1;
          } break;
          case L'b': {
            g_process_priority = PROCESS_MODE_BACKGROUND_BEGIN;
          } break;
          case L'c': {
            g_line_ending = "\r\n";
          } break;
          case L'l': {
            g_user_time_func = GetLocalTime;
          } break;
          case L't': {
            int chars_read;
            swscanf_s(c, L"%u%n", &g_timer_period_ms, &chars_read);
            if (chars_read == 0) {
              output("Couldn't parse -t argument");
            }
            c += chars_read;
          } break;
        
          default: {
            char text_buffer[100];
            snprintf(text_buffer, sizeof(text_buffer), "Unrecognized option '-%c'\n", *c);
            output(text_buffer);
          }
        }
      }
      continue;
    } else if (*c == L' ') {
      c++;
    } else {
      g_output_path = c;
      break;
    }
  }

  if (g_output_path == 0) {
    output("No directory provided\n");
    ExitProcess(1);
  }

  // DESC: start checks:
  {
    TrackerError error;
    SYSTEMTIME time;
    g_user_time_func(&time);

    wchar_t path[MAX_PATH];

    size_t size_w = sizeof(path) / sizeof(wchar_t);
    // WARN: although the _TRUNCATE, we don't care about the output if it got truncated, we just doesn't want windows CRT exception
    int count = _snwprintf_s(path, size_w, _TRUNCATE, L"%ws\\%04d-%02d-%02d.csv", g_output_path, time.wYear, time.wMonth, time.wDay);
    if (count > 0) {
      HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (file == INVALID_HANDLE_VALUE) {
        had_error(error)
        error.should_quit = 1;

        char buffer[MAX_PATH + 200];
        snprintf(buffer, sizeof(buffer), "Startup check: error opening file at the following path. Make sure the directory exists.\n\"%.366ls\".\n", path); 

        show_error_and_quit_message(error, buffer);
      } else {
        // DESC: success
        CloseHandle(file);
      }
    } else {
      had_error(error)
      error.should_quit = 1;

      char buffer[MAX_PATH + 200];
      snprintf(buffer, sizeof(buffer), "Startup check: path passed is too big:\n\"%.414ls...\"\n", path); 
      show_error_and_quit_message(error, buffer);
    }
  }

  {
    TrackerError icon_error = icon_create();
    if (icon_error.error_ocurred != 0) {
      show_error_and_quit_message(icon_error, "Failed to create notification icon. The program will start, but can only be stoped by task killing\n");
    }
  }

  
  UINT_PTR timer_identifier = create_timer(g_timer_period_ms);

	if(SetPriorityClass(GetCurrentProcess(), g_process_priority))
	{
    OutputDebugStringA("Successefully set thread priority\n");
  } else {
    show_error_and_quit((TrackerError){ .should_quit = 0, .error_ocurred = 1, .line = __LINE__});
	}

  OutputDebugStringA("Initialization finished\n");

  // DESC: main loop
  WPARAM exit_code = 1;
  {
    ULONGLONG last_query_ms = GetTickCount64();
    while (g_running && WaitMessage()) {
      MSG message;

      // NOTE: I've a suspission that WaitMessage sometimes disregards WM_TIMER messages.
      // an "if" here *should* work, but "while" is more reliable
      while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE)) {
        switch (message.message) {
          case WM_TIMER: {
            if (message.wParam == timer_identifier) {
              {
                ULONGLONG currenty_query_ms = GetTickCount64();
                ULONGLONG time_between_queries = currenty_query_ms - last_query_ms;
                if (time_between_queries > 2*g_timer_period_ms) {
                  TrackerError error;
                  had_error(error)
                  char buffer[100];
                  snprintf(buffer, sizeof(buffer), "too much time between tracks. Time between: %llu ms", time_between_queries);
                  show_error_and_quit_message(error, buffer);
                }
                last_query_ms = currenty_query_ms;
              }

              TrackerError error = get_write_data();
              if (error.error_ocurred) {
                show_error_and_quit(error);
              }
            }

            DispatchMessageW(&message);
          } break;
          case WM_QUIT: {
            exit_code = message.wParam;
            g_running = 0;
          } break;
          default: {
            DispatchMessageW(&message);
          }
        }
      }
    }
  }


  return (int)exit_code;
}
