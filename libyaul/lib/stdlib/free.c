#include <sys/cdefs.h>

#include <internal.h>

void __weak
free(void *addr)
{
        _internal_user_free(addr);
}
