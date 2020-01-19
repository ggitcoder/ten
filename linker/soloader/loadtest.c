#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h> 
int (*printtest)();
int file_size(char* filename)
{
    struct stat statbuf;
    int ret;
    ret = stat(filename, &statbuf);
    if(ret!=0) return -1;
    return statbuf.st_size; 
}

void* m_load_library(char* name)
{
    int fd = open(name, O_CLOEXEC | O_RDWR);
    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    int load_size_ = file_size(name);
    void * addr = 0x8000;
    void* start = mmap(addr, load_size_, PROT_NONE, mmap_flags, -1, 0);
    int load_start_ = start;
    int load_bias_ = start - addr;
    void* seg_addr = mmap((void*)0,
                            0xb000,
                            PROT_WRITE|PROT_READ|PROT_EXEC,
                            MAP_PRIVATE,
                            fd,
                            0);  //将文件整个映射到内存
    printf("mmap addr is %x, value is %x\n", seg_addr, *((int *)seg_addr));
    return seg_addr;  
}


int main()
{
    void* base_addr = m_load_library("libhelloworld.so");
    void* phandle = dlopen("/system/lib/libc.so", RTLD_LAZY);
    if(phandle == 0)
    {
        printf("cannot found\n");
        return 0;
    }

    void* puts_addr = dlsym(phandle, "puts");
    printf("puts addr is %x\n", puts_addr);
    void* puts_plt_addr = base_addr + 0x2e0; //用ida预先看好
    printf("puts_plt value is 0x%x\n", *(int *)(base_addr + 0x2e0));
    printf("puts plt addr is %x\n", puts_plt_addr);
    *((int*)puts_plt_addr) = 0xe59ff000;
    *((int*)(puts_plt_addr + 4)) = puts_addr;
    *((int*)(puts_plt_addr + 8)) = puts_addr;  //修改plt，使他强行跳转到print函数那边
    printtest = (int(*)()) (base_addr + 0x328);  //用ida预先看好
    //printf("printtest value is 0x%x\n", *(int *)(base_addr + 0xccc));
    printtest();

    while(1){};
}
