export SMV=./smv64
$SMV cz pgopatch1.svf smv.cbp -sha1 -sha256 -md5
$SMV i pgopatch1.svf smv.cbp.mak -rf smv.cbp smv.cbp.mak
$SMV lv pgopatch1.svf 
$SMV -m pgopatch1.svf pgopatch1a.svf -recpr  -all -compressratio 45
$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 128
$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 129
$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 171

$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 179

$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 219
$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 201
$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 173
$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 67
$SMV -m pgopatch1a.svf pgopatch1c.svf -recpr  -all -compressratio 3
$SMV -m pgopatch1.svf pgopatch1c.svf -recpr  -all -compressratio 9

$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 219
$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 201
$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 173


$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 67
$SMV -m pgopatch1b.svf pgopatch1a.svf -recpr  -all -compressratio 552
$SMV -m pgopatch1a.svf pgopatch1b.svf -recpr  -all -compressratio 178

$SMV xz pgopatch1b.svf pgopatch2.zip -v 1
$SMV lv pgopatch1b.svf
unzip -v pgopatch2.zip 
