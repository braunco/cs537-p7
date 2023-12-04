#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s disk_path\n", argv[0]);
        exit(1);
    }

    const char *disk_path = argv[1];
    FILE *disk = fopen(disk_path, "w");
    if (disk == NULL) {
        perror("fopen");
        exit(1);
    }

    // Initialize the superblock
    wfs_sb sb;
    sb.magic = 0xdeadbeef;
    sb.head = sizeof(wfs_sb);  // Assuming the first log entry starts right after the superblock

    if (fwrite(&sb, sizeof(wfs_sb), 1, disk) != 1) {
        perror("fwrite");
        fclose(disk);
        exit(1);
    }

    // Optionally zero out the rest of the disk file for a clean start
    // ...

    fclose(disk);
    return 0;
}
