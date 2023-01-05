for %%n in (*586*.asm) do nasm -fwin32 %%n
nasm -fwin32 x86cpuid.asm
for %%n in (*64*.asm) do nasm -f win64 -DNEAR -Ox -g  %%n

