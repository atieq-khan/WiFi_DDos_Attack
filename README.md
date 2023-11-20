WiFi DDos Attack using ESP32
====================

This project is build for myself and for other that are entering in the cybersecurity or Embedded systems developmenter to improve your skills.
```This project should not be used in wrong ways!!!```

In This project, we can do this these
1) Scan for Wifi both Hidden and non-hidden.
2) Performing DDos Attack on WiFi.
3) Refreash the Wifi list.

## Usage:
build the file.
``` 
idf.py build
```

After that flash it.
``` 
idf.py -p COMx flash monitor
```
the x is for COM number.

connect yout laptop or moblie with ESP32.
and go to:
``` 
192.168.4.1/
```
The html page will be open and list of WiFi and attack option will be displayed.
