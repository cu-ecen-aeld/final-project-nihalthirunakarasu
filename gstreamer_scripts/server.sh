#ECEN-5713 Advanced Embedded Software Development
#Author name: Khyati Satta
#Date: 04/25/2023
#File Description: Server script for sending the video frames over Gstreamer pipeline (UDP)

#!/bin/bash

# Port number
PORT_NUMBER=5000
# Client IP address
CLIENT_IP=$1
# Camera device
CAM_DEV=/dev/video0

#Error check 1: Check if number of arguments is 1
if [ $# -ne 1 ]
then
    echo "Please enter one argument only- IP address of the RECEIVING Raspberry Pi"
    exit 1
fi

echo "Sending stream to the client with IP address: $CLIENT_IP"

gst-launch-1.0 -v v4l2src device=$CAM_DEV ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    videoconvert ! \
    jpegenc ! \
    rtpjpegpay ! \
    udpsink host=$CLIENT_IP port=$PORT_NUMBER