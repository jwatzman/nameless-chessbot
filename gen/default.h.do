gens=""
for c in $2/*.c
do
	gens="$gens ${c%.c}.gen"
done

redo-ifchange $gens

echo "// @generated"
for g in $gens
do
	./$g
	echo
done
