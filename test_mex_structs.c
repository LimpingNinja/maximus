#include <stdio.h>
#include <stddef.h>
#include "prog.h"
#include "vm.h"
#include "mex_max.h"

int main() {
    printf("=== IADDR ===\n");
    printf("sizeof(IADDR) = %zu\n\n", sizeof(IADDR));
    
    printf("=== mex_usr struct ===\n");
    printf("sizeof(mex_usr) = %zu\n", sizeof(struct mex_usr));
    printf("  name:       offset=%zu size=%zu\n", offsetof(struct mex_usr, name), sizeof(IADDR));
    printf("  city:       offset=%zu\n", offsetof(struct mex_usr, city));
    printf("  alias:      offset=%zu\n", offsetof(struct mex_usr, alias));
    printf("  phone:      offset=%zu\n", offsetof(struct mex_usr, phone));
    printf("  pwd:        offset=%zu\n", offsetof(struct mex_usr, pwd));
    printf("  dataphone:  offset=%zu\n", offsetof(struct mex_usr, dataphone));
    printf("  msgs_posted:offset=%zu\n", offsetof(struct mex_usr, msgs_posted));
    printf("  xp_mins:    offset=%zu\n", offsetof(struct mex_usr, xp_mins));
    printf("  xkeys:      offset=%zu\n", offsetof(struct mex_usr, xkeys));
    printf("  up:         offset=%zu\n", offsetof(struct mex_usr, up));
    printf("  down:       offset=%zu\n", offsetof(struct mex_usr, down));
    printf("  downtoday:  offset=%zu\n", offsetof(struct mex_usr, downtoday));
    printf("  msg:        offset=%zu\n", offsetof(struct mex_usr, msg));
    printf("  files:      offset=%zu\n", offsetof(struct mex_usr, files));
    printf("  dob:        offset=%zu\n", offsetof(struct mex_usr, dob));
    printf("  nup:        offset=%zu\n", offsetof(struct mex_usr, nup));
    printf("  call:       offset=%zu\n", offsetof(struct mex_usr, call));
    
    printf("\n=== mex_ffind struct ===\n");
    printf("sizeof(mex_ffind) = %zu\n", sizeof(struct mex_ffind));
    printf("  finddata:   offset=%zu size=%zu\n", offsetof(struct mex_ffind, finddata), sizeof(void*));
    printf("  filename:   offset=%zu\n", offsetof(struct mex_ffind, filename));
    printf("  filesize:   offset=%zu\n", offsetof(struct mex_ffind, filesize));
    printf("  filedate:   offset=%zu\n", offsetof(struct mex_ffind, filedate));
    printf("  fileattr:   offset=%zu\n", offsetof(struct mex_ffind, fileattr));
    
    return 0;
}
