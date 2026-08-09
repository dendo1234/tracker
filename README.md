# tracker
tracker is a Windows C application that logs the foreground window in a regular schedule.

I've made it for my own personal use after wasting multiple days doing unproductive things. It didn't stopped me from that, but now I know exactly how much time I've spent doing what on my computer.

The program is a standalone executable with no installer. It doesn't intentionally[^1] mess/create any folder/file/registry except the provided output directory.

[^1]: Windows may create registry keys and files itself do to the way the OS works. There is no way to avoid that.

It should have a pretty small memory and CPU footprint, although I didn't bother measuring, trust me broooooo.

## How it works
It's just Windows API calls, not too complicated. The most relevant one is GetForegroundWindow, which gives back the window the user currently has active. From that, the program gets the window title + executable path and logs it to a file specified in the command line arguments.

Because it so dependent on Windows API, there is no point on porting this to other operating systems, it would be better to just rewrite this.

## Output

A directory must be provided. In it, a CSV file for each day tracked, named "YYYY-MM-DD.csv", is created. It has UTF-8 encoding with LF line endings (although the latter can be changed to CRLF with command line options). The columns are:

\<time_stamp\>,\<executable_path\>,\<window_title\>

time_stamp format follows ISO 8601: "YYYY-MM-DDThh:mmTZD".

Example output:

```
2026-08-08T21:35Z,C:\Windows\explorer.exe,tes te – Explorador de Arquivos
2026-08-08T21:36Z,C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe,NOTIFYICONDATAW (shellapi.h) - Win32 apps | Microsoft Learn e mais 61 páginas - Pessoal — Microsoft​ Edge
2026-08-08T21:37Z,C:\Program Files\WindowsApps\Microsoft.WindowsTerminal_1.24.11911.0_x64__8wekyb3d8bbwe\WindowsTerminal.exe,tracker start
2026-08-08T21:38Z,C:\Program Files\WindowsApps\Microsoft.WindowsTerminal_1.24.11911.0_x64__8wekyb3d8bbwe\WindowsTerminal.exe,tracker start
```

If the interval is close to one minute or less, a given time_stamp can be repeated. executable_path or window_title might be missing if the tracker failed.

It should then be easy to analyse the .csv file with any tools you like, or just manually read it with your favorite text reader.

## Installation

>### Disclaimer:
>
> This tool doesn't automatically deletes the data it collects. If worry about your privacy, remember to delete the old .csv files after you are done with them.
>
> Also, it does write data to the disk, which means, depending on the circumstances, that even deleted files could be restored.
>
> You can always stop tracker by quitting though the notification icon on the taskbar to prevent all of this, or just don't use it if it is too much of a concern.

Since it comes with no installer, you must manually setup it. If you just want to run it once, you can run it though the command line. Otherwise, I suggest running though shortcuts, follow the guide:

1. Move the .exe to a application folder. Suggestion: if you are a Windows purist, you can install at:
    1. %ProgramFiles%\tracker\tracker-v0.exe - for a system wide install, requires admin privilege
    2. %LocalAppData%\Programs\tracker\tracker-v0.exe - for a user only install
2. Create a .lnk shortcut. You can use explorer for that by:
    1. right click on the .exe file
    2. if on Windows 11, click on "Show more options"
    3. and click on "Create Shortcut"
3. Configure shortcut:
    1. right click the shortcut and go to properties
    2. on the "Target" field, there will be the executable path you just installed
    3. insert command line options if any after the path on the "Target" field
    4. after the options, insert the path of the directory you want to save the tracker data (don't include quotes "")
4. (Optional) Move the shortcut to auto initialize tracker on system startup to the path:
    1. %AppData%\Microsoft\Windows\Start Menu\Programs\Startup
5. Done :) You can start the program by double clicking the shortcut. A notification icon will show up on the taskbar when it's running

## Options

* -h: shows help.
* -s: don't show message boxes. Note: they show up in case of an error/warning and pause execution till dismissed. The message boxes for fatal errors are still shown.
* -b: set process priority to PROCESS_MODE_BACKGROUND_BEGIN instead of BELOW_NORMAL_PRIORITY_CLASS. Note: This will lower the process priority by a lot, but may be buggy, specially if the system is overloaded. Use with caution.
* -c: use CRLF line endings. Note: Use it if your text reader reads every output on the same line.
* -l: logs the time in 'local time' instead of UTC. Note: timezone information is appended to the timestamp.
* -t\[MILLISECONDS\]: how much milliseconds between each track. Default: 60000.

## Compiling

In the build.bat file, there are the compilation commands. There are no dependencies apart from Windows and the C standard library.

The compiler must support C23's `int variable = {}` feature

You can compile it by running:
```
cl tracker.c User32.lib Shell32.lib
```

If you have MSYS2 gcc or clang you can run:
```
gcc tracker.c -mwindows -municode
clang tracker.c -mwindows -municode
```


## End

If you end up using this or finding it interesting, considering sending a message. I've spent quite some time developing it and tightening the logic so that it never fails, making sure it even works on a Windows 7 SP1 vm, and adding some extra options that aren't relevant to me, so it would be pretty cool to know if someone actually cares about this.

Feel free to send contributions.

TODO: figure a cooler and unique name for this program

