#include <syscall.h>
        
int 
main (void)
{
        const char *s = "Hello from user\n";
        write (1, s, 16);
        for (;;)
                ;
}
