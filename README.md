![image alt](https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640/blob/74446f2b24ace2f015ff6b2798d865e3bbcb936a/PIC/1747295280793.jpg)
### This project is a complete streaming display project using esp32-s3 with ov2640 camera which has wifimanager + mqtt system and is ready to connect to node-red..

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |
| **Tested On Targets** |  &#10060; Not working every ESP32|&#10060;|&#9989;|&#9989;|  &#10060; |  &#10060; | &#10060;|&#10060;|

### Structure of ESP32S3_N16R8camov2640  
![image alt](https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640/blob/74446f2b24ace2f015ff6b2798d865e3bbcb936a/PIC/1747297138379.jpg)

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

When run, the display will be displayed on the monitor.
![image alt](https://github.com/yotsaphatlee-2518/ESP32S3_N16R8camov2640/blob/52adb9e2e606f106fb6709ab0b59103b4542a99b/PIC/1747283546665.jpg
)



