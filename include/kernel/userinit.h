#ifndef BRIGHTS_USERINIT_H
#define BRIGHTS_USERINIT_H

/* Initialize VFS2 and user mode infrastructure */
void brights_userinit(void);

/* Enter user mode with embedded/shell init process */
void brights_userinit_enter(void);

#endif
