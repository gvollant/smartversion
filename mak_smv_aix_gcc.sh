cp smv.cbp.mak a1

#sed  's/g++/xlc++/g' a1 > a2
#sed  's/gcc/xlc/g' a2 > a1

sed  's/Wunused\-parameter/DAIX -Dint64=int64_t -Dloff_t=off64_t -maix64 -fPIC /g' a1 > a2
sed  's/Wall/D_LARGE_FILES -DAIX  -Dint64=int64_t -Dloff_t=off64_t -maix64 -fPIC /g' a2 > a1
sed  's/-flto/ -O3 /g' a1 > a2
sed  's/$(LDFLAGS/ -lpthread -maix64 $(LDFLAGS/g' a2 > a1
sed  's/ -static-libxlc -static-libstdc++ -Wl,--wrap=memcpy/  -lpthread /g' a1 > a2
sed  's/ldl -lresolv/ldl /g' a2 > a1
cp a1 smv_aix.mak
rm a1
rm a2
echo now run one of these command:
echo make -f smv_aix.mak debug
echo make -f smv_aix.mak release
echo
