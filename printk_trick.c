#define MOD_AUTH "\1MOD_AUTH"
#define MOD_FILE "\1MOD_FILE"

#define LVL_CRITICAL "\1LVLC"
#define LVL_ERROR    "\1LVLE"
#define LVL_NORMAL   "\1LVLN"

logmsg("a simple message, just like printf\n");

logmsg(LVL_CRITICAL "a critical message, no specific module\n");

logmsg(LVL_AUTH "a auth message, no specific level\n");

logmsg(LVL_FILE LVL_ERROR "a error message from the file subsystem\n");

