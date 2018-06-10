#include "CImg.h"
#include<iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <vector>
#include <assert.h>
#include <vector>
#include <iostream>
#include <ff/pipeline.hpp>
#include <ff/node.hpp>

using namespace cimg_library;
using namespace ff;

struct Task_t {
    Task_t(CImg<float> imgin, int save_flag, float threshold):imgin(imgin),save_flag(save_flag),threshold(threshold) {}
    CImg<float> imgin;
    int save_flag;
    float threshold;
};

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
 
void HistogramThresolding(CImg<float> imgout, int save_flag, float threshold)
{
    //std::cout << save_flag <<" HistogramThresolding : " << std::this_thread::get_id() <<std::endl;
    //CImg<float> imgout(imgin.width(),imgin.height(),1,1,0);
    // RGBtoGrayScale
    //imgout = RGBtoGrayScale(imgin);

    // Histogram
    int histogram[256];// allcoate memory for no of pixels for each intensity value
    Histogram(imgout, histogram);
  
    // Thresholding
    Thresholding(imgout, threshold, histogram);

    // Save image
    if(save_flag == 0)
    {                 
       //std::cout << "Save image: " << std::this_thread::get_id() << std::endl;
       imgout.save("outputImage.jpg");
    }
}

struct firstStage: ff_node_t<Task_t> {
    CImg<float> imgin;
    size_t nImages;
    int save_flag;
    float threshold;
    firstStage(CImg<float> imgin, size_t nImages, int save_flag, float threshold)
        :imgin(imgin),nImages(nImages),save_flag(save_flag),threshold(threshold) {}

    Task_t *svc(Task_t *in) {
        while (true) {
            if(nImages)
            {
               if(save_flag)
               {
                  Task_t *task = new Task_t(imgin, save_flag, threshold); 
                  ff_send_out(task);      
               }
               else
               {
                  Task_t *task = new Task_t(imgin, save_flag, threshold);
                  ff_send_out(task);
                  save_flag = 1; 
               }
               nImages--;
            }else{ break;}
         }
         return EOS;
    }
};

struct secondStage: ff_node_t<Task_t> {
    Task_t * svc(Task_t *in) {
        CImg<float> imgout(in->imgin.width(),in->imgin.height(),1,1,0);
        // RGBtoGrayScale
        imgout = RGBtoGrayScale(in->imgin);
        in->imgin = imgout;
        return in;
    }
};

struct thirdStage: ff_node_t<Task_t> {
    Task_t * svc(Task_t *in) {
        HistogramThresolding(in->imgin,in->save_flag,in->threshold);
        return GO_ON;
    }
};

int main(int argc, char * argv[]) 
{
  if(argc < 3) 
  {
      std::cerr << "use: "<< argv[0] << " nimages threshold"<<std::endl;
      return -1; 
  }

  size_t nImages = std::stoll(argv[1]);
  float threshold = atof(argv[2]);

  int save_flag=0;
  ffTime(START_TIME);

  // read the image
  CImg<float> imgin("inputImage.jpg");

  firstStage  Emitter(imgin, nImages ,save_flag, threshold);
  secondStage RGBtoGrayScale;
  thirdStage  HistogramThresolding;

  // builds the pipeline
  ff_Pipe<> pipe(Emitter,RGBtoGrayScale,HistogramThresolding);

  // starts the pipeline
  if (pipe.run_and_wait_end()<0){
	error("runtime error, exiting!\n");
	return -1;
  }

  ffTime(STOP_TIME);
  std::cout <<"Time: " << ffTime(GET_TIME) << " (ms)" <<std::endl;

  return 0;
}
