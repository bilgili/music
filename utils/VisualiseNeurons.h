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
#include <assert.h>


#ifndef _VISUALISE_NEURONS
#define _VISUALISE_NEURONS

#define TIMESTEP 1e-3
#define PI 3.141592653589793


class VisualiseNeurons : public MUSIC::event_handler_global_index {

 public:
  VisualiseNeurons() {
    tau_ = 10e-3;
    time_ = 0;
    stopTime_ = 0;
    oldTime_ = 0;
    done_ = 0;

    maxDist_ = 0;
    is3dFlag_ = 0;
  }
  
  void init(int argc, char **argv);
  void readConfigFile(string filename);
  void start();
  void finalize();

  // Event handler for incomming spikes
  void operator () ( double t, MUSIC::global_index id );

  void display();
  void rotateTimer();
  void tick();

  void addNeuron(double x, double y, double z, double r);

  // Static wrapper functions
  static void displayWrapper();
  static void rotateTimerWrapper(int v);
  static void tickWrapper(int v);

  typedef struct {
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble r; // radie of neuron
  }neuronCoord;

  typedef struct {
    GLdouble r;
    GLdouble g;
    GLdouble b;
  }neuronColour;

 private:

  MUSIC::runtime* runtime_; // Music runtime object
  
  GLuint neuronList_;  // OpenGL list for drawing object

  std::vector<neuronCoord> coords_;  // Coordinates of neuron population
  std::vector<double> volt_;  // Activity of neuron population
 
  neuronColour baseLineCol_;  // Colour of resting neuron
  neuronColour excitedCol_;   // Colour of spiking neuron
  double spikeScale_;         // eg, 0.1 = scale up spiking neurons by 10%

  double tau_;       // Tau decay of activity
  double time_;      // Current time
  double oldTime_;   // Previous timestep
  double stopTime_;  // End of simulations
  int done_;         // Simulation done?

  double rotAngle_;  // Current rotation angle

  int rank_;         // Rank of this process

  double maxDist_;   // Distance from origo to outermost neuron
                     // Used when calculating camera position

  int is3dFlag_;

};

static std::vector<VisualiseNeurons*> objTable_;



#endif 
