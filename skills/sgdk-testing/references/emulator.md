# Launch and observe ares

Prefer the emulator CLI for a known build rather than stepping through Load menus.
Check the installed binary's `--help`; our ares v148 accepts `--system` and an
image path. Select **Mega Drive** explicitly, so a format-selection dialog is not
mistaken for an unresponsive game.

```sh
ares --system "Mega Drive" "/absolute/path/game.bin"
```

On macOS, if the app executable is not on PATH:

```sh
open -a ares --args --system "Mega Drive" "/absolute/path/game.bin"
```

Quote paths or pass an argument list, especially with spaces. On Windows use the
installed executable path (PowerShell's `&` when quoting it). On Linux use the
installed `ares` executable. Verify the available CLI on each host; the examples
do not constitute Windows emulator qualification.

An already-running app may only be focused by the OS launcher. Verify the loaded
image path or a unique build label before testing. Prefer one known instance;
if the launcher ignores arguments, quit it normally and relaunch the CLI.
Reserve UI Load menus for a genuine CLI limitation. Avoid repeatedly launching
new instances, which can leave the game in one window and captures/input in
another. Do not change bindings, region preferences or other emulator settings
without the user's request.

CLI launch is not gameplay input. Bind captures to the actual game window,
inspect a fresh frame and confirm the correct regional build/rate. If automated
keys fail, distinguish focus, configured bindings and tool limitations from a
game fault. Ask for a physical-button check when needed; do not call a game
frozen merely because synthetic keys did not reach it. Record visual/audio
observations separately from build and CPU-test results.
