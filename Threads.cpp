#include "CImg.h"
#include<iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <vector>
#include <assert.h>
#include <mutex>
using namespace cimg_library;
int save_flag=0;
int count=0;
std::mutex mtx;
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
        if(percentage <= p)
          in(i,j,0) = 255;// 255:white
        else
          in(i,j,0) = 0;// 0:black
      }
}
 
void HistogramThresolding(CImg<float> imgin, float threshold)
{
    //std::cout << "HistogramThresolding : " << std::this_thread::get_id() <<std::endl;
    CImg<float> imgout(imgin.width(),imgin.height(),1,1,0);
    // RGBtoGrayScale
    imgout = RGBtoGrayScale(imgin);

    // Histogram
    int histogram[256];// allcoate memory for no of pixels for each intensity value
    Histogram(imgout, histogram);
  
    // Thresholding
    Thresholding(imgout, threshold, histogram);

    // Save image
    if(save_flag == 0)
    {                 
       imgout.save("outputImage.jpg");
       if (mtx.try_lock()) {
          save_flag = 1;
          mtx.unlock();
       }
    }
}

int main(int argc, char * argv[]) 
{
  //std::cout << "input :" << argv[0] << " " << argv[1] << " " << argv[2] << " " << argv[3] << std::endl;
  if(argc < 4) 
  {
      std::cerr << "use: "<< argv[0] << " nimages nworkers threshold"<<std::endl;
      return -1; 
  }

  size_t nImages = std::stoll(argv[1]);
  size_t nworkers = std::stoll(argv[2]);
  float threshold = atof(argv[3]);
  assert(nImages >= nworkers);
  
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::thread> workers;


  // read the image
  CImg<float> imgin("inputImage.jpg");

  while(true)
  {
     if(nImages)
     {
        workers.push_back(std::thread(HistogramThresolding,imgin,threshold));
        nImages--;
        if(workers.size() == nworkers)
        {
           for(std::thread& t: workers) {
              try
              {
                 t.join();
              }catch (const std::exception& e) { std::cout<< "error : "<<e.what()<<std::endl; }
           }
           workers.clear();
        }
     }else{ break;}
  }

  if(workers.size())
  {
    for(std::thread& t: workers) {
       try
       {
          t.join();
       }catch (const std::exception& e) { std::cout<<e.what()<<std::endl; }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "nworkers: " << nworkers << " ; " <<"Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<< " (ms)" <<std::endl;

  return 0;
}
