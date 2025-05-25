sendfile="numa_both_send_2.json"
receivefile="numa_both_receive_2.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {20..27}
do
	for j in {1..10}
	do
		echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode s --msg_size $i --duration 30 >> $sendfile &"
		numactl --cpunodebind=0 --membind=0 ./program --mode s --msg_size $i --duration 30 >> $sendfile &
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --msg_size $i --duration 30 >> $receivefile"
		numactl --cpunodebind=0 --membind=0 ./program --mode r --msg_size $i --duration 30 >> $receivefile
		echo "Done"
		date +%H:%M:%S
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile

sendfile="nonuma_both_send_2.json"
receivefile="nonuma_both_receive_2.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {20..27}
do
	for j in {1..10}
	do
		echo "Run: ./program --mode s --msg_size $i --duration 30 >> $sendfile &"
		./program --mode s --msg_size $i --duration 30 >> $sendfile &
		echo "Run: ./program --mode r --msg_size $i --duration 30 >> $receivefile"
		./program --mode r --msg_size $i --duration 30 >> $receivefile
		echo "Done"
		date +%H:%M:%S
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile