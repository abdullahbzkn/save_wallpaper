# save_wallpaper – Windows Wallpaper Saver Utility

`save_wallpaper.c` is a lightweight Windows utility that copies your **most recently added wallpaper** (from the system cache) — or, if none is found, your **currently active wallpaper** — into a dedicated folder on your Desktop.

---

## Background
I’ve been using Windows Spotlight for about seven months.
Windows Spotlight is a built-in Windows feature that automatically displays stunning, high-resolution wallpapers from Microsoft’s Bing service on your lock screen and desktop. These images change automatically — sometimes daily, sometimes every couple of days — featuring professional photographs from around the world.

On my computer, the wallpaper changes unpredictably, once or twice a day. Occasionally, I come across an image I really like — but saving it manually used to take time, as I had to locate the cache folder and copy the file myself.

That’s why I decided to create this small utility.
Now, with a single click on the save_wallpaper.exe shortcut on my desktop, I can instantly save the current or most recently added wallpaper into my personal archive folder without any manual effort.

---

## Features
- **Priority order:**
  1. The **latest added** image in `%APPDATA%\Microsoft\Windows\Themes\CachedFiles` (`CreationTime`).
  2. The **currently active wallpaper** path via `SystemParametersInfo(SPI_GETDESKWALLPAPER)`.
  3. Fallback: `%APPDATA%\Microsoft\Windows\Themes\TranscodedWallpaper`.
- Supported formats: `.jpg`, `.jpeg`, `.png`, `.bmp`, `.jfif`
- Automatically creates the target directory: `C:\Users\<User>\Desktop\savedwallpapers`
- File naming: `wallpaper_YYYY-MM-DD_HH-MM-SS.ext`  
  (Adds milliseconds to avoid filename collisions.)

---

## How It Works
1. **CachedFiles → latest added:** Scans all supported image files, checks their creation time (`ftCreationTime`), and picks the most recently added file.  
2. **Active wallpaper:** If no cached image is found, retrieves the current wallpaper path using `SPI_GETDESKWALLPAPER`.  
3. **TranscodedWallpaper fallback:** If both above fail, copies `%APPDATA%\Microsoft\Windows\Themes\TranscodedWallpaper`.  
4. **Copy:** Writes the selected file to `Desktop\savedwallpapers` with a timestamped name.

> Note: On some Windows builds, “creation time” better reflects when a cache file was added than “last write time,” so CreationTime is used to determine the latest item.

---

## Build Instructions

### 1) MinGW-w64 (MSYS2) – recommended
Prerequisite: MSYS2 with `mingw-w64-x86_64-gcc` installed.

```powershell
# Navigate to project folder (example)
cd "$env:USERPROFILE\Desktop"

# Compile
gcc -municode .\save_wallpaper.c -o .\save_wallpaper.exe -lole32 -lshell32 -luuid -luser32

# Run
.\save_wallpaper.exe
```

### 2) Visual Studio Developer Command Prompt
```bat
cl /W4 /DUNICODE /D_UNICODE save_wallpaper.c shell32.lib ole32.lib uuid.lib user32.lib
save_wallpaper.exe
```

---

## Usage

Simply run the executable:

```powershell
.\save_wallpaper.exe
```

### Example Output
```
[INFO] CachedFiles (latest added): C:\Users\...\AppData\Roaming\Microsoft\Windows\Themes\CachedFiles\road.jpg
[OK] C:\...\road.jpg → C:\Users\...\Desktop\savedwallpapers\wallpaper_2025-10-27_15-02-33.jpg
```

---

## Contributing
Contributions and issue reports are welcome!  
Testing across multiple Windows versions and setups (multi-monitor, slideshow, etc.) is highly appreciated.
