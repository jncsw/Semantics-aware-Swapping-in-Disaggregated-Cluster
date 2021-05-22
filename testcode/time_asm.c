#include <stdio.h>
#include <time.h>

int main(){

    time_t tt;
    struct tm *t;
//    tt = time(NULL);
    asm volatile(
        "mov $0,%%ebx\n\t"
        "mov $0xd,%%eax\n\t"
        "int $0x80\n\t"
        "mov %%eax,%0\n\t"
        : "=m" (tt)
    );

    t = localtime(&tt);
    printf("time:%d:%d:%d:%d:%d:%d\n", t->tm_year+1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    return 0;

}
