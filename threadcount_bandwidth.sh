sendfile="data/numa_thread_send_22.json"
receivefile="data/numa_thread_receive_22.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode sb --num_thread $i --duration 30 --msg_size 22 >> $sendfile &"
		numactl --cpunodebind=0 --membind=0 ./program --mode sb --num_thread $i --duration 30 --msg_size 22 >> $sendfile &
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode rb --num_thread $i --duration 30 --msg_size 22 >> $receivefile"
		numactl --cpunodebind=0 --membind=0 ./program --mode rb --num_thread $i --duration 30 --msg_size 22 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile

sendfile="data/nonuma_thread_send_22.json"
receivefile="data/nonuma_thread_receive_22.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: ./program --mode sb --num_thread $i --duration 30 --msg_size 22 >> $sendfile &"
		./program --mode sb --num_thread $i --duration 30 --msg_size 22 >> $sendfile &
		# echo "Run: ./program --mode rb --num_thread $i --duration 30 --msg_size 22 >> $receivefile"
		./program --mode rb --num_thread $i --duration 30 --msg_size 22 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile