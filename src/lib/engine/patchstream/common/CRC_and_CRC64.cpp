

#ifdef UNCOMPRESS_LZMASDK

#include "lzma_sdk/C/Alloc.h"

#include "lzma_sdk/C/7zCrc.h"
#include "lzma_sdk/C/XzCrc64.h"

struct CCRCTableInit_32 { CCRCTableInit_32() { CrcGenerateTable(); } } g_CRCTableInit_32;
struct CCRCTableInit_64 { CCRCTableInit_64() { Crc64GenerateTable(); } } g_CRCTableInit_64;
#endif
