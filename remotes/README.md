**Remotes known to be working**

RFT Remote W (536-0124):
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/remotes/remote1.png "RFT Remote W (536-0124)")

**Remotes known to be partly working**
RFT AUTO C02 (536-0150):
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/remotes/remote2.png "RFT AUTO C02 (536-0150)")
A button press on the button marked with "A" sends the command "medium".

Since firmware version 2.0.4 the buttons of this remote are working. The RF performance seems to be less than with the older RFT Remote W (536-0124) remote that's why it is listed under "partly working". More in depth analysis is needed to find out what the reason is, probably the settings of the CC1101 chip need to be adjusted.


**Trying to add a non working remote manually**

1. Turn debug level1 on in a new tab: http://[nrg-itho-A1B2]/api.html?debug=level1
2. Go back to the main web interface of the add-on
3. Press 4 buttons at the same time to send a leave command (or try any other button)
4. In the web interface there will be a debug message (blue box on the top), the remote ID (and command type is shown), copy that ID. (press the button multiple times to have a little bit more time to copy an ID)
5. Go to: http://[nrg-itho-A1B2]/edit (username/pwd both admin)
6. Edit remotes.json and replace an empty slot ie.: {"index":0,"id":[0,0,0,0,0,0,0,0],"name":"remote"} with the remote ID ie.: {"index":0,"id":[106,153,102,170,166,102,169,170],"name":"remote"}
7. Save the file and reboot the add-on
8. Remote commands should be working now

==
To further debug command switch to debug level2: http://[nrg-itho-A1B2]/api.html?debug=level2
The needed command bytes to implement a non working button will now also be visible on the debug message

Debug level3 will show IDs and commands for all packets received, also non itho devices and can clutter the interface.