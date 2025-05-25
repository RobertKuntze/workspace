sendfile="numa_thread_send_25.json"
receivefile="numa_thread_receive_25.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode s --num_thread $i --duration 30 --msg_size 25 >> $sendfile &"
		numactl --cpunodebind=0 --membind=0 ./program --mode s --num_thread $i --duration 30 --msg_size 25 >> $sendfile &
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode r --num_thread $i --duration 30 --msg_size 25 >> $receivefile"
		numactl --cpunodebind=0 --membind=0 ./program --mode r --num_thread $i --duration 30 --msg_size 25 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile

sendfile="nonuma_thread_send_25.json"
receivefile="nonuma_thread_receive_25.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: ./program --mode s --num_thread $i --duration 30 --msg_size 25 >> $sendfile &"
		./program --mode s --num_thread $i --duration 30 --msg_size 25 >> $sendfile &
		# echo "Run: ./program --mode r --num_thread $i --duration 30 --msg_size 25 >> $receivefile"
		./program --mode r --num_thread $i --duration 30 --msg_size 25 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile