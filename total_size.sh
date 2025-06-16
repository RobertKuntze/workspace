#!/bin/sh

# sendfile="data/numa_both_send_total.json"
# receivefile="data/numa_both_receive_total.json"

# echo "[" > $sendfile
# echo "[" > $receivefile

# for i in $(seq 10 33)
# do
# 	for j in $(seq 5 $i)
# 	do
# 		echo "Run: numactl --membind=0 ./program --mode sbt --msg-size $j --total-size $i --threads 7 >> $sendfile &"
# 		numactl --membind=0 ./program --mode sbt --msg-size $j --total-size $i --threads 7 >> $sendfile &
# 		pid=$!
# 		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --msg-size $j --total-size $i --threads 7 >> $receivefile"
# 		numactl --membind=0 ./program --mode rb --msg-size $j --total-size $i --threads 7 >> $receivefile
# 		wait $pid
# 		if [ "$i" -ne 33 ] || [ "$j" -ne "$i" ]; then
#             echo "," >> "$receivefile"
#             echo "," >> "$sendfile"
#         fi
# 		echo "Done"
# 		date +%H:%M:%S
# 	done
# done

# for i in {}
# echo "]" >> $sendfile
# echo "]" >> $receivefile
sendfile="data/packet-size_send.json"
receivefile="data/packet-size_receive.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in $(seq 10 22)
do
	echo "Run numactl --membind=0 ./program --mode sbt --msg-size 22 --packet-size $i >> $sendfile &"
	numactl --membind=0 ./program --mode sbt --msg-size 22 --packet-size $i >> $sendfile &
	pid=$!
	numactl --membind=0 ./program --mode rb --msg-size 22 --packet-size $i >> $receivefile
	wait $pid
	if [ "$i" -ne 22 ]; then
		echo "," >> "$sendfile"
		echo "," >> "$receivefile"
	fi
done

echo "]" >> $sendfile
echo "]" >> $receivefile

# sendfile="nonuma_both_send_8gb.json"
# receivefile="nonuma_both_receive_8gb.json"

# echo "[" > $sendfile
# echo "[" > $receivefile

# for i in {20..27}
# do
# 	for j in {1..10}
# 	do
# 		echo "Run: ./program --mode s --msg-size $j --total-size $i --threads 7 >> $sendfile &"
# 		./program --mode sb --msg-size $j --total-size $i --threads 7 >> $sendfile &
# 		pid=$!
# 		echo "Run: ./program --mode r --msg-size $j --total-size $i --threads 7 >> $receivefile"
# 		./program --mode rb --msg-size $j --total-size $i --threads 7 >> $receivefile
# 		wait $pid
# 		echo "Done"
# 		date +%H:%M:%S
# 	done
# done
# echo "]" >> $sendfile
# echo "]" >> $receivefile

# sendfile="data/numar_both_send_8gb.json"
# receivefile="data/numar_both_receive_8gb.json"

# echo "[" > $sendfile
# echo "[" > $receivefile

# for i in {20..27}
# do
# 	for j in {1..10}
# 	do
# 		echo "Run: numactl --membind=1 ./program --mode s --msg-size $j --total-size $i >> $sendfile &"
# 		numactl --membind=1 ./program --mode sb --msg-size $j --total-size $i >> $sendfile &
# 		pid=$!
# 		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --msg-size $j --total-size $i >> $receivefile"
# 		numactl --membind=1 ./program --mode rb --msg-size $j --total-size $i >> $receivefile
# 		wait $pid
# 		if [ "$i" -ne 27 ] || [ "$j" -ne 10 ]; then
#             echo "," >> "$receivefile"
#             echo "," >> "$sendfile"
#         fi
# 		echo "Done"
# 		date +%H:%M:%S
# 	done
# done
# echo "]" >> $sendfile
# echo "]" >> $receivefile