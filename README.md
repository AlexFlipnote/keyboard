# keyboard
Small Windows application I made to remap keyboard natively without the use of AHK (which was only installed for this).

## Build
```bat
make         :: list the targets below
make all     :: stop, rebuild and start it
make build   :: build bin\keyboard.exe (stops it first if it has to relink)
make kill    :: stop the running keyboard.exe
make clean   :: stop it and remove build output
```

## Run on startup
Press `Win + R`, type `shell:startup`, and put a shortcut to `keyboard.exe` in that folder.

## Stopping it
Right-click its icon in the system tray and choose **Exit**.

## Limitations
- Keys pressed inside apps running as administrator aren't remapped unless `keyboard.exe` also runs as administrator.
