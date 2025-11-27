#include <stdio.h>
#include <stddef.h>
#include "prog.h"
#include "vm.h"
#include "mex_max.h"

int main() {
    printf("IADDR size: %zu\n", sizeof(IADDR));
    printf("mex_usr size: %zu\n", sizeof(struct mex_usr));
    printf("\nField offsets in mex_usr:\n");
    
    // All string (IADDR) fields before dob
    printf("  name:       %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, name));
    printf("  city:       %3zu\n", offsetof(struct mex_usr, city));
    printf("  alias:      %3zu\n", offsetof(struct mex_usr, alias));
    printf("  phone:      %3zu\n", offsetof(struct mex_usr, phone));
    printf("  lastread:   %3zu (word, 2 bytes)\n", offsetof(struct mex_usr, lastread_ptr));
    printf("  pwd:        %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, pwd));
    printf("  times:      %3zu (word, 2 bytes)\n", offsetof(struct mex_usr, times));
    printf("  help:       %3zu (byte)\n", offsetof(struct mex_usr, help));
    printf("  video:      %3zu\n", offsetof(struct mex_usr, video));
    printf("  dataphone:  %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, dataphone));
    printf("  time:       %3zu (word)\n", offsetof(struct mex_usr, time));
    printf("  deleted:    %3zu (byte)\n", offsetof(struct mex_usr, deleted));
    printf("  msgs_posted:%3zu (dword, 4 bytes)\n", offsetof(struct mex_usr, msgs_posted));
    printf("  msgs_read:  %3zu (dword)\n", offsetof(struct mex_usr, msgs_read));
    printf("  width:      %3zu (byte)\n", offsetof(struct mex_usr, width));
    printf("  xp_priv:    %3zu (sword, 2 bytes)\n", offsetof(struct mex_usr, xp_priv));
    printf("  xp_date:    %3zu (stamp, 6 bytes)\n", offsetof(struct mex_usr, xp_date));
    printf("  xp_mins:    %3zu (dword, 4 bytes)\n", offsetof(struct mex_usr, xp_mins));
    printf("  sex:        %3zu (byte)\n", offsetof(struct mex_usr, sex));
    printf("  ludate:     %3zu (stamp, 6 bytes)\n", offsetof(struct mex_usr, ludate));
    printf("  xkeys:      %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, xkeys));
    printf("  lang:       %3zu (byte)\n", offsetof(struct mex_usr, lang));
    printf("  def_proto:  %3zu (byte)\n", offsetof(struct mex_usr, def_proto));
    printf("  up:         %3zu (dword, 4 bytes)\n", offsetof(struct mex_usr, up));
    printf("  down:       %3zu (dword)\n", offsetof(struct mex_usr, down));
    printf("  downtoday:  %3zu (dword)\n", offsetof(struct mex_usr, downtoday));
    printf("  msg:        %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, msg));
    printf("  files:      %3zu (string, 10 bytes)\n", offsetof(struct mex_usr, files));
    printf("  compress:   %3zu (byte)\n", offsetof(struct mex_usr, compress));
    printf("  dob:        %3zu (string, 10 bytes) <-- TARGET\n", offsetof(struct mex_usr, dob));
    printf("  date_1stcall:%3zu (stamp, 6 bytes)\n", offsetof(struct mex_usr, date_1stcall));
    printf("  nup:        %3zu (dword)\n", offsetof(struct mex_usr, nup));
    printf("  call:       %3zu (word, 2 bytes)\n", offsetof(struct mex_usr, call));
    
    return 0;
}
