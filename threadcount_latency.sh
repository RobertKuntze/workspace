sendfile="data/numa_thread_latency.json"
receivefile="data/numa_thread_latency.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode sl --num_thread $i --duration 30 --msg_size 10 >> $sendfile &"
		numactl --cpunodebind=0 --membind=0 ./program --mode sl --num_thread $i --duration 30 --msg_size 10 >> $sendfile &
		# echo "Run: numactl --cpunodebind=0 --membind=0 ./program --mode rl --num_thread $i --duration 30 --msg_size 10 >> $receivefile"
		numactl --cpunodebind=0 --membind=0 ./program --mode rl --num_thread $i --duration 30 --msg_size 10 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile

sendfile="data/nonuma_thread_latency.json"
receivefile="data/nonuma_thread_latency.json"

echo "[" > $sendfile
echo "[" > $receivefile

for i in {1..16}
do
	for j in {1..10}
	do
		echo "Run: ./program --mode sl --num_thread $i --duration 30 --msg_size 10 --iterations 10000 >> $sendfile &"
		./program --mode sl --num_thread $i --duration 30 --msg_size 10 --iterations 10000 >> $sendfile &
		# echo "Run: ./program --mode rl --num_thread $i --duration 30 --msg_size 10 --iterations 10000 >> $receivefile"
		./program --mode rl --num_thread $i --duration 30 --msg_size 10 --iterations 10000 >> $receivefile
		echo "Done"
		date +%H:%M:%S
		sleep 1
	done
done
echo "]" >> $sendfile
echo "]" >> $receivefile