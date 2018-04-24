rmmod bright
rm /dev/bright
make clean
make
insmod bright.ko
mknod /dev/bright c 242 0
chmod 777 /dev/bright