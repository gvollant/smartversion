

fart -C %1 "md5\md5-ms-amd64-v2.obj" "..\..\src\lib\hash\md5-ms-amd64-v2.obj"
fart -C %1 "md5\\md5-ms-amd64-v2.obj" "..\\..\\src\\lib\\hash\\md5-ms-amd64-v2.obj"
fart -C %1 "md5\\x86_64cpuid.obj" "..\\..\\src\\lib\\hash\\x86_64cpuid.obj"
fart -C %1 "md5\\md5-x86_64.obj" "..\\..\\src\\lib\\hash\\md5-x86_64.obj"
fart -C %1 "md5\\sha1-x86_64.obj" "..\\..\\src\\lib\\hash\\sha1-x86_64.obj"
fart -C %1 "md5\\sha256-x86_64.obj" "..\\..\\src\\lib\\hash\\sha256-x86_64.obj"
fart -C %1 "..\\..\\src\\lib\\hash\\md5-ms-amd64-v2.obj" "..\\..\\src\\lib\\hash\\md5-x86_64.obj"


fart -C  %1 "..\..\src\lib\hash\md5-x86_64.obj;..\..\src\lib\hash\x86_64cpuid.obj;..\..\src\lib\hash\sha1-x86_64.obj;..\..\src\lib\hash\md5-x86_64.obj" "..\..\src\lib\hash\x86_64cpuid.obj;..\..\src\lib\hash\sha1-x86_64.obj;..\..\src\lib\hash\md5-x86_64.obj"
fart -C  %1 "..\\..\\src\\lib\\hash\\md5-x86_64.obj;..\\..\\src\\lib\\hash\\x86_64cpuid.obj;..\\..\\src\\lib\\hash\\sha1-x86_64.obj;..\\..\\src\\lib\\hash\\md5-x86_64.obj" "..\\..\\src\\lib\\hash\\x86_64cpuid.obj;..\\..\\src\\lib\\hash\\sha1-x86_64.obj;..\\..\\src\\lib\\hash\\md5-x86_64.obj"

