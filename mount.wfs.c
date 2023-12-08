#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "wfs.h"
#include "time.h"

FILE *disk_image = NULL; // Global file descriptor for the disk image
const char *disk_image_path;
FILE * debug_log;
void *mapped_disk_image = NULL; 
size_t mapped_disk_image_size;

/* HELPER FUNCTIONS */


struct wfs_log_entry* get_root_entry() {
    char* currAddr = (char*)mapped_disk_image + sizeof(struct wfs_sb);
    struct wfs_log_entry* currLogEntry = (struct wfs_log_entry*)currAddr;
    struct wfs_log_entry* actualRootEntry = NULL;
    while(currAddr - (char*)mapped_disk_image < mapped_disk_image_size) {
        currLogEntry = (struct wfs_log_entry*)currAddr;

        if(currLogEntry->inode.inode_number == 0 && currLogEntry->inode.mode == 0x41ed) {
            actualRootEntry = currLogEntry;
        }

        currAddr += sizeof(struct wfs_inode) + currLogEntry->inode.size;
    }
    return actualRootEntry;
}

struct wfs_log_entry* get_entry_from_number(int inode_number) {
    struct wfs_sb* sb = (struct wfs_sb*)mapped_disk_image;
    int head = sb->head;
    char* currAddr = (char*)mapped_disk_image + sizeof(struct wfs_sb);
    struct wfs_log_entry* currLogEntry = (struct wfs_log_entry*)currAddr;
    struct wfs_log_entry* actualInode = NULL;
    while(currAddr - (char*)mapped_disk_image < mapped_disk_image_size && currAddr - (char*)mapped_disk_image < head) {
        currLogEntry = (struct wfs_log_entry*)currAddr;

        if(currLogEntry->inode.inode_number == inode_number) {
            actualInode = currLogEntry;
        }

        currAddr += sizeof(struct wfs_inode) + currLogEntry->inode.size;
    }

    return actualInode;
}

struct wfs_dentry* find_dentry(struct wfs_log_entry* entry, const char* name) {
    int numDentries = entry->inode.size / sizeof(struct wfs_dentry);
    if(numDentries == 0) return NULL; //return null if no entries, as cannot have file

    struct wfs_dentry* currDentry = (struct wfs_dentry*)entry->data;
    for(int i=0; i<numDentries; i++) {
        //printf("\tGoing through dentry#%d\n", i);
        //printf("\tComparing %s with %s...\n", currDentry->name, name);
        if(strcmp(currDentry->name, name) == 0) { //if this dentry matches path token we are looking for.
            //printf("\t\tMatched with inode #%d\n", get_entry_from_number(currDentry->inode_number)->inode.inode_number);
            return currDentry;
        }
        currDentry++;
    }
    return NULL;
}

int find_next_free_inode() {
    struct wfs_sb* sb = (struct wfs_sb*)mapped_disk_image;
    int head = sb->head;
    char* currAddr = (char*)mapped_disk_image + sizeof(struct wfs_sb);

    int numToReturn = 1;
    struct wfs_log_entry* currLogEntry;

    while(numToReturn <= 0xFFFFFFFF) {
        while(currAddr - (char*)mapped_disk_image < mapped_disk_image_size && currAddr - (char*)mapped_disk_image < head) {
            currLogEntry = (struct wfs_log_entry*)currAddr;

            if(currLogEntry->inode.inode_number == numToReturn && currLogEntry->inode.deleted != 1) {
                numToReturn++;
                break;
            }

            currAddr += sizeof(struct wfs_inode) + currLogEntry->inode.size;
        }
        return numToReturn;
    }
    return -1;
}

/* HELPER FUNCTIONS */
struct wfs_inode* find_inode_by_path(const char* path)
{
    /*
    breakdown path into string[] broken up by '/'
    get root inode, iterate through all directory entries, searching for string[0]
    repeat
    */

    //need to find newest root. iterate backwards
    struct wfs_log_entry* currLogEntry = get_root_entry();

    char pathCopy[strlen(path)];
    strcpy(pathCopy, path);


    char* token = strtok(pathCopy, "/");
    while(token) {
        //printf("In while Loop!\n");
        int numDentries = currLogEntry->inode.size / sizeof(struct wfs_dentry);
        if(numDentries == 0) return NULL; //return null if no entries, as cannot have file

        //struct wfs_dentry* currDentry = (struct wfs_dentry*)actualRootEntry->data;
        struct wfs_dentry* currDentry = find_dentry(currLogEntry, token);
        if(currDentry == NULL) {
            return NULL;
        }

        token = strtok(NULL, "/");
        if(token == NULL) { //case 1: it was the last thing
            return &(get_entry_from_number(currDentry->inode_number)->inode);
        } 
        else { //case 2: it was not the last thing
            currLogEntry = get_entry_from_number(currDentry->inode_number);
        }
        
    }

    return NULL;
}

// Thomas helper function for printing
void fprintInode(struct wfs_inode* inode) {
    fprintf(debug_log, "Thomas - inode number: %x\n", inode->inode_number);
    fprintf(debug_log, "Thomas - deleted: %x\n", inode->deleted);
    fprintf(debug_log, "Thomas - mode: %x\n", inode->mode);
    fprintf(debug_log, "Thomas - uid: %x\n", inode->uid);
    fprintf(debug_log, "Thomas - gid: %x\n", inode->gid);
    fprintf(debug_log, "Thomas - flags: %x\n", inode->flags);
    fprintf(debug_log, "Thomas - size: %x\n", inode->size);
    fprintf(debug_log, "Thomas - atime: %x\n", inode->atime);
    fprintf(debug_log, "Thomas - mtime: %x\n", inode->mtime);
    fprintf(debug_log, "Thomas - ctime: %x\n", inode->ctime);
    fprintf(debug_log, "Thomas - links: %x\n", inode->links);
    fprintf(debug_log, "Thomas - \n");
}

void printInode(struct wfs_inode* inode) {
    printf("Thomas - inode number: %x\n", inode->inode_number);
    printf("Thomas - deleted: %x\n", inode->deleted);
    printf("Thomas - mode: %x\n", inode->mode);
    printf("Thomas - uid: %x\n", inode->uid);
    printf("Thomas - gid: %x\n", inode->gid);
    printf("Thomas - flags: %x\n", inode->flags);
    printf("Thomas - size: %x\n", inode->size);
    printf("Thomas - atime: %x\n", inode->atime);
    printf("Thomas - mtime: %x\n", inode->mtime);
    printf("Thomas - ctime: %x\n", inode->ctime);
    printf("Thomas - links: %x\n", inode->links);
    printf("Thomas - \n");
}


static int wfs_getattr(const char *path, struct stat *stbuf) {
    fprintf(debug_log, "Debug - getattr called for path: %s\n", path);
    printf("\t\tCALLED getattr path: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));


    if (strcmp(path, "/") == 0) {
        fprintf(debug_log, "Debug - Path is root directory\n");
        stbuf->st_mode = S_IFDIR;
        stbuf->st_nlink = 2;
    } else {
        // Read the log entries from disk to find the file or directory
        // Fill in the stat structure with data from the inode

        // If file/directory not found, return -ENOENT
        // Implement logic to find the inode for the given path
        fprintf(debug_log, "Debug - Path is not root, searching for inode\n");
        struct wfs_inode *inode = find_inode_by_path(path);
        if (!inode) {
            fprintf(debug_log, "Debug - Inode not found for path: %s\n", path);
            return -ENOENT;  // No such file or directory
        }

        // Fill in the stat structure
        fprintf(debug_log, "Debug - Inode found for path: %s\n", path);
        stbuf->st_mode = inode->mode;
        stbuf->st_nlink = inode->links;
        stbuf->st_size = inode->size;
        stbuf->st_uid = inode->uid;
        stbuf->st_gid = inode->gid;
        stbuf->st_atime = inode->atime;
        stbuf->st_mtime = inode->mtime;
        stbuf->st_ctime = inode->ctime;

        fprintf(debug_log, "Debug - Filled state structure for path: %s\n", path);
        
    }
    fprintf(debug_log, "Debug - getattr returning success for path: %s\n", path);

    return 0;
}

void printDirectoryContents(struct wfs_log_entry* entry) {
    printf("#####PRINTING DIRECTORY CONTENTS#####\n");
    int numDentries = entry->inode.size / sizeof(struct wfs_dentry);
    if(numDentries == 0) {
        printf("#####END PRINT#####\n");
        return; //return null if no entries, as cannot have file
    }

    struct wfs_dentry* currDentry = (struct wfs_dentry*)entry->data;
    for(int i=0; i<numDentries; i++) {
        printf("%s: %ld\n", currDentry->name, currDentry->inode_number);
        currDentry++;
    }
    printf("#####END PRINT#####\n");
}

int copyDentries(struct wfs_log_entry* orig, struct wfs_log_entry* new) { //returns num of dentries present
    int numDentries = orig->inode.size / sizeof(struct wfs_dentry);
    struct wfs_dentry* currDentry = (struct wfs_dentry*)orig->data;
    struct wfs_dentry* newCurrDentry = (struct wfs_dentry*)new->data;
    for(int i=0; i<numDentries; i++) {
        memcpy((void*)newCurrDentry, (void*)currDentry, sizeof(struct wfs_dentry));
        currDentry++;
        newCurrDentry++;
    }
    return numDentries;
}

static int wfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    printf("\t\tCALLED mknod\n");
    fprintf(debug_log, "CURRENT PATH INSIDE OF WFS_MKNOD: %s\n", path);
    // Check if the file already exists
    struct wfs_inode *inode = find_inode_by_path(path);
    if (inode != NULL) {
        return -EEXIST;
    }
    // If not, create a new inode and log entry for the file
    else {
        struct wfs_sb* sb = (struct wfs_sb*)mapped_disk_image;
        int head = sb->head;
        struct wfs_log_entry* newLogEntry = (struct wfs_log_entry*)((char*)mapped_disk_image + head);
        struct wfs_inode* newInode = &newLogEntry->inode;
        int nextInode = find_next_free_inode();
        fprintf(debug_log, "THIS IS THE NEXT INODE: %d\n", nextInode);
        printInode(&newLogEntry->inode);

        newInode->inode_number = nextInode;
        newInode->deleted = 0;
        newInode->mode = mode;
        struct fuse_context * curr_fuse = fuse_get_context();
        newInode->uid = curr_fuse->uid;
        newInode->gid = curr_fuse->gid;
        newInode->flags = 0;
        newInode->size = 0;
        newInode->atime = time(NULL);
        newInode->mtime = time(NULL);
        newInode->ctime = time(NULL);
        newInode->links = 1;

        //printInode(&newLogEntry->inode);
        sb->head += sizeof(struct wfs_inode);
        mapped_disk_image_size += sizeof(struct wfs_inode);

        struct wfs_log_entry* rootLogEntry = get_root_entry();
        // printf("OLD ROOT ENTRY: \n");
        // printInode(&rootLogEntry->inode);
        struct wfs_inode* rootInode = &rootLogEntry->inode;
        rootInode->deleted = 1;
    
        struct wfs_log_entry* newRootEntry = (struct wfs_log_entry*)((char*)mapped_disk_image + sb->head);
        struct wfs_inode* newRootInode = &newRootEntry->inode;

        //copy over some stuff
        newRootInode->inode_number = rootInode->inode_number;
        newRootInode->deleted = 0;
        newRootInode->mode = rootInode->mode;
        newRootInode->uid = rootInode->uid;
        newRootInode->gid = rootInode->gid;
        newRootInode->flags = rootInode->flags;
        newRootInode->size = rootInode->size + sizeof(struct wfs_dentry);
        newRootInode->atime = time(NULL);
        newRootInode->mtime = time(NULL); //IDK IF THESE 3 SHOULD ALL BE MODIFIED BUT I THINK C FOR SURE
        newRootInode->ctime = time(NULL); 
        newRootInode->links = 1;

        int numCurrentDentries = copyDentries(rootLogEntry, newRootEntry);
        struct wfs_dentry* newDentry = (struct wfs_dentry*)newRootEntry->data;
        newDentry += (sizeof(struct wfs_dentry) * numCurrentDentries);
        
        char* pathCopy = malloc(sizeof(char) * MAX_FILE_NAME_LEN);
        memset(pathCopy, 0, MAX_FILE_NAME_LEN);
        memcpy(pathCopy, path, MAX_FILE_NAME_LEN);
        char* token = strtok(pathCopy, "/");
        char* last = malloc(sizeof(char) * MAX_FILE_NAME_LEN);
        memset(last, 0, MAX_FILE_NAME_LEN);
        memcpy(last, token, MAX_FILE_NAME_LEN);
        while(token) {
            memset(last, 0, MAX_FILE_NAME_LEN);
            memcpy(last, token, MAX_FILE_NAME_LEN);
            token = strtok(NULL, "/");
        }
        memcpy(newDentry->name, last, MAX_FILE_NAME_LEN);
        free(last);
        free(pathCopy);
        newDentry->inode_number = nextInode;

        sb->head += sizeof(struct wfs_dentry);
        mapped_disk_image_size += sizeof(struct wfs_inode);
    }
    // Append the log entry to the disk
    // Update the parent directory's log entry to include this new file
    return 0; // or appropriate error code
}

static int wfs_mkdir(const char *path, mode_t mode) {
    //printf("\t\tCALLED mkdir\n");
    // Similar to wfs_mknod, but for directories
    // Remember to set the mode of the inode to indicate a directory (S_IFDIR)
    return 0; // or appropriate error code
}

static int wfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    //printf("\t\tCALLED write\n");
    // Locate the file's inode using the path
    // Append a new log entry with the updated file content
    return size; // return the number of bytes written or an error code
}

static int wfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    //printf("\t\tCALLED read\n");
    // Locate the file's inode and its log entry
    struct wfs_inode *currentInode = find_inode_by_path(path);
    //fprintf(debug_log, "Current path: %s\n", path);
    //printInode(currentInode);


    struct wfs_log_entry * currEntry = get_entry_from_number(currentInode->inode_number);
    fprintf(debug_log, "Debug - current log entry data: %d and current size: %d\n", currEntry->inode.inode_number, currEntry->inode.size);
    size = currEntry->inode.size;
    
    // Read the content into buf
    memcpy(buf+offset, currEntry->data, sizeof(char)*size);
    
    return size; // return the number of bytes read or an error code
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    //printf("\t\tCALLED readdir\n");
    // Locate the directory's inode and its log entry
    // For each entry in the directory, use filler() to add it to the list
    return 0; // or appropriate error code
}

static int wfs_unlink(const char *path) {
    //printf("\t\tCALLED unlink\n");
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
    debug_log = fopen("debug_log.txt", "a");
    if (!debug_log) {
        perror("Error opening debug log file");
        return 1;
    }
    fprintf(debug_log, "Debug - Starting main function\n");

    if (argc < 4) {
        fprintf(stderr, "usage: %s <disk_image_path> <mount_point>\n", argv[0]);
        return 1;
    }

    disk_image_path = argv[argc - 2];
    fprintf(debug_log, "Debug - Disk image path: %s\n", disk_image_path);

    char *mount_point = argv[argc - 1];
    fprintf(debug_log, "Debug - Mount point: %s\n", mount_point);

    disk_image = fopen(disk_image_path, "rb+");
    if (!disk_image) {
        perror("Unable to open disk image");
        return 1;
    }

    fseek(disk_image, 0, SEEK_END);
    size_t size = ftell(disk_image);
    fseek(disk_image, 0, SEEK_SET);
    //fprintf(debug_log, "Debug - size: %ld\n", size);
    mapped_disk_image_size = size;

    mapped_disk_image = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(disk_image), 0);


    fprintf(debug_log, "Debug - Disk image opened successfully\n");

    char* new_argv[argc];
    for (int i = 0; i < argc - 2; ++i) {
        new_argv[i] = argv[i];
    }

    // Adjust arguments for fuse_main
    new_argv[argc - 2] = mount_point; // Set mount point as the second argument
    new_argv[argc - 1] = NULL; // Null-terminate argument list


    fprintf(debug_log, "Debug - Calling fuse_main\n");
    int result = fuse_main(argc - 1, new_argv, &wfs_oper, NULL);
    fprintf(debug_log, "Debug - fuse_main returned %d\n", result);

    fclose(disk_image);
    fprintf(debug_log, "Debug - Disk image closed\n");

    //munmap(address);

    fclose(debug_log);
    munmap(mapped_disk_image, size);
    fclose(disk_image);
    return result;
}
