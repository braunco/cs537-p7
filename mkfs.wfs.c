#include "wfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s disk_path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *disk = fopen(argv[1], "wb");
    if (!disk) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct wfs_sb sb = {
        .magic = WFS_MAGIC,
        .head = sizeof(struct wfs_sb) + sizeof(struct wfs_log_entry)
    };

    // Write superblock
    fwrite(&sb, sizeof(sb), 1, disk);

    // Setup root directory inode
    struct wfs_log_entry root_dir = {
        .inode = {
            .inode_number = 0,  // root directory inode number
            .mode = S_IFDIR,
            .links = 2
        }
    };

    // Write root directory log entry
    fwrite(&root_dir, sizeof(root_dir), 1, disk);

    fclose(disk);
    return 0;
}
