ssh laptop "killall -9 serialdump-linux &> /dev/null; cp glossy.log glossy.log.bak; screen -S contiki -d -m ./serial.sh"
