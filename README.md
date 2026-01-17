# barosavnc
BarosaVNC is a remote VNC server connector and screen viewer. You can remote into public VNC servers and display the
viewport, send commands, send chat messages, and filter servers over the VNC protocol. Viewports can be sent to slack/discord/any other remote webhook embedding of your choice for status updates and other VNC server tasks.


## Quick start 
### Build
```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
```
### Usage
```sh
barosavnc COMMAND [OPTIONS]
  -C  <ip> [-p port] [-pw password] [-ss]  
  -F  <ips file> <-pwlist passwords.file> <-fmt format>
```
Try to connect to VNC server (port 5900 by default)
```sh
barosavnc -C 127.0.0.1 -pw 123456
barosavnc -C 127.0.0.1:5904 -pw 1111 -ss
```

Filter alive VNC servers using ip list in masscan's json format (use passwords file)
```sh
barosavnc -F masscan_ips.json -pwlist passwords.txt -fmt json
```

Filter VNCs in parallel
```sh
barosavnc -F ips.txt -fmt lines -pwlist passwords.txt -ct 1 -j 10
```
