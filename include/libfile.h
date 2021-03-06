#ifndef __LIBFILE_H
#define __LIBFILE_H

int write_full(int fd, const void *buf, size_t size);
int read_full(int fd, void *buf, size_t size);

char *read_file_line(const char *fmt, ...);

int memcpy_interruptible(void *dst, const void *src, size_t sz);

void *read_file(const char *filename, size_t *size);

int read_file_2(const char *filename, size_t *size, void **outbuf,
		loff_t max_size);

int write_file(const char *filename, void *buf, size_t size);

int copy_file(const char *src, const char *dst, int verbose);

int copy_dir (const char *src, const char *dst, int verbose);

int copy_recursive(const char *src, const char *dst);

int compare_file(const char *f1, const char *f2);

int open_and_lseek(const char *filename, int mode, loff_t pos);

/* Create a directory and its parents */
int make_directory(const char *pathname);

int unlink_recursive(const char *path, char **failedpath);

#endif /* __LIBFILE_H */
