#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_FILE_NAME_LEN 32

int main() {
  FILE *fp;
  fp = fopen("mnt/data1.txt", "w");
  if (fp < 0)
    return 0;

  else {
    fclose(fp);
    return 1;
  }
}