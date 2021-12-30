cp smv.cbp.mak a1

sed  's/g++/xlc++/g' a1 > a2
sed  's/gcc/xlc/g' a2 > a1
 
sed  's/Wunused\-parameter/DAIX -q64 -bmaxdata:0x800000000/g' a1 > a2
sed  's/Wall/D_LARGE_FILES -DAIX -q64 -bmaxdata:0x800000000/g' a2 > a1
sed  's/s -flto -static-libxlc -static-libstdc++ -Wl,--wrap=memcpy/s -q64 -bmaxdata:0x800000000 -lpthread /g' a1 > a2
sed  's/$(LDFLAGS/-q64 -bmaxdata:0x800000000 -lpthread $(LDFLAGS/g' a2 > a1
sed  's/ -static-libxlc -static-libstdc++ -Wl,--wrap=memcpy/ -q64 -bmaxdata:0x800000000 -lpthread /g' a1 > a2
sed  's/ldl -lresolv/ldl -q64 -bmaxdata:0x800000000/g' a2 > a1
cp a1 smv_aix.mak
rm a1
rm a2
echo now run one of these command:
echo make -f smv_aix.mak debug
echo make -f smv_aix.mak release
echo
