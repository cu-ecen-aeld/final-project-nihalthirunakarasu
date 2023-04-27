##################################################################################
# Filename : mqtt-subscriber.sh
# Description : Subscriber script for mosquitto MQTT broker
# Author : Maanas Makam Dileep Kumar
# Date : 04/26/2023
##################################################################################
#! /bin/sh

# Check number of arguments
if [ "$#" -ne 1 ]
then
    echo "Usage mqtt-subscriber.sh <Broker_IPAddress>"
    exit 0
fi

# logic to parse mqtt commands and control LED
while true;
do
    command=$(mosquitto_sub -h $1 -t aesd -C 1)
    if [ "$command" == "motion" ];
    then
        echo "Motion Detected! LED ON!!"
        # Turn LED ON
        echo 1 > /dev/etx_device
        # We want the LED to be ON atlease for 10 seconds when motion is detected
        sleep 10
        # Turn LED OFF
        echo 0 > /dev/etx_device
    fi
done

