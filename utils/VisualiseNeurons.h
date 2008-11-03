// VisualiseNeurons.h written by Johannes Hjorth, hjorth@nada.kth.se


#include <GL/gl.h>
#include <GL/freeglut.h>
#include <math.h>
#include <mpi.h>
#include <music.hh>
#include <iostream>
#include <sstream>
#include <vector>
#include "datafile.h"



#ifndef _VISUALISE_NEURONS
#define _VISUALISE_NEURONS

#define TIMESTEP 1e-3


class VisualiseNeurons : public MUSIC::event_handler_global_index {

 public:
  VisualiseNeurons() {
    tau_ = 50e-3;
    time_ = 0;
    stopTime_ = 0;
    oldTime_ = 0;
    done_ = 0;

    maxDist_ = 0;

  }
  
  void init(int argc, char **argv);
  void readConfigFile(string filename);
  void start();
  void finalize();

  void operator () ( double t, MUSIC::global_index id );

  void display();
  void rotateTimer();
  void tick();

  void addNeuron(double x, double y, double z, double r);

  static void displayWrapper();
  static void rotateTimerWrapper(int v);
  static void tickWrapper(int v);

  typedef struct {
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble r;
  }neuronCoord;

  typedef struct {
    GLdouble r;
    GLdouble g;
    GLdouble b;
  }neuronColour;

 private:

  MUSIC::runtime* runtime_;
  
  GLuint neuronList_;

  std::vector<neuronCoord> coords_;
  std::vector<double> volt_;

  neuronColour baseLineCol_;
  neuronColour excitedCol_;
  double spikeScale_;

  double tau_;
  double time_;
  double oldTime_;
  double stopTime_;
  int done_;

  double rotAngle_;

  int rank_;

  double maxDist_;

};

static std::vector<VisualiseNeurons*> objTable_;



#endif 
