**Remotes known to be working**

![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/remotes/remote1.png "RFT Remote")

**Remotes known to be partly working**
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/remotes/remote2.png "RFT AUTO C02 (536-0150)")

Most commands from this remote are recognized but most notably not the join command. One button press on the button marked with "A" sends the command "medium", a double press on this button results in an unknown command.

For now, to manually add this remote:
1. Turn debug level1 on in a new tab: http://[nrg-itho-A1B2]/api.html?debug=level1
2. Go back to the main web interface of the add-on
3. Press  4 buttons at the same time to send a leave command
4. In the web interface there will be a debug message (blue box on the top), the remote ID and command type is shown, copy that ID. (press leave multiple times to have a little bit more time to copy an ID)
5. Go to: http://[nrg-itho-A1B2]/edit (username/pwd both admin)
6. Edit remotes.json and replace an empty slot ie.: {"index":0,"id":[0,0,0,0,0,0,0,0],"name":"remote"} with the remote ID ie.: {"index":0,"id":[106,153,102,170,166,102,169,170],"name":"remote"}
7. Save the file and reboot the add-on
8. Remote commands should be working now