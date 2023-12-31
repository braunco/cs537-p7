NAME = mount.wfs mkfs.wfs fsck.wfs test_prebuilt_disk disk

CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=gnu18
FUSE_CFLAGS = `pkg-config fuse --cflags --libs`

.PHONY: all
all: $(NAME)

.PHONY: mount.wfs
mount.wfs:
	$(CC) $(CFLAGS) mount.wfs.c $(FUSE_CFLAGS) -o mount.wfs

.PHONY: mkfs.wfs
mkfs.wfs:
	$(CC) $(CFLAGS) -o mkfs.wfs mkfs.wfs.c

.PHONY: fsck.wfs
fsck.wfs:
	$(CC) $(CFLAGS) -o fsck.wfs fsck.wfs.c

.PHONY: clean
clean:
	rm -rf $(NAME)

test: mkfs.wfs mount.wfs
	~cs537-1/tests/P7/start.py
	
testl_CB: mkfs.wfs mount.wfs
	~/private/CS537/Project7/cs537-p7/tests/start.py

testl_TO: mkfs.wfs mount.wfs
	./tests/start.py

cltest: 
	rm disk

mtest: mkfs.wfs mount.wfs
	create_disk.sh
	mkfs.wfs disk
	mount.wfs -f -s disk mnt