Windows Live Messenger History Import Plugin
Imports messages from Windows Live Messenger
Copyright (C) 2008  Very Crazy Dog (VCD)

Tested on Miranda IM 0.7.7 on Windows XP
Tested with Windows Live Messenger 8.1 and 8.5 database

This plugin is released under GPL.



Acknowledge
===========
This plugin is based on import.dll plugin that is developed by Miranda team.
This plugin use Parsifal XML Parser 1.0 for parsing XML.
Website of Parsifal XML Parser: http://www.saunalahti.fi/~samiuus/toni/xmlproc/



Current Limitation
==================
- Not able to handle contacts with the same email account name but with 
  different email provider. For example, "ABC@yahoo.com" and "ABC@gmail.com".
  In such cases, the message history from "ABC@yahoo.com" and "ABC@gmail.com"
  will be mixed and imported to both contacts in Miranda IM.

- Due to the limited information stored on the history database, the plugin is
  unable to handle message event if the name of sender and the name of receiver
  are the same.

- Not able to import chat log (message to multiple receivers).

- Not able to handle corrupted database.



Installation
============
1. Copy wlmHistoryImport.dll to Miranda IM plugin folder
2. Copy parsifal.dll to the folder where miranda32.exe is located (Miranda IM root folder)
3. Backup your current profile
4. Run Miranda IM and use the plugin :)
