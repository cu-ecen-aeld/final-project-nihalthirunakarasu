#! /bin/sh

# This script will reside in the /etc/init.d/S99aesdsocket folder
# It will reference to the code residing at /usr/bin/aesdsocket
# This script will reference to the location where the aesdsocket assignment resides
# This script will use start-stop-daemon whihc will run the socket code as a daemon on start up and
# kills the process during shutdown

# During start up "start"  will be passed as an argument to the script similarly during stop "stop" will be passed
case "$1" in
    start)
        echo "Starting aesdsocket as a daemon process...."
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    
    stop)
         echo "Stoping aesdsocket as a daemon process...."
         start-stop-daemon -K -n aesdsocket
         ;;
    *)

    echo "Usage: $0{start|stop}"

    exit 1
esac

exit 0