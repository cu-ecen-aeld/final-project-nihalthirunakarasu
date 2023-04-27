#ECEN-5713 Advanced Embedded Software Development
#Author name: Khyati Satta
#Date: 04/25/2023
#File Description: Client script for receiving the video frames over Gstreamer pipeline (UDP)

#!/bin/bash

# Port number
PORT_NUMBER=5000

echo "Receiving stream from the server\n"

# Gstreamer Pipeline
gst-launch-1.0 -v udpsrc port=$PORT_NUMBER ! \
    application/x-rtp,encoding-name=JPEG, payload=26 ! \
    rtpjpegdepay ! \
    jpegdec ! \
    videoconvert ! \
    autovideosink