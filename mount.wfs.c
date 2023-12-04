#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wfs.h"

// Define your filesystem operation functions here
static int wfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        // Read the log entries from disk to find the file or directory
        // Fill in the stat structure with data from the inode

        // If file/directory not found, return -ENOENT
    }

    return 0;
}

static int wfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    // Check if the file already exists
    // If not, create a new inode and log entry for the file
    // Append the log entry to the disk
    // Update the parent directory's log entry to include this new file
    return 0; // or appropriate error code
}

static int wfs_mkdir(const char *path, mode_t mode) {
    // Similar to wfs_mknod, but for directories
    // Remember to set the mode of the inode to indicate a directory (S_IFDIR)
    return 0; // or appropriate error code
}

static int wfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Locate the file's inode using the path
    // Append a new log entry with the updated file content
    return size; // return the number of bytes written or an error code
}

static int wfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Locate the file's inode and its log entry
    // Read the content into buf
    return size; // return the number of bytes read or an error code
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    // Locate the directory's inode and its log entry
    // For each entry in the directory, use filler() to add it to the list
    return 0; // or appropriate error code
}

static int wfs_unlink(const char *path) {
    // Locate the file's inode
    // Mark the file's inode as deleted in a new log entry
    // Update the parent directory's log entry to remove this file
    return 0; // or appropriate error code
}



static struct fuse_operations wfs_oper = {
    .getattr = wfs_getattr,
    .mknod = wfs_mknod,
    .mkdir = wfs_mkdir,
    .write = wfs_write,
    .read = wfs_read,
    .readdir = wfs_readdir,
    .unlink = wfs_unlink,
    // Add other operations (read, write, mkdir, etc.)
};

int main(int argc, char *argv[]) {
    // Process arguments and initialize your filesystem structure

    return fuse_main(argc, argv, &wfs_oper, NULL);
}
