/*
  a replacement for opendir/readdir/telldir/seekdir/closedir for BSD systems
  
*/

struct dir_buf;

struct dir_buf *rep_opendir(const char *dname);
struct dirent *rep_readdir(struct dir_buf *d);
off_t rep_telldir(struct dir_buf *d);
void rep_seekdir(struct dir_buf *d, off_t ofs);
int rep_closedir(struct dir_buf *d);

/* and for compatibility ... */
#define DIR struct dir_buf
#define opendir rep_opendir
#define readdir rep_readdir
#define telldir rep_telldir
#define seekdir rep_seekdir
#define closedir rep_closedir
