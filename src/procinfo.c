#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

static void usage(const char *a){
    fprintf(stderr,"Usage: %s <pid>\n",a);
     exit(1);
}

static int isnum(const char*s){
    for(;*s;s++) if(!isdigit(*s)) return 0; 
    return 1;
}



int main(int c,char**v){
 if(c!=2||!isnum(v[1])) usage(v[0]);
 printf("TODO: implement procinfo\n");
 return 0;
}
