#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wfs.h"

/* HELPER FUNCTIONS */
char* get_path_from_log_entry(const struct wfs_log_entry* entry) {
    static char path[MAX_FILE_NAME_LEN + 2]; // +2 for '/' and null-terminator
    if (entry->inode.inode_number == 0) {
        // Root directory
        strcpy(path, "/");
    } else {
        // File or directory directly under root
        // Assuming 'data' contains the name directly for simplicity
        snprintf(path, sizeof(path), "/%s", entry->data);
    }
    return path;
}


struct wfs_inode* find_inode_by_path(const char* path) {
    // Open the disk image file
    FILE *disk = fopen("/", "rb");
    if (!disk) {
        perror("Error opening disk image");
        return NULL;
    }

    // Read the superblock to find the starting point of the log entries
    struct wfs_sb sb;
    if (fread(&sb, sizeof(sb), 1, disk) != 1) {
        perror("Error reading superblock");
        fclose(disk);
        return NULL;
    }

    // Traverse the log entries to find the inode for the given path
    fseek(disk, sizeof(struct wfs_sb), SEEK_SET); // Start at the beginning of the log
    while (ftell(disk) < sb.head) {
        struct wfs_log_entry entry;
        if (fread(&entry, sizeof(entry), 1, disk) != 1) {
            perror("Error reading log entry");
            break; // or handle the error as needed
        }

        char *entry_path = get_path_from_log_entry(&entry); 
        if (entry_path != NULL && strcmp(entry_path, path) == 0) {
            struct wfs_inode* inode = malloc(sizeof(struct wfs_inode));
            if (inode != NULL) {
                *inode = entry.inode;
            }
            fclose(disk);
            return inode; // return a dynamically allocated inode
        }

        // Move to the next log entry if this one doesn't match
        // This may involve reading the size of the data part of the log entry
        // and seeking forward in the file by that amount
    }

    fclose(disk);
    return NULL; // Return NULL if the path's inode was not found
}


/* END HELPER FUNCTIONS */

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
        // Implement logic to find the inode for the given path
        struct wfs_inode *inode = find_inode_by_path(path);
        if (!inode) {
            return -ENOENT;  // No such file or directory
        }

        // Fill in the stat structure
        stbuf->st_mode = inode->mode;
        stbuf->st_nlink = inode->links;
        stbuf->st_size = inode->size;
        stbuf->st_uid = inode->uid;
        stbuf->st_gid = inode->gid;
        stbuf->st_atime = inode->atime;
        stbuf->st_mtime = inode->mtime;
        stbuf->st_ctime = inode->ctime;
        
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
    //need to filter out disk path

    //char* disk_path = argv[argc - 2];
    char* mount_point = argv[argc - 1];
    argc -= 1;
    argv[argc - 1] = mount_point;
    int result = fuse_main(argc, argv, &wfs_oper, NULL);

    return result;
}
