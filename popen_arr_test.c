#include <stdio.h>
#include "popen_arr.h"

int main(int argc, char* argv[]) {
    FILE *in, *out;
    int ret;
    ret = popen2_arr_p(&in, &out,  argv[1], (const char**)argv+1, NULL);
    fprintf(stderr, "Child pid: %d\n", ret);
    
    char buf[4096];
    fgets(buf, 4096, stdin);
    fprintf(in, "%s", buf);
    fclose(in);
    fgets(buf, 5096, out);
    printf("%s",buf);
    return 0;
}
