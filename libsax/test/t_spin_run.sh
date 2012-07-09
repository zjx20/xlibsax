#!/bin/bash

if [ $# -eq 0 ]; then
	echo "usage: $0 reader_num writer_num count"
	exit 1
fi

list=("SPIN_PURE" "SPIN_RWLOCK_PREFER_READER" "SPIN_RWLOCK_PREFER_WRITER"	\
	"PTHREAD_MUTEX" "PTHREAD_SPIN" "PTHREAD_RWLOCK_PREFER_READER" "PTHREAD_RWLOCK_PREFER_WRITER")

for x in ${list[@]};
do
	echo "running ${x} version"
	time ./t_spin_${x}.exe $@
	if [ "$?" != "0" ]; then
		exit 1
	fi
	echo ""
done
