// VisualiseNeurons.h written by Johannes Hjorth, hjorth@nada.kth.se


#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <mpi.h>
#include <music.hh>
#include <iostream>
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
    oldTime_ = 0;

    maxX_ = 0; maxY_ = 0; maxZ_ = 0;
    minX_ = 1e99; minY_ = 1e99; minZ_ = 1e99;

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

  double tau_;
  double time_;
  double oldTime_;

  double rotAngle_;

  int rank_;

  double maxX_, maxY_, maxZ_, minX_, minY_, minZ_;

};

static std::vector<VisualiseNeurons*> objTable_;



#endif 
