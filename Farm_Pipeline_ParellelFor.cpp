#include "CImg.h"
#include<iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <vector>
#include <assert.h>
#include <vector>
#include <iostream>
#include <ff/farm.hpp>
#include <ff/node.hpp>
#include <ff/pipeline.hpp>
#include <ff/parallel_for.hpp>

using namespace cimg_library;
using namespace ff;

struct Task_t {
    Task_t(CImg<float> imgin, float threshold, int* histogram):imgin(imgin),threshold(threshold),histogram(histogram) {}
    CImg<float> imgin;
    float threshold;
    int* histogram;// allcoate memory for no of pixels for each intensity value
};

CImg<float> RGBtoGrayScale(CImg<float> in)
{
  // Creat a grayImage as the size of input image 
  CImg<float> grayImage(in.width(),in.height(),1,1,0);
  ParallelFor pr1(28);
  
  // Translate to weighted gray image
  pr1.parallel_for(0,in.width(),[&grayImage,in](const long i){ 
     for(int j=0;j<in.height();j++)
        grayImage(i,j,0) = (float)(0.299*in(i,j,0) + 0.587*in(i,j,1) + 0.114*in(i,j,2));
        // Because our eyes are more sensible to light from the green frequencies.
  });

  return grayImage;
}

void Histogram(CImg<float> in, int* histogram)
{ 
  ParallelFor pr2(3);
  // initialize all intensity values to 0
  for(int i = 0; i < 256; i++)
    histogram[i] = 0;
  // histogram
  pr2.parallel_for(0,in.width(),[&histogram,in](const long i){
    for(int j=0;j<in.height();j++)
      histogram[(int)in(i,j,0)]++;
  });
}

void Thresholding(CImg<float> &in, float p, int* histogram)
{
  int nPixelsBrighter[256];
  ParallelFor pr3(3);
  // initialize all intensity values to 0
  for(int i = 0; i < 256; i++)
    nPixelsBrighter[i] = 0;

  // get the number of Pixels Brighter
  for(int v = 0; v < 256; v++)
    for(int u = v+1; u < 256; u++)
      nPixelsBrighter[v] += histogram[u];

  // Thresholding
  pr3.parallel_for(0,in.width(),[&in,nPixelsBrighter,p](const long i){
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
   });
}

struct Emitter: ff_node_t<Task_t> {
    CImg<float> imgin;
    size_t nImages;
    float threshold;
    Emitter(CImg<float> imgin, size_t nImages, float threshold)
        :imgin(imgin),nImages(nImages),threshold(threshold) {}

    Task_t *svc(Task_t *in) {
        //auto start0 = std::chrono::high_resolution_clock::now();
        if (in == nullptr){
           while (true) {
              if(nImages)
              {
                 int* histogram = new int[256];// allcoate memory for no of pixels for each intensity value
                 Task_t *task = new Task_t(imgin, threshold, histogram); 
                 ff_send_out(task);    
                 nImages--;
              }else{break;}
           }
           //auto end0 = std::chrono::high_resolution_clock::now();
           //std::cout << "ff_send_out: " << std::chrono::duration_cast<std::chrono::milliseconds>(end0-start0).count()<< " (ms)" <<std::endl;
           return GO_ON;
        }else{
           in->imgin.save("outputImage.jpg");
           //auto end0 = std::chrono::high_resolution_clock::now();
           //std::cout << "save image: " << std::chrono::duration_cast<std::chrono::milliseconds>(end0-start0).count()<< " (ms)" <<std::endl;
           return EOS;
        }
  }
};

struct secondStage: ff_node_t<Task_t> {

    Task_t * svc(Task_t *in) {
        //auto start1 = std::chrono::high_resolution_clock::now();
        CImg<float> imgout(in->imgin.width(),in->imgin.height(),1,1,0);
        // RGBtoGrayScale
        imgout = RGBtoGrayScale(in->imgin);
        in->imgin = imgout;
        //auto end1 = std::chrono::high_resolution_clock::now();
        //std::cout << "RGBtoGrayScale : " << std::chrono::duration_cast<std::chrono::milliseconds>(end1-start1).count()<< " (ms)" <<std::endl;
        return in;
    }
};

struct thirdStage: ff_node_t<Task_t> {
    Task_t * svc(Task_t *in) {
        //auto start2 = std::chrono::high_resolution_clock::now();
        Histogram(in->imgin,in->histogram);
        //auto end2 = std::chrono::high_resolution_clock::now();
        //std::cout << "Histogram : " << std::chrono::duration_cast<std::chrono::milliseconds>(end2-start2).count()<< " (ms)" <<std::endl;
        return in;
    }
};

struct fourthStage: ff_node_t<Task_t> {
    Task_t * svc(Task_t *in) {
        //auto start3 = std::chrono::high_resolution_clock::now();
        Thresholding(in->imgin,in->threshold,in->histogram);
        //auto end3 = std::chrono::high_resolution_clock::now();
        //std::cout << "Thresholding : " << std::chrono::duration_cast<std::chrono::milliseconds>(end3-start3).count()<< " (ms)" <<std::endl;

        return in;
    }
};

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
  int save_flag=0;
  ffTime(START_TIME);

  // read the image
  CImg<float> imgin("inputImage.jpg");

  std::vector<std::unique_ptr<ff_node> > Works;
  for(size_t i=0;i<nworkers;++i)
     Works.push_back(make_unique<ff_Pipe<Task_t>>(make_unique<secondStage>(),make_unique<thirdStage>(),make_unique<fourthStage>()));
  ff_Farm<Task_t>farm(std::move(Works));
  Emitter E(imgin, nImages , threshold);
  farm.add_emitter(E);      // replacing the default emitter
  farm.remove_collector();  // removing the default collector
  farm.set_scheduling_ondemand(); // set on-demand scheduling policy 
  farm.wrap_around();
    if (farm.run_and_wait_end()<0) {
	error("runtime error, exiting!\n");
	return -1;
    }

  ffTime(STOP_TIME);
  std::cout << "nworkers: " << nworkers << " ; " <<"Time: " << ffTime(GET_TIME) << " (ms)" <<std::endl;

  return 0;
}
