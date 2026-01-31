#include <string.h>
#include "maxcfg.h"
#include "ui.h"

static const PickerOption privilege_options[] = {
    { "Twit", "Lowest access level. User has minimal privileges and restricted access to system features.", NULL },
    { "Disgrace", "Disgraced user. Limited access, typically used as punishment or probation status.", NULL },
    { "Limited", "Limited access user. More privileges than Twit but still restricted in many areas.", NULL },
    { "Normal", "Standard user access level. Default for regular callers with typical BBS privileges.", NULL },
    { "Worthy", "Above-average user. Trusted caller with additional privileges beyond normal users.", NULL },
    { "Privil", "Privileged user. Enhanced access to system features and areas.", NULL },
    { "Favored", "Favored user. High-trust level with extensive privileges and access.", NULL },
    { "Extra", "Extra privileges. Near-staff level access with special permissions.", NULL },
    { "Clerk", "Clerk level. Staff member with administrative duties and elevated access.", NULL },
    { "AsstSysop", "Assistant System Operator. Co-sysop with near-full system control.", NULL },
    { "Sysop", "System Operator. Full system access and control over all BBS operations.", NULL },
    { "Hidden", "Hidden access level. Special privilege level for testing or hidden features.", NULL },
    { NULL, NULL, NULL }
};

int privilege_picker_show(int current_idx)
{
    int num_options = 0;
    while (privilege_options[num_options].name) {
        num_options++;
    }
    
    return picker_with_help_show("Select Privilege Level", privilege_options, num_options, current_idx);
}

const char *privilege_picker_get_name(int index)
{
    int count = 0;
    while (privilege_options[count].name) {
        if (count == index) {
            return privilege_options[count].name;
        }
        count++;
    }
    return NULL;
}
