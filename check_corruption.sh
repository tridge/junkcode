#!/bin/bash
# check for data corruption on migrate/recall with TSM/HSM
# tridge@samba.org June 2008

# do 10k files by default
N=10000

if [ $# -ge 1 ]; then
    N="$1"
fi


[ -d rand1M ] && {
    echo "Cleaning up from old run"
    rm -rf rand1M
}

mkdir rand1M || exit 1
cd rand1M || exit 1
echo "Creating $N files"
for i in `seq 1 $N`; do echo $i; dd if=/dev/urandom of=rand$i.dat bs=1M count=1; done

echo "Syncing and sleeping 10 seconds"
sync
sleep 10
sync

echo "Checking sizes"
for i in `seq 1 $N`; do 
    [ `stat -c '%b' rand$i.dat` = 2048 ] || {
	echo "ERROR: rand$i.dat is not blocks 2048 in size"
	stat rand$i.dat
	exit 1
    }
done


echo "Checksumming $N files with md5sum"
md5sum rand*.dat | tee ../rand1Msum.dat

echo "Waiting 2 minutes for files to be migratable"
sleep 120

echo "Migrating $N files"
dsmmigrate rand*dat

echo "Checking sizes"
for i in `seq 1 $N`; do 
    [ `stat -c '%b' rand$i.dat` = 0 ] || {
	echo "ERROR: rand$i.dat is not fully migrated"
	stat rand$i.dat
	exit 1
    }
done

echo "Checksumming again"
md5sum rand*.dat | tee ../rand1Msum_after_migration.dat

count=`comm -23 ../rand1Msum.dat ../rand1Msum_after_migration.dat | wc -l`

if [ $count -ne 0 ]; then
    echo "ERROR: These $count files were CORRUPTED"
    comm -23 ../rand1Msum.dat ../rand1Msum_after_migration.dat
    exit 1
fi

echo "No files were corrupted on recall"

exit 0
