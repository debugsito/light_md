# light_md

**English** | [Español](README.es.md) | [Português](README.pt.md) | [Français](README.fr.md) | [Deutsch](README.de.md)

A lightweight Markdown editor in C++ and Qt 6, for everyone.

Open a `.md` file and read the formatted page right away. Need to change something? Press **Ctrl+E** to edit. Prefer menus? Everything lives under **Options**, each with a keyboard shortcut.

![light_md: the writing pane on the left and the formatted result on the right](docs/screenshot.png)

**[Download light_md for Windows](https://github.com/josel120/light_md/releases/latest)**: unzip it and open `bin\light_md.exe`. Nothing to install.

## What it does

- Opens in **Reader** mode (formatted page only). **Ctrl+E** shows the writing pane; **Ctrl+R** or **Esc** returns to Read only.
- **Options** menu in everyday words: New, Open, Save, Format, Theme, Help — every item has a shortcut.
- Open and save `.md` documents, also by dragging them onto the window or using "Open with" from the file manager.
- Export to PDF and print: the result looks just like the reading pane.
- Word count and an unsaved-changes warning, including when closing the window.
- Themes: **System** (desktop default), **Minimal** (quiet light look), **Omarchy** (live desktop theme on Linux). Windows and macOS keep **System** by default.

## Keyboard shortcuts

| Action | Shortcut |
| --- | --- |
| Edit document | Ctrl+E |
| Read only | Ctrl+R or Esc |
| New / Open / Save / Save as | Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S |
| Export PDF / Print | Ctrl+Shift+E / Ctrl+P |
| Undo / Redo | Ctrl+Z / Ctrl+Shift+Z (or Ctrl+Y) |
| Bold / Italic / Link | Ctrl+B / Ctrl+I / Ctrl+K |
| Big title / Subtitle | Ctrl+1 / Ctrl+2 |
| Bullet / Numbered / Quote | Ctrl+Shift+8 / Ctrl+Shift+7 / Ctrl+Shift+Q |
| Theme System / Minimal / Omarchy | Ctrl+Shift+1 / Ctrl+Shift+2 / Ctrl+Shift+3 |
| Help | F1 |

On macOS, use **Cmd** instead of Ctrl where the platform expects it (standard Qt shortcuts).

## Build on Linux

Needs Qt 6 development packages (`Widgets` and `PrintSupport`). On Arch / [Omarchy](https://omarchy.org/):

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-wayland
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/light_md
```

Install for your user (binary, `.desktop` entry, and icon):

```bash
cmake --install build --prefix ~/.local
```

Or build an Arch package from a release tag with `packaging/arch/PKGBUILD` (update `sha256sums` when publishing).

On first launch under Omarchy, the theme defaults to **Omarchy** and updates when you run `omarchy theme set`. Elsewhere it defaults to **System**. Your choice is remembered.

### Mobile UI preview (Linux / desktop QML)

The same build can produce `build/bin/light_md_mobile`: a touch-oriented QML shell (Reader / Edit / Open / Save) on the shared `Document` core. This is the Android UI prototype; turn it off with `-DLIGHT_MD_BUILD_MOBILE=OFF`.

```bash
./build/bin/light_md_mobile path/to/file.md
```


## Build on Windows (Qt 6 and MinGW)

You need Qt 6 installed (for example in `C:\Qt`) with its CMake and Ninja.

```powershell
$env:PATH = "C:\Qt\Tools\mingw1510_64\bin;C:\Qt\6.12.1\mingw_64\bin;$env:PATH"
C:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.12.1/mingw_64
C:\Qt\Tools\CMake_64\bin\cmake.exe --build build
.\build\light_md.exe
```

Adjust the paths if your Qt or MinGW version is different.

## Take it to another Windows computer

To create a folder that runs on any Windows PC without installing Qt:

```powershell
C:\Qt\Tools\CMake_64\bin\cmake.exe --install build --prefix dist
```

Copy the whole `dist` folder and open `dist\bin\light_md.exe`.

## License

MIT

Omarchy theme watching follows the approach used in Omacut (MIT, David Heinemeier Hansson).
