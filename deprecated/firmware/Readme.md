BUILD
=====


In build after cmake .. and the device is plugged into a usb.
``` shell
ninja; if [ $? -eq 0 ]; then (sudo picotool load -f Firmware.uf2) fi
```

Coding Practices
================

* language should only be the subset of c99 that is most portable, obviously loading extensions and other code to make things work.

* This is a unified build
	Don't split things into other translation units (h+c object combo), If you must because it is something that must be linked into somewhere else durring link time(This should never be the case almost ever.), then the STB style of creating a single translation unit in the .h file should be fine.

* Avoid any pico specific features that can harm porting LIBRARY code in the future.

* No malloc no free.


