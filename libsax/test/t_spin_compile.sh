#!/bin/bash

list=("SPIN_PURE" "SPIN_RWLOCK_PREFER_READER" "SPIN_RWLOCK_PREFER_WRITER"	\
	"PTHREAD_MUTEX" "PTHREAD_SPIN" "PTHREAD_RWLOCK_PREFER_READER" "PTHREAD_RWLOCK_PREFER_WRITER")

for x in ${list[@]};
do
	g++ -Wall -g -o t_spin_${x}.exe t_spin.cpp -I.. -I/usr/local/include -L.. -lsax -lpthread -D${x}
	if [ "$?" != "0" ]; then
		exit 1
	fi
done
