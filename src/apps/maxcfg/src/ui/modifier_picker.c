#include <string.h>
#include "maxcfg.h"
#include "ui.h"

static const PickerOption modifier_options[] = {
    { "(None)", "No modifier. Option displays normally for all users.", NULL },
    { "NoDsp", "Don't display on menu (hidden). Useful for creating hotkey bindings without showing the option to users.", NULL },
    { "Ctl", "Produce .CTL file for external command. Used with some external menu actions.", NULL },
    { "NoCLS", "Don't clear screen before displaying menu. Only used with Display_Menu command.", NULL },
    { "RIP", "Display this option only to users with RIPscrip graphics enabled.", NULL },
    { "NoRIP", "Display this option only to users without RIPscrip graphics.", NULL },
    { "Then", "Execute only if the last IF condition was true.", NULL },
    { "Else", "Execute only if the last IF condition was false.", NULL },
    { "Stay", "Don't perform menu cleanup; remain in current menu context.", NULL },
    { "UsrLocal", "Display this option only when user is logged in at the local console (SysOp).", NULL },
    { "UsrRemote", "Display this option only when user is logged in remotely.", NULL },
    { "ReRead", "Re-read LASTUSER.BBS after running external command. Used with Xtern_Dos and Xtern_Run.", NULL },
    { "Local", "Display this option only in areas with Style Local attribute.", NULL },
    { "Matrix", "Display this option only in areas with Style Net attribute.", NULL },
    { "Echo", "Display this option only in areas with Style Echo attribute.", NULL },
    { "Conf", "Display this option only in areas with Style Conf attribute.", NULL },
    { NULL, NULL, NULL }
};

int modifier_picker_show(int current_idx)
{
    int num_options = 0;
    while (modifier_options[num_options].name) {
        num_options++;
    }
    
    return picker_with_help_show("Select Modifier", modifier_options, num_options, current_idx);
}

const char *modifier_picker_get_name(int index)
{
    int count = 0;
    while (modifier_options[count].name) {
        if (count == index) {
            return modifier_options[count].name;
        }
        count++;
    }
    return NULL;
}
