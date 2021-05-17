#include <iostream>

#include "opencv4/opencv2/opencv.hpp"
using namespace std;
using namespace cv;

struct HSV{
    float H;
    float S;
    float V;
};

HSV RGB2HSV(unsigned char R, unsigned char G, unsigned char B){
    float r, g, b, M, m, H = 0, S, V, C;

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

    return {H, S, V};
}

//check if HSV is in red range of RGB
bool checkRed(HSV hsv){
    if((hsv.H>240 || hsv.H<10) && hsv.S>40 && hsv.V>30){
        return true;
    }
    return false;
}

//check if HSV is in yellow range of RGB
bool checkYellow(HSV hsv){
    if((hsv.H >= 18 && hsv.H<=45) && hsv.S>=148 && hsv.V>=66){
        return true;
    }
    return false;
}

//check if HSV is in blue range of RGB
bool checkBlue(HSV hsv){
    if((hsv.H>120 && hsv.H<=175) && hsv.S>=127.5 && hsv.V>=20){
        return true;
    }
    return false;
}



Mat obtainROI(Mat src) {

    HSV hsv;
    Mat roiImg = src.clone();

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            Vec3b pixel = src.at<Vec3b>(i, j);
            unsigned char R = pixel[2];   //R
            unsigned char G = pixel[1];   //G
            unsigned char B = pixel[0];   //B

            hsv = RGB2HSV(R, G, B);     //convert image from RGB to HSV

            if(checkRed(hsv)){
                roiImg.at<Vec3b>(i, j) = Vec3b(255,255,255);
            }else{
                roiImg.at<Vec3b>(i, j) = Vec3b(0,0,0);
            }
        }
    }
    return roiImg;
}


void detectTrafficSign(String pathImage){
    Mat srcImage = imread(pathImage, IMREAD_UNCHANGED);
    Mat mytest = obtainROI(srcImage);

    imshow("Orignal image", srcImage);
    imshow("MY HSV", mytest);
    waitKey(0);
}


int main()
{
    //    detectTrafficSign("test images/PI-L2/objects_24bits.bmp");
    detectTrafficSign("test images/photo20.jpeg");
//    detectTrafficSign("test images/hsvTest.png");
    return 0;
}
