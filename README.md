# wlmHistoryImport
Windows Live Messenger History Import Plugin for [Miranda IM] (http://www.miranda-im.org/). Imports message history from Windows Live Messenger.

Tested on Miranda IM 0.8.1 on Windows XP with with Windows Live Messenger 8.1 and 8.5 message history.

# Acknowledge
This plugin is based on import.dll plugin that is developed by Miranda team.

# Current Limitation
- Not able to handle contacts with the same email account name but with 
  different email provider. For example, "ABC@yahoo.com" and "ABC@gmail.com".
  In such cases, the message history from both "ABC@yahoo.com" and 
  "ABC@gmail.com" will be mixed and imported to both contacts in Miranda IM.
- Due to the limited information stored in the message history XML files, the 
  plugin is unable to handle message event if the name of sender and the name of
  receiver are the same. In such cases, the event will be counted as failed
  import history event.
- Not able to import chat log (message to multiple receivers). The event counted
  as skipped chat log history event.
- Not able to handle corrupted message history XML files. The behavior of the 
  program is unexpected.

# Notice for user using wlmHistoryImport.dll 0.1.2.0 and below
There is a bug in previous version (0.1.2.0 and below) of the plugin in which 
all line ending characters becomes Unix convention LF instead of the Windows 
convention CRLF (I did not notice that because History++ plugin will display 
them correctly).

If users import WLM history using the new version (0.8.0.0 and above), they will
find that duplicated messages (but with different line ending characters) appear
in Miranda IM history and they are not able to be detected or removed by Miranda
Database Tool.

If you do not wish to see duplicated messages, you can convert LF to CRLF in all
MSN history events by running a separated plugin called LF to CRLF (LF2CRLF.dll)
written by me.

It is suggested to do the conversion before importing any WLM history using the
new version of the plugin. However if you forgotten to do that, or just saw this
notice, or looking for a solution for duplicated messages, you can still do the 
conversion, and then run the Miranda Database Tool to remove the duplicated 
messages in Miranda IM profile.

# Installation
1. Copy wlmHistoryImport.dll to Miranda IM plugin folder.
2. Backup your current profile.
3. Run Miranda IM and use the plugin. :)

# License
Copyright (C) 2008 - 2009  Very Crazy Dog (VCD)

This plugin is released under [GPL](http://www.gnu.org/licenses/gpl-3.0.en.html).
