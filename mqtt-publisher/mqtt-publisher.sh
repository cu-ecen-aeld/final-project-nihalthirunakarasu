##################################################################################
# Filename : mqtt-publisher.sh
# Description : SubscrPublisher script for mosquitto MQTT broker
# Author : Maanas Makam Dileep Kumar
# Date : 04/28/2023
##################################################################################
#! /bin/sh

# Check number of arguments
if [ "$#" -ne 1 ]
then
    echo "Usage mqtt-publisher.sh <Broker_IPAddress>"
    exit 0
fi

# Send motion detected message to the subscriber
mosquitto_pub -h $1 -t aesd -m motion