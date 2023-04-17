/********************************************************************************
 *
 * This file contains the code that detects motion in a video input stream.
 * The code is written keeping in mind inputs from the camera or from a file.
 * 
 * CAM_CAPTURE is set to 0 to take iput from a video file
 * 
 * CONTINUOUS_MODE is set to 0 to step through the frames one by one. To 
 * progress ahead press any key on the keyboard and to exit the program press ESC.
 * 
 * DEBUG_SHOW is set to 1 to display all the image processing steps.
 * 
 * DEBUG_PRINT is set to 1 to display all debug logs.
 * 
 * Sources: https://docs.opencv.org/4.x/d7/dfc/group__highgui.html
 *          https://towardsdatascience.com/image-analysis-for-beginners-creating-a-motion-detector-with-opencv-4ca6faba4b42 
 * 
 ********************************************************************************/
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

#define DEBUG_SHOW 0       // Set to 1 to show frames at each step
#define DEBUG_PRINT 1      // Set to 1 to print debug statements
#define CONTINUOUS_MODE 1  // Set to zero to step though frames
#define CAM_CAPTURE 0      // Set 0 to take input from file else from cam

#define H_RES 320
#define V_RES 240
#define FPS 30

int main(int argc, char** argv)
{
    /***************************************************************
     *                Source of Input
     **************************************************************/
    #if CAM_CAPTURE
    // Open the default camera
    VideoCapture cap(0);
    #if DEBUG_PRINT
    cout << 
    "Capturing from camera"
    << endl  ;
    #endif
    
    #else
    VideoCapture cap("../test_videos/test.mp4");
    #if DEBUG_PRINT
    cout << 
    "Capturing from file"
    << endl  ;
    #endif
    #endif
    

    // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream" << endl;
        return -1;
    }
    

    // Read a new frame from the camera
    Mat frame;
    Mat prev_frame;
    Mat pres_frame;
    Mat processed_frame;
    bool isFrame0 = true;

    // Loop to read frames from the camera and display them in the window
    while(true){
        
        /***************************************************************
         *                Step1: Capturing the frame
         **************************************************************/
        cap.read(frame);
        
        // Check if frame was successfully read
        if(frame.empty()){
            cout << "End of video stream" << endl;
            break;
        }
    
        imshow("Input", frame);

        #if DEBUG_SHOW
        // Display the frame in the window
        imshow("Step1: Captured Image", frame);
        #endif
        #if DEBUG_PRINT
        cout << 
        "Size of image: " << frame.size() << endl << "Type of image: " << 
        (frame.type() == CV_8UC3 ? "BGR Color" : 
        (frame.type() == CV_8UC1 ? "Gray Scale" : "Format other than color or gray")) 
        << endl  ;
        #endif
        
        /***************************************************************
         *          Step2: Converting the frame to greyscale
         **************************************************************/
         cvtColor(frame, processed_frame, COLOR_BGR2GRAY);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step2: Converting the frame to greyscale", processed_frame);
         #endif
         
        /***************************************************************
         *        Step3: Blurring the image to remove any noise
         **************************************************************/
         GaussianBlur(processed_frame, processed_frame, Size(5,5),  0);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step3: Blurring the image to remove any noise", processed_frame);
         #endif
       
         
        /***************************************************************
         *        Step4: Detecting differences between frames
         **************************************************************/
         // Code to store the previous frame and the current frame to detect motion
         // Clone has to be done in order to avoid shallow copy of matrix
         if(isFrame0)
         {
             pres_frame = processed_frame.clone();
             isFrame0 = false;
         }
            
         prev_frame = pres_frame.clone();
         pres_frame = processed_frame.clone();
         
        
         absdiff(prev_frame, pres_frame, processed_frame);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step4: Detecting differences between frames", processed_frame);
         #endif
         
        /***************************************************************
         *       Step5a: Dilating the pixels with differences
         **************************************************************/
         Mat kernel = Mat::ones(5, 5, CV_8UC1);
         dilate(processed_frame, processed_frame, kernel);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step5a: Dilating the pixels with differences", processed_frame);
         #endif
         
        /***************************************************************
         *       Step5b: Eroding the pixels with differences
         **************************************************************/
         erode(processed_frame, processed_frame, kernel);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step5b: Eroding the pixels with differences", processed_frame);
         #endif
         
        /***************************************************************
         *       Step6: Thresholding the pixels to higher value
         **************************************************************/
         int thresholdVal = 20;
         threshold(processed_frame, processed_frame, thresholdVal, 255, THRESH_BINARY);
         
         #if DEBUG_SHOW
         // Display the frame in the window
         imshow("Step6: Thresholding the pixels to higher value", processed_frame);
         #endif
       
        /***************************************************************
         *                 Step7: Finding contours
         **************************************************************/
         vector<vector<Point>> contours;
         vector<Vec4i> hierarchy;
         
         findContours(processed_frame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
         
        /***************************************************************
         *                 Step7: Drawing contours
         **************************************************************/
         int idx = 0;
         Scalar color(0, 0, 255);
         
         // Drawing simple contours
         //drawContours(frame, contours, -1, color, 2, LINE_AA);
         
         #if DEBUG_PRINT
         cout << 
         "Number of contours: " << contours.size()
         << endl  ;
         #endif
         
         // Drawing rectangles around the contours
         for (int i = 0; i < contours.size(); i++)
         {
             if(contourArea(contours[i]) < 100)
             {
             }
             else
            {
                #if DEBUG_PRINT
                cout << 
                "Area of contour " << i << " : " << contourArea(contours[i])
                << endl  ;
                #endif
                Rect bounding_rect = boundingRect(contours[i]);
                rectangle(frame, bounding_rect, color, 2);
            }   
         }
         
        /***************************************************************
         *                       Display Output
         **************************************************************/
         // Display the frame in the window
         imshow("output: Motion Detection", frame);         
         
         #if CONTINUOUS_MODE
         // Wait for 30 milliseconds and check if user wants to quit
         if(waitKey(60) == 27){
            cout << "User quit" << endl;
            break;
         }
         #else
         int temp = 0;
         while(!temp)
         {
             temp = waitKey();
         };
         if (temp == 27)
            break;
         #endif
     }

     // Release the camera and close the window
     cap.release();
     destroyAllWindows();

     return 0;
}
