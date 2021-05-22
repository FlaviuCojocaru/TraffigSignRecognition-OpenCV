#include <iostream>

#include "opencv4/opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "leptonica/allheaders.h"
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
    return ((H>240 || H<10) && S>40 && V>30) ? true : false;
}

//check if HSV is in yellow range of RGB
bool checkYellow(Vec3b pixel){
    unsigned char H = pixel[0];
    unsigned char S = pixel[1];
    unsigned char V = pixel[2];
    return ((H >= 18 && H<=45) && S>=148 && V>=66) ? true : false;
}

//check if HSV is in blue range of RGB
bool checkBlue(Vec3b pixel){
    unsigned char H = pixel[0];
    unsigned char S = pixel[1];
    unsigned char V = pixel[2];
    return ((H>120 && H<=175) && S>=127.5 && V>=20) ? true : false;
}


//check if HSV is in black range of RGB
bool checkBlack(Vec3b pixel){
    unsigned char H = pixel[0];
    unsigned char S = pixel[1];
    unsigned char V = pixel[2];
    return ((H>=10 && H<=210) && (S>=10 && S <= 205) && (V>=0 && V <= 65)) ? true : false;
}

//dilation
Mat computeDilation(Mat originalImage) {
    Mat dilatedImage;
    int morph_size = 2;
    Mat element = getStructuringElement(
            MORPH_ELLIPSE, Size(2 * morph_size + 1,
                             2 * morph_size + 1),
            Point(morph_size, morph_size));

        dilate(originalImage, dilatedImage, element,Point(-1, -1), -1);

    return dilatedImage;
}

Mat computeErosion(Mat originalImage) {
    Mat erodedImage;
    int morph_size = 2;
    Mat element = getStructuringElement(
            MORPH_ELLIPSE, Size(2 * morph_size + 1,
                             2 * morph_size + 1),
            Point(morph_size, morph_size));

    erode(originalImage, erodedImage, element,Point(-1, -1), -1);

    return erodedImage;
}

Mat computeOpening(Mat originalImage) {
    Mat openingMat = computeErosion(originalImage);
    openingMat = computeDilation(openingMat);
    return openingMat;
}

Mat computeClosing(Mat originalImage) {
    Mat closingMat = computeDilation(originalImage);
    closingMat = computeErosion(closingMat);
    return closingMat;
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

Mat detectSpeedLimit(Mat sign){

    Mat signRoi = obtainRoi(sign, checkBlack);

    //preprocessing image
//    signRoi = computeDilation(signRoi);
//    medianBlur(signRoi, signRoi, 5);
    bitwise_not(signRoi,signRoi);

    char *otxt;
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

   // initialize tesseract-ocr with English
   if(api->Init(NULL,"eng", tesseract::OEM_LSTM_ONLY)){
       fprintf(stderr,"Could not initialize tesseract.\n");
       exit(1);
   }

    api->SetPageSegMode(tesseract::PSM_RAW_LINE);
    api->SetVariable("tessedit_char_whitelist", "0123456789");
    api->SetImage(signRoi.data,signRoi.cols,signRoi.rows,1,signRoi.step);

    otxt = api->GetUTF8Text();
    printf("Speed limit: %s\n",otxt);

    api->End();
    delete api;
    delete [] otxt;


    return signRoi;
}

void detectTrafficSign(String pathImage){
    Mat srcImage = imread(pathImage, IMREAD_UNCHANGED);

    //color based filter
    Mat srcRoi = convertImg2HSV(srcImage);
    srcRoi = obtainRoi(srcRoi, checkRed);

//    preprocessing
//    testROI = computeDilation(testROI);
    medianBlur(srcRoi, srcRoi, 7);

    //shaped based filtering
    vector<Vec3f> circles;
    HoughCircles(srcRoi, circles, HOUGH_GRADIENT, 1.5, srcRoi.rows/8, 200, 50, 0, srcRoi.cols/2);


    //drawing circles
    Mat m1;
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
           rectangle(srcImage,p1,p2,Scalar(0,255,0), 3, LINE_AA);

           //detect speed limit
           Rect r = Rect(p1,p2);
           Mat sign = srcImage(r);
           sign = convertImg2HSV(sign);

           //improving the contrast in an image using histogram equalization
           vector<Mat> channels;
           split(sign, channels);
           equalizeHist(channels[2], channels[2]);
           merge(channels, sign);
           m1 = detectSpeedLimit(sign);

           std::ostringstream oss;
           oss << "signTest" << i;
           std::string imgName = oss.str();

           imshow(imgName, m1);
       }

    imshow("Orignal image", srcImage);
    imshow("Binary image", srcRoi);
    waitKey(0);
}


int main()
{
    detectTrafficSign("test images/photo1.jpg");

    return 0;
}


//in detect sign

//cvtColor(sign, sign, COLOR_BGR2HSV);
//vector<Mat> channels;
//split(sign, channels);
//equalizeHist(channels[2], channels[2]);   //equalizam dupa V
//merge(channels, sign);
//cvtColor(sign, sign, COLOR_HSV2BGR);






