#!/bin/bash

sendfile="data/numa_both_send_queue.json"
receivefile="data/numa_both_receive_queue.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {20..27}
do
	for j in {1..10}
	do
		echo "Run: numactl --membind=0 ./program --mode sb --msg-size $i --duration 10 --num_thread 7 >> $sendfile &"
		numactl --membind=0 ./program --mode sb --msg-size $i --duration 10 --threads 7 >> $sendfile &
		pid=$!
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --msg-size $i --duration 10 --num_thread 7 >> $receivefile"
		numactl --membind=0 ./program --mode rb --msg-size $i --duration 10 --threads 7 >> $receivefile
		wait $pid
		if [ "$i" -ne 27 ] || [ "$j" -ne 10 ]; then
            echo "," >> "$receivefile"
            echo "," >> "$sendfile"
        fi
		echo "Done"
		date +%H:%M:%S
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile

# sendfile="nonuma_both_send_queue.json"
# receivefile="nonuma_both_receive_queue.json"

# echo "[" > $sendfile
# echo "[" > $receivefile

# for i in {20..27}
# do
# 	for j in {1..10}
# 	do
# 		echo "Run: ./program --mode s --msg-size $i --duration 10 --num_thread 7 >> $sendfile &"
# 		./program --mode sb --msg-size $i --duration 10 --num_thread 7 >> $sendfile &
# 		pid=$!
# 		echo "Run: ./program --mode r --msg-size $i --duration 10 --num_thread 7 >> $receivefile"
# 		./program --mode rb --msg-size $i --duration 10 --num_thread 7 >> $receivefile
# 		wait $pid
# 		echo "Done"
# 		date +%H:%M:%S
# 	done
# done
# echo "]" >> $sendfile
# echo "]" >> $receivefile

sendfile="data/numar_both_send_queue.json"
receivefile="data/numar_both_receive_queue.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {20..27}
do
	for j in {1..10}
	do
		echo "Run: numactl --membind=1 ./program --mode sb --msg-size $i --duration 10 >> $sendfile &"
		numactl --membind=1 ./program --mode sb --msg-size $i --duration 10 --threads 7 >> $sendfile &
		pid=$!
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --msg-size $i --duration 10 >> $receivefile"
		numactl --membind=1 ./program --mode rb --msg-size $i --duration 10 --threads 7 >> $receivefile
		wait $pid
		if [ "$i" -ne 27 ] || [ "$j" -ne 10 ]; then
            echo "," >> "$receivefile"
            echo "," >> "$sendfile"
        fi
		echo "Done"
		date +%H:%M:%S
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile