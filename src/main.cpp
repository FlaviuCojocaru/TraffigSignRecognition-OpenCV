#include <iostream>

#include "opencv4/opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "leptonica/allheaders.h"
#define DETECTION_TEXT_COLOR Scalar(251,240,248)
#define DETECTION_BOX_COLOR Scalar(238,32,99)
#define DETECTION_TEXT_BOX_COLOR Scalar(238,32,99)
using namespace std;
using namespace cv;

Vec3b RGB2HSV(Vec3b pixel){
    float r, g, b, M, m, H = 0, S, V, C;

    unsigned char R = pixel[2];   //R
    unsigned char G = pixel[1];   //G
    unsigned char B = pixel[0];   //B

    r = R / 255.0;
    g = G / 255.0;
    b = B / 255.0;

    M = max(r, max(g, b));
    m = min(r, min(g, b));

    C = M - m;

    //calculare componente
    V = M;
    S = (V != 0) ? C / V : 0;

    if (C != 0) {
        if (M == r) {
            H = 60 * (g - b) / C;
        }

        if (M == g) {
            H = 120 + 60 * (b - r) / C;
        }

        if (M == b) {
            H = 240 + 60 * (r - g) / C;
        }
    }
    else {
        H = 0;
    }

    if (H < 0) {
        H += 360;
    }

    //normalizare componente
    H = H * 255 / 360;
    S *= 255;
    V *= 255;

    return Vec3b(H, S, V);
}

//check if HSV is in red range of RGB
bool checkRed(Vec3b pixel){
    unsigned char H = pixel[0];
    unsigned char S = pixel[1];
    unsigned char V = pixel[2];
    return ((H>235 || H<10) && S>40 && V>30) ? true : false;
}

//check if HSV is in black range of RGB
bool checkBlack(Vec3b pixel){
    unsigned char H = pixel[0];
    unsigned char S = pixel[1];
    unsigned char V = pixel[2];
//    return (H<=240 && S<= 165 && V <= 65) ? true : false;
        return (H<=240 && S<= 155 && V <= 70) ? true : false;
}


Mat computeDilation(Mat originalImage) {
    int ii = 0;
    int jj = 0;
    int di[] = { -1,-1, 0, 1,  1, 1, 0, -1 };
    int dj[] = { 0 ,-1,-1, -1, 0, 1, 1, 1 };
    Mat dilatedImage = originalImage.clone();

    for (int i = 0; i < originalImage.rows; i++ ) {
        for (int j = 0; j < originalImage.cols; j++) {
            if (originalImage.at<uchar>(i, j) == 0) {
                for (int k = 0; k < 8; k++) {
                    //desenam pixelii vecini cu negru
                    ii = i + di[k] < 0 ? 0 : (i + di[k] >= originalImage.rows ? originalImage.rows-1 : i + di[k]);
                    jj = j + dj[k] < 0 ? 0 : (j + dj[k] >= originalImage.cols ? originalImage.cols-1 : j + dj[k]);

                    dilatedImage.at<uchar>(ii, jj) = 0; //aaaaaaaaaa
                }
            }else {
                dilatedImage.at<uchar>(i, j) = 255;  //pixel fundal este copiat pur si simplu
            }
        }
    }
    return dilatedImage;
}

// src = source image matrix(hsv space color)
// checkColor = function used for color based detection
Mat obtainRoi(Mat src, bool (*checkColor) (Vec3b)) {
    Mat roiImg = Mat(src.rows, src.cols, CV_8UC1);

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            Vec3b pixel = src.at<Vec3b>(i, j);
            roiImg.at<uchar>(i, j) = checkColor(pixel) ? 255 : 0;
        }
    }
    return roiImg;
}

//convert BGR image to HSV
Mat convertImg2HSV(Mat sign){

    Mat hsvImg = sign.clone();
    for(int i = 0; i< sign.rows; i++){
        for(int j = 0; j<sign.cols; j++){
            hsvImg.at<Vec3b>(i, j) = RGB2HSV(sign.at<Vec3b>(i, j));     //convert image from RGB to HSV
        }
    }
    return hsvImg;
}

void writeSpeedLimitOnImage(Mat src, Point center, int radius, float size ,string speedLimit){
    string text, text2;
    Point p1;

    if(((center.x - radius)-215) >= 0  && (center.y-25) >= 0){
        //place text on the left of the sign if there is enough
        int x = center.x - radius;
        int y = center.y;
        p1 = Point(x, y);
        Point p3(x, y), p4(x,y);

        if(speedLimit.empty()){
            //case 1
            p3.x -= 135;
            p3.y -= 20;
            p4.x -= 10;
            p4.y += 15;
            text = "unknown";
            rectangle(src,p3,p4,DETECTION_TEXT_BOX_COLOR, FILLED, LINE_AA);
            p1.x -= 130;
        }else{
            //case 2

            //drawing the box that wraps the text
            p3.x -= 215;
            p3.y -= 25;
            p4.x -= 10;
            p4.y += 35;
            rectangle(src,p3,p4,DETECTION_TEXT_BOX_COLOR, FILLED, LINE_AA);

            //writing first line
            p1.x -= 175;
            speedLimit.pop_back();
            text2 = speedLimit + " km/h" ;
            text = "speed limit sign";
            putText( src, text2, p1, FONT_HERSHEY_COMPLEX_SMALL, size, DETECTION_TEXT_COLOR, 1, LINE_AA);
            p1.y += 20;
            p1.x -= 40;
        }
    }else{
        //place text on the bottom of the sign if there is enough
        int x = center.x - radius;
        int y = center.y + radius;
        p1 = Point(x, y);
        Point p3(x, y), p4(x,y);

        if(speedLimit.empty()){
            //case 1
            p3.y += 15; //-20
            p4.x += 125;
            p4.y += 50;
            text = "unknown";
            rectangle(src,p3,p4,DETECTION_TEXT_BOX_COLOR, FILLED, LINE_AA);
            p1.x += 5;
            p1.y += 35;
        }else{
            //case 2

            //drawing the box that wraps the text
            p3.y += 15;
            p4.x += 205;
            p4.y += 65;
            rectangle(src,p3,p4,DETECTION_TEXT_BOX_COLOR, FILLED, LINE_AA);

            //writing first line
            p1.x += 40;
            p1.y += 35;
            speedLimit.pop_back();
            text2 = speedLimit + " km/h" ;
            text = "speed limit sign";
            putText( src, text2, p1, FONT_HERSHEY_COMPLEX_SMALL, size, DETECTION_TEXT_COLOR, 1, LINE_AA);
            p1.y += 20;
            p1.x -= 40;
        }
    }

    putText( src, text, p1, FONT_HERSHEY_COMPLEX_SMALL, size, DETECTION_TEXT_COLOR, 1, LINE_AA);  //writing second line if case 2 or first line if case 1
}

//return speed limit written on traffic sign
string detectSpeedLimit(Mat sign){

    Mat signRoi = obtainRoi(sign, checkBlack);

//    preprocessing image
    bitwise_not(signRoi,signRoi);
    signRoi = computeDilation(signRoi);

    string speedLimit;
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

   // initialize tesseract-ocr with English
   if(api->Init(NULL,"eng", tesseract::OEM_LSTM_ONLY)){
       fprintf(stderr,"Could not initialize tesseract.\n");
       exit(1);
   }

    api->SetPageSegMode(tesseract::PSM_RAW_LINE);
    api->SetVariable("tessedit_char_whitelist", "0123456789");
    api->SetImage(signRoi.data,signRoi.cols,signRoi.rows,1,signRoi.step);

    speedLimit = string(api->GetUTF8Text());
    cout<<speedLimit;

    api->End();
    delete api;

    return speedLimit;
}

void detectTrafficSign(String pathImage){
    string speedLimit;
    Mat srcImage = imread(pathImage, IMREAD_UNCHANGED);

    // color based filter
    Mat srcRoi = convertImg2HSV(srcImage);
    srcRoi = obtainRoi(srcRoi, checkRed);

    // preprocessing the binary image
    bitwise_not(srcRoi, srcRoi);
    srcRoi = computeDilation(srcRoi);
    medianBlur(srcRoi, srcRoi, 9);
    bitwise_not(srcRoi, srcRoi);

   //shaped based filtering
    vector<Vec3f> circles;
    HoughCircles(srcRoi, circles, HOUGH_GRADIENT, 1, srcRoi.rows/8, 200, 25, 0, srcRoi.cols/2);


    //drawing circles
    for( size_t i = 0; i < circles.size(); i++ )
       {
           Vec3i c = circles[i];
           Point center = Point(c[0], c[1]);
           int radius = c[2];

           //draw rectangle
           int x1 = center.x - radius;
           int y1 = center.y - radius;
           Point p1 = Point(x1, y1);
           int x2 = center.x + radius;
           int y2 = center.y + radius;
           Point p2 = Point(x2, y2);
           rectangle(srcImage,p1,p2,DETECTION_BOX_COLOR, 3, LINE_AA);

           //detect speed limit
           Rect r = Rect(p1,p2);
           Mat sign = srcImage(r);
           sign = convertImg2HSV(sign);

//           //improving the contrast in an image using histogram equalization
           vector<Mat> channels;
           split(sign, channels);
           equalizeHist(channels[2], channels[2]);
           merge(channels, sign);
           speedLimit = detectSpeedLimit(sign);
           writeSpeedLimitOnImage(srcImage, center, radius, 1 ,speedLimit);
       }

    imshow("Sign detection", srcImage);
//    imshow("Binary image", srcRoi);
    waitKey(0);
}


int main()
{
//    detectTrafficSign("test images/photo2.jpg");
    detectTrafficSign("test images/photo101.jpg");
//    detectTrafficSign("test images/photo2.jpg");
//    detectTrafficSign("test images/photo3.jpg");
//    detectTrafficSign("test images/photo4.jpg");
//    detectTrafficSign("test images/photo5.jpg");
//    detectTrafficSign("test images/photo6.jpg");
//    detectTrafficSign("test images/photo7.jpg");
//    detectTrafficSign("test images/photo8.jpg");
//    detectTrafficSign("test images/photo9.jpg");
//    detectTrafficSign("test images/photo10.jpg");
//    detectTrafficSign("test images/photo11.jpg");
//    detectTrafficSign("test images/photo13.jpg");
//    detectTrafficSign("test images/photo12.jpg");
//    detectTrafficSign("test images/photo102.jpg");

    return 0;
}




