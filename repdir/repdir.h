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
#define opendir(d) rep_opendir(d)
#define readdir(d) rep_readdir(d)
#define telldir(d) rep_telldir(d)
#define seekdir(d, ofs) rep_seekdir(d, ofs)
#define closedir(d) rep_closedir(d)
