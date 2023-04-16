// This program is a test program to extract input from the camera and display it on the windowx
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    // Open the default camera
    VideoCapture cap(0);

    // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream" << endl;
        return -1;
    }

    // Create a window for the video display
    namedWindow("Camera", WINDOW_NORMAL);

    // Loop to read frames from the camera and display them in the window
    while(true){

        // Read a new frame from the camera
        Mat frame;
        cap.read(frame);

        // Check if frame was successfully read
        if(frame.empty()){
            cout << "End of video stream" << endl;
            break;
        }

        // Display the frame in the window
        imshow("Camera", frame);

        // Wait for 30 milliseconds and check if user wants to quit
        if(waitKey(30) == 27){
            cout << "User quit" << endl;
            break;
        }
    }

    // Release the camera and close the window
    cap.release();
    destroyAllWindows();

    return 0;
}
