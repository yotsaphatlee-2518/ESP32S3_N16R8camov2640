![image alt](https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640/blob/74446f2b24ace2f015ff6b2798d865e3bbcb936a/PIC/1747295280793.jpg)
### This project is a complete streaming display project using esp32-s3 with ov2640 camera which has wifimanager + mqtt system and is ready to connect to node-red..

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |
| **Tested On Targets** |  &#10060; Not working every ESP32|&#10060;|&#9989;|&#9989;|  &#10060; |  &#10060; | &#10060;|&#10060;|

### Usage, you just clone this link https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640.git and put it in C:\Users\xxx\esp\esp-idf and run it like this.
    1.C:\Users\xxx\esp\esp-idf>cd ESP32S3_N16R8camov2640
    2.idf.py set-target esp32s3
    3.idf.py menuconfig
    4.idf.py build
    5.idf.py -p COMxx flash monitor

### Device Connects AP 192.168.0.24  

Device Connects AP via information from HTTP server and when connection establishes, the device automatically saves the creds to the NVS and closes the HTTP Server. After any restart device first look nvs and accordingly the result from nvs re-opens HTTP server for new connection or doesn't open any task and establish connection using creds from nvs. It can connect any open-wifi network except whose have a captive portal to do it. Only you can do for it now giving any password info to device and clicks connect. ( Js side is already knows AP authmode but I don't update web side for a long time because of to-do list of project)
![image alt](https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640/blob/74446f2b24ace2f015ff6b2798d865e3bbcb936a/PIC/1747296423398.jpg)


Whole infrastructure is event-driven and it should have minimal cpu time, also it uses only %30 percent of I/SRAM of ESP32-C3-N16R8 CAMERA OV2640

If anyone wants to change any parameter of wifiManager, can use menuconfig every parameter of application is defined in KConfig file.

Now, you can choose two types of SSID for your ESP32:
- You can use dynamic naming with a prefix. Dynamic means, it differs from device to device because it makes SSID postfix is unique MAC of used esp32.
- You can use static ssid, this feature is the default.

## __Usage__

Due to its a first stable version; you should config some parameters of your menuconfig with your own, these parameters are:
- You should set __Max HTTP URI Length__ to 1024.
- If you have 4 MB flash, you set __Partition Table__ to "Single factory app (large), no OTA".

After that, anyone wants to use this project just copy and paste "components" folder to your project and add this ```set(EXTRA_COMPONENT_DIRS components/)```this to the main CMake project.

Your CMake is should looking like this ,after:
```
# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS components/) # Add the components directory to the project
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP32_WifiManager) # This is unique to your project.
```

After this steps you can just call ´´´wifiManager.h´´´ from any file and call ´´´wifiManager_init();´´´ function, also you can look at main part of project to see. 

Also you can just download whole version and send it to the device.  

## Minimum Requirements:
Any esp32 with 4 MB flash and minimum 190 KB SRAM

 ## Cases: 
- If NVS creds found when opening, only opens relevant tasks and not opens or initializes HTTP related tasks. When device disconnects, firstly saved creds cleared out and opens HTTP and related tasks.
- If NVS creds not found opens all tasks of device and waits for connection request from web server. When establishes, saves creds and closes HTTP server and related tasks and queues. If disconnects, deletes creds and restarts esp32.

****

## Active To-do List :
- Dynamic AP name. &#x2705;
- Button Interrupt for deleting creds. (Choosable) &#9989;
- mDNS.

## Missing :
- HTTP User Experience is in pre-release version,  not handling every command I sent from Esp32 with not good looking user interface. Maybe I won't update this for a long time due to long to-do list of this project.
- Manual Connect button is not working, it is now a test button for dynamically creates new buttons with NULL ssid and other infos. (It's not a bug, It's a feature)

## Known Bugs :
- When a user attempts to connect to an Access Point (AP) without providing a password, the system autonomously identifies the AP as an open network. If the AP is not open and requires authentication, the ESP enters a panic state.


