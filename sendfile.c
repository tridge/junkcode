#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>


 int copy_file(const char *src, const char *dest)
 {
 	int fd1, fd2;
 	int n;
        struct stat sb;
        off_t t;
 
 	fd1 = open(src, O_RDONLY);
 	if (fd1 == -1) return -1;

	unlink(dest);
	fd2 = open(dest, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0666);
	if (fd2 == -1) {
		close(fd1);
		return -1;
	}
 
        fstat(fd1, &sb);
        t = 0;
        n = sendfile(fd2, fd1, &t, sb.st_size);
        if (n != sb.st_size) {
            close(fd2);
            close(fd1);
            unlink(dest);
            return -1;
        }
 
 	close(fd1);

	/* the close can fail on NFS if out of space */
	if (close(fd2) == -1) {
		unlink(dest);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
  return copy_file(argv[1], argv[2]);
}
