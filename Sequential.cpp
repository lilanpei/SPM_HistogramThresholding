#include "CImg.h"
#include<iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <vector>
#include <assert.h>

using namespace cimg_library;

CImg<float> RGBtoGrayScale(CImg<float> in)
{
  // Creat a grayImage as the size of input image 
  CImg<float> grayImage(in.width(),in.height(),1,1,0);
  
  // Translate to weighted gray image
  for(int i=0;i<in.width();i++) 
     for(int j=0;j<in.height();j++)
        grayImage(i,j,0) = (float)(0.299*in(i,j,0) + 0.587*in(i,j,1) + 0.114*in(i,j,2));
        // Because our eyes are more sensible to light from the green frequencies.
  return grayImage;
}

void Histogram(CImg<float> in, int* histogram)
{ 
  // initialize all intensity values to 0
  for(int i = 0; i < 256; i++)
    histogram[i] = 0;
  
  // histogram
  for(int i=0;i<in.width();i++) 
    for(int j=0;j<in.height();j++)
      histogram[(int)in(i,j,0)]++;
}

void Thresholding(CImg<float> &in, float p, int* histogram)
{
  int nPixelsBrighter[256];

  // initialize all intensity values to 0
  for(int i = 0; i < 256; i++)
    nPixelsBrighter[i] = 0;

  // get the number of Pixels Brighter
  for(int v = 0; v < 256; v++)
    for(int u = v+1; u < 256; u++)
      nPixelsBrighter[v] += histogram[u];

  // Thresholding
  for(int i=0;i<in.width();i++) 
    for(int j=0;j<in.height();j++)
      {
        int tNumPixels = in.width()*in.height();
        //int curNum = histogram[(int)in(i,j,0)];
        float percentage = (float)nPixelsBrighter[(int)in(i,j,0)]/tNumPixels;
        //std::cout<<"nPixelsBrighter= "<< nPixelsBrighter[(int)in(i,j,0)]<<" curNum="<< curNum<<" percentage="<<percentage<<std::endl;
        if(percentage <= p)
          in(i,j,0) = 255;// 255:white
        else
          in(i,j,0) = 0;// 0:black
      }
}

void HistogramThresolding(CImg<float> imgin, CImg<float>* imgout, float threshold)
{
    // RGBtoGrayScale
    auto start1 = std::chrono::high_resolution_clock::now();
    *imgout = RGBtoGrayScale(imgin);
    auto end1 = std::chrono::high_resolution_clock::now();
    std::cout << "RGBtoGrayScale : " << std::chrono::duration_cast<std::chrono::milliseconds>(end1-start1).count()<< " (ms)" <<std::endl;

    // Histogram
    auto start2 = std::chrono::high_resolution_clock::now();
    int histogram[256];// allcoate memory for no of pixels for each intensity value
    Histogram(*imgout, histogram);
    auto end2 = std::chrono::high_resolution_clock::now();
    std::cout << "Histogram : " << std::chrono::duration_cast<std::chrono::milliseconds>(end2-start2).count()<< " (ms)" <<std::endl;

    // Thresholding
    //std::cout<< "threshold = " << threshold << std::endl;
    auto start3 = std::chrono::high_resolution_clock::now();
    Thresholding(*imgout, threshold, histogram);
    auto end3 = std::chrono::high_resolution_clock::now();
    std::cout << "Thresholding : " << std::chrono::duration_cast<std::chrono::milliseconds>(end3-start3).count()<< " (ms)" <<std::endl;
}

int main(int argc, char * argv[]) 
{
  if(argc < 3) 
  {
      std::cerr << "use: "<< argv[0] << " nimages threshold"<<std::endl;
      return -1; 
  }

  size_t nImages = std::stoll(argv[1]);
  float threshold = atof(argv[2]);
  assert(nImages >= 1);
  
  auto start = std::chrono::high_resolution_clock::now();
  int save_flag=0;

  // read the image
  auto start4 = std::chrono::high_resolution_clock::now();
  CImg<float> imgin("inputImage.jpg");
  auto end4 = std::chrono::high_resolution_clock::now();
  std::cout << "OpenImage : " << std::chrono::duration_cast<std::chrono::milliseconds>(end4-start4).count()<< " (ms)" <<std::endl;
  CImg<float> imgout(imgin.width(),imgin.height(),1,1,0);

  while(true)
  {
     if(nImages)
     {
        //std::cout << "nImages = " << nImages <<std::endl;
        HistogramThresolding(imgin,&imgout,threshold);
        nImages--;
        //std::cout << "save image " <<std::endl;
        if(save_flag == 0)
        {           
           auto start5 = std::chrono::high_resolution_clock::now();      
           imgout.save("outputImage.jpg");
           auto end5 = std::chrono::high_resolution_clock::now();
           std::cout << "SaveImage : " << std::chrono::duration_cast<std::chrono::milliseconds>(end5-start5).count()<< " (ms)" <<std::endl;
           save_flag = 1;
        }
     }else{ break;}
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "TotalTime : " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<< " (ms)" <<std::endl;

  return 0;
}
