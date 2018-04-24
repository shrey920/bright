while read -n 1 -s char; do
    sudo echo $char > /dev/bright
done