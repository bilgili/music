#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <mpi.h>
#include <music.hh>
#include <iostream>
#include <vector>

#ifndef _VISUALISE_NEURONS
#define _VISUALISE_NEURONS

#define TIMESTEP 1e-3

class VisualiseNeurons : public MUSIC::event_handler_global_index {

 public:
  VisualiseNeurons() {
    tau_ = 50e-3;
    time_ = 0;
    oldTime_ = 0;
  }
  
  void init(int argc, char **argv);
  void start();
  void finalize();

  void operator () ( double t, MUSIC::global_index id );

  void display();
  void rotateTimer(int v);
  void updateVolt(int v);
  void addNeuron(double x, double y, double z);
  void tick(int v);

  typedef struct {
    GLdouble x;
    GLdouble y;
    GLdouble z;
  }point3d;

 private:

  MUSIC::runtime* runtime_;
  
  GLuint neuronList_;

  std::vector<point3d> coords_;
  std::vector<double> volt_;

  double tau_;
  double time_;
  double oldTime_;

  double rotAngle_;
};

#endif 
