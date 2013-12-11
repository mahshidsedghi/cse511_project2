#include "config.hh"
#include <stdlib.h>

int pfs_create(const char *filename, int stripe_width);
int pfs_open(const char *filename, const char mode);
ssize_t pfs_read(int filedes, void *buf, size_t nbyte, off_t offset, int *cache_hit);
ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit);
int pfs_close(int filedes);
int pfs_delete(const char *filename);
int pfs_fstat(int filedes, struct pfs_stat *buf); // Check the config file for the definition of pfs_stat structure

