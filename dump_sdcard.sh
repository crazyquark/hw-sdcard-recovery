echo "Press enter to start"
echo "Watch the serial output for progress info"
ADDRESS=192.168.1.177
PORT=23
FILE=sdcard.img

start=`date +%s`
nc $ADDRESS $PORT > $FILE
end=`date +%s`
runtime=$((end-start))

echo "Running time: $runtime"
echo "Done"