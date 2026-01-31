#include <string.h>
#include "maxcfg.h"
#include "ui.h"

static const PickerOption command_options[] = {
    { "Chat_CB", "Invoke internal multinode chat facility in CB simulator mode. See section 7.2 for more information.", "Chat" },
    { "Chat_Page", "Prompt user for node number to page, send chat request, then place user in multinode chat until other user responds. See section 7.2.", "Chat" },
    { "Chat_Pvt", "Invoke internal multinode chat in private chat mode. See section 7.2 for more information.", "Chat" },
    { "Chat_Toggle", "Toggle user's multinode chat availability flag.", "Chat" },
    { "Chg_Alias", "Change user's alias to new value. New alias must not conflict with name or alias of any other user on system.", "Change" },
    { "Chg_Archiver", "Select default compression method for downloading QWK packets. Normally user is prompted for every download; this suppresses prompt.", "Change" },
    { "Chg_City", "Change user's city to a new value.", "Change" },
    { "Chg_Clear", "Toggle user's clearscreen setting.", "Change" },
    { "Chg_Editor", "Toggle user's full-screen editor flag. Toggles between line editor and full-screen MaxEd editor.", "Change" },
    { "Chg_FSR", "Toggle full-screen reader flag. When enabled, attractive message header displayed to ANSI/AVATAR users with all fields at once.", "Change" },
    { "Chg_Help", "Change user's help level. Maximus supports novice, regular and expert help levels.", "Change" },
    { "Chg_Hotkeys", "Toggle user's hotkeys setting. With hotkeys enabled, menu commands execute as soon as key is pressed without waiting for Enter.", "Change" },
    { "Chg_IBM", "Toggle IBM extended ASCII flag. Controls whether Maximus sends high-bit characters directly or translates to ASCII equivalents.", "Change" },
    { "Chg_Language", "Select new language file. Any language files specified in language control file can be selected here.", "Change" },
    { "Chg_Length", "Change user's screen length to a new value.", "Change" },
    { "Chg_More", "Toggle display of 'More [Y,n,=]?' prompts.", "Change" },
    { "Chg_Nulls", "Change number of NUL characters sent after every transmitted line.", "Change" },
    { "Chg_Password", "Change user's password to a new value.", "Change" },
    { "Chg_Phone", "Change user's telephone number.", "Change" },
    { "Chg_Protocol", "Select new default protocol. Normally Maximus prompts for protocol for every upload/download; default suppresses this prompt.", "Change" },
    { "Chg_Realname", "This option is obsolete.", "Change" },
    { "Chg_RIP", "Toggle user's RIPscrip graphics setting. Enabling RIPscrip also forces ANSI graphics and screen clearing to be enabled.", "Change" },
    { "Chg_Tabs", "Toggle user's tab setting. Tells Maximus if it can transmit tab characters; if not, translates tabs to up to eight spaces.", "Change" },
    { "Chg_Userlist", "Toggle 'display in userlist' flag. If no, user never displayed in userlist. If yes, may be displayed depending on Flags Hide attribute.", "Change" },
    { "Chg_Video", "Select new video mode. Maximus supports TTY, ANSI and AVATAR video modes.", "Change" },
    { "Chg_Width", "Change user's screen width to a new value.", "Change" },
    { "Clear_Stacked", "Clear user's command-stack buffer. Eliminates any leftover input in keyboard input buffer. Normally linked with another option.", "System" },
    { "Display_File", "Display .bbs file specified by filespec argument. Supports external program translation chars (use + not % in filenames).", "System" },
    { "Display_Menu", "Display menu specified by name argument (no path/extension). Flat call - called menu doesn't implicitly know how to return.", "System" },
    { "Edit_Abort", "Abort entry of current message. Only valid within line editor.", "Editor" },
    { "Edit_Continue", "Append to message in line editor, or return to full-screen MaxEd display. Only valid within editor menus.", "Editor" },
    { "Edit_Delete", "Delete a line from message in line editor. Only valid within line editor.", "Editor" },
    { "Edit_Edit", "Modify one line within message. Only valid within line editor.", "Editor" },
    { "Edit_From", "Edit From: field of message. Only valid within editor menus.", "Editor" },
    { "Edit_Handling", "Modify message attributes. Should only be accessible to SysOp. Only valid within editor menus.", "Editor" },
    { "Edit_Insert", "Insert a line into message. Only valid within line editor.", "Editor" },
    { "Edit_List", "List lines in current message. Only valid within line editor.", "Editor" },
    { "Edit_Quote", "Copy text from message user is replying to and place in current message. Only valid within line editor.", "Editor" },
    { "Edit_Save", "Save message and place in message base. Only valid within line editor.", "Editor" },
    { "Edit_Subj", "Edit Subject: field of message. Only valid within editor menus.", "Editor" },
    { "Edit_To", "Edit To: field of message. Only valid within editor menus.", "Editor" },
    { "File_Area", "Prompt user to select new file area.", "File" },
    { "File_Contents", "Display contents of compressed file. Supports ARC, ARJ, PAK, ZIP, and LZH compression methods.", "File" },
    { "File_Download", "Download (send) file from file areas.", "File" },
    { "File_Hurl", "Move file from one file area to another.", "File" },
    { "File_Kill", "Delete file from current file area.", "File" },
    { "File_Locate", "Search all file areas for file with certain filename or description. Can also display list of new files.", "File" },
    { "File_NewFiles", "Search for new files in all file areas. Identical to [newfiles] MECCA token.", "File" },
    { "File_Override", "Temporarily change download path for current area. Allows SysOp to access any directory as if it were normal file area.", "File" },
    { "File_Raw", "Display raw directory of current file area. Shows all files in directory, not just those in files.bbs.", "File" },
    { "File_Tag", "Add (tag) file and place in download queue. Tagged files can be downloaded later using File_Download command.", "File" },
    { "File_Titles", "Display list of all files in current area including name, timestamp and description. Reads files.bbs for current area.", "File" },
    { "File_Type", "Display contents of ASCII text file in current file area.", "File" },
    { "File_Upload", "Upload (receive) file. Maximus can automatically exclude specific file types using badfiles.bbs exclusion list.", "File" },
    { "Goodbye", "Log off the system.", "System" },
    { "Key_Poke", "Insert keystrokes into user's keystroke buffer. Maximus behaves as if user entered keystrokes manually. Replace spaces with underscores.", "System" },
    { "Leave_Comment", "Place user in message editor and address message to SysOp. Message placed in area defined by Comment Area in system control file.", "System" },
    { "Link_Menu", "Nest menus. When menu is displayed, Return option can return to calling menu. Supports up to eight nested Link_Menu options.", "System" },
    { "MEX", "Execute MEX script. Argument is script filename without extension.", "System" },
    { "Msg_Area", "Prompt user to select new message area.", "Message" },
    { "Msg_Browse", "Invoke Browse function. Allows user to selectively read, list, or pack messages for QWK. Select by area, type, and search criteria.", "Message" },
    { "Msg_Change", "Modify existing message. Only allows modifying messages not read by addressee, scanned as EchoMail, or sent as NetMail.", "Message" },
    { "Msg_Checkmail", "Invoke built-in mailchecker. Equivalent to [msg_checkmail] MECCA token.", "Message" },
    { "Msg_Current", "Redisplay current message.", "Message" },
    { "Msg_Download_Attach", "Lists unreceived file attaches addressed to current user. Prompts user to download each file attach.", "Message" },
    { "Msg_Edit_User", "Invoke user editor and automatically search on user in From: field of current message. Normally only accessible to SysOp.", "Message" },
    { "Msg_Enter", "Create new message in current area.", "Message" },
    { "Msg_Forward", "Forward copy of current message to another user.", "Message" },
    { "Msg_Hurl", "Move message from one area to another.", "Message" },
    { "Msg_Kill", "Delete message from current area. Message must be To: or From: current user (or user has MailFlags ShowPvt attribute).", "Message" },
    { "Msg_Kludges", "Toggle display of <ctrl-a> kludge lines at beginning of messages. Lines hold routing/control info. Normally only for SysOp.", "Message" },
    { "Msg_Reply", "Reply to current message. Reply placed in current area.", "Message" },
    { "Msg_Reply_Area", "Reply to current message in another area. If area specified, reply placed there; if . specified, user prompted for area.", "Message" },
    { "Msg_Restrict", "Limit QWK message downloads by message date. Users set date so messages prior to date not downloaded. Current session only.", "Message" },
    { "Msg_Tag", "Tag (select) specific message areas of interest. List of tagged areas can be recalled when using Msg_Browse command.", "Message" },
    { "Msg_Unreceive", "Unreceive current message. Resets received flag regardless of message author. Normally only available to SysOp.", "Message" },
    { "Msg_Upload", "Create message by uploading file rather than invoking internal editors. Prompts for header info, then prompts to upload ASCII file.", "Message" },
    { "Msg_Upload_QWK", "Upload .rep reply packet generated by QWK off-line reader.", "Message" },
    { "Msg_Xport", "Export message to ASCII text file. Allows explicit path/filename, so only SysOp should access. Use filename 'prn' to print.", "Message" },
    { "Press_Enter", "Set text color to white and prompt user to press Enter. Normally linked with another menu option.", "System" },
    { "Read_DiskFile", "Import ASCII text file from local disk and place in current message. Only from editor menus. Only for SysOp (allows explicit path).", "Read" },
    { "Read_Individual", "Read specific message number in current area.", "Read" },
    { "Read_Next", "Read next message in current area.", "Read" },
    { "Read_Nonstop", "Display all messages in area without pausing. If last command was Read_Next, forward order; if Read_Previous, reverse order.", "Read" },
    { "Read_Original", "Read original message in current thread. See also Read_Reply.", "Read" },
    { "Read_Previous", "Read previous message in current area.", "Read" },
    { "Read_Reply", "Display next message in current thread. See also Read_Original.", "Read" },
    { "Return", "Return from menu that was called with Link_Menu option.", "System" },
    { "Same_Direction", "Continue reading messages in same direction as specified by last Read_Previous or Read_Next menu option.", "Read" },
    { "Statistics", "Display system statistics and information.", "System" },
    { "User_Editor", "Invoke system user editor. Should only be accessible to SysOp.", "System" },
    { "Userlist", "Display list of all users on system. Users who disable 'In UserList' not displayed. Users in classes with Flags Hide never displayed.", "System" },
    { "Version", "Display Maximus version number and credit information. Prompts user to press Enter after displaying screen.", "System" },
    { "Who_Is_On", "On multinode system, displays names, task numbers, and status of users who are logged on.", "System" },
    { "Xtern_Dos", "Run external program specified by cmd. If cmd has arguments, replace all spaces with underscores. See section 6 for more info.", "External" },
    { "Xtern_Erlvl", "Exit to DOS with specified errorlevel. If cmd has arguments, replace spaces with underscores. See section 6 for more info.", "External" },
    { "Xtern_Run", "Run external program with I/O redirection specified by cmd. Replace spaces with underscores. See section 6 for more info.", "External" },
    { "Yell", "Page system operator. Maximus will play one of yell tunes if yelling currently enabled.", "System" },
    { NULL, NULL, NULL }
};

int command_picker_show(int current_idx)
{
    int num_options = 0;
    while (command_options[num_options].name) {
        num_options++;
    }
    
    return picker_with_help_show("Select Command", command_options, num_options, current_idx);
}

const char *command_picker_get_name(int index)
{
    int count = 0;
    while (command_options[count].name) {
        if (count == index) {
            return command_options[count].name;
        }
        count++;
    }
    return NULL;
}
