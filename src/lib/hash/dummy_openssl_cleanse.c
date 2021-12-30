#ifdef DUMMY_OPENSSL_CLEANSE
#include <stdlib.h>
#include <string.h>


void OPENSSL_cleanse(void *ptr, size_t len)
{
}
#endif
