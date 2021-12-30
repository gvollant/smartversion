

exec_cbp2mak()
{
        cp $1.cbp $1_tmp.cbp
        sed -i  's/huf_decompress_amd64.S/huf_decompress_amd64.c/g' $1_tmp.cbp
        cbp2make --wrap-objects --wrap-options -in $1_tmp.cbp
        sed -i  's/huf_decompress_amd64.c/huf_decompress_amd64.S/g' $1_tmp.cbp.mak
        cp $1_tmp.cbp.mak $1.cbp.mak
        rm $1_tmp.cbp $1_tmp.cbp.mak
}

exec_cbp2mak smv
