// viewevents.cpp written by Johannes Hjorth, hjorth@nada.kth.se


#include "VisualiseNeurons.h"
#include <iostream>
#include <sstream>
#include <string>
/*
double xCoord[27] = { -150, -150, -150, 0, 0, 0, 150, 150, 150,
                      -150, -150, -150, 0, 0, 0, 150, 150, 150,
                      -150, -150, -150, 0, 0, 0, 150, 150, 150 };

double yCoord[27] = { -150, 0, 150, -150, 0, 150, -150, 0, 150,
                      -150, 0, 150, -150, 0, 150, -150, 0, 150,
                      -150, 0, 150, -150, 0, 150, -150, 0, 150 };

double zCoord[27] = { -150, -150, -150, -150, -150, -150, -150, -150, -150,
                        0,   0,   0,   0,   0,   0,   0,   0,   0,
                      150, 150, 150, 150, 150, 150, 150, 150, 150};

double volt[27] = { 0.8147, 0.9058, 0.1270,
                    0.9134, 0.6324, 0.0975,
                    0.2785, 0.5469, 0.9575,
                    0.9649, 0.1576, 0.9706,
                    0.9572, 0.4854, 0.8003,
                    0.1419, 0.4218, 0.9157,  
                    0.7922, 0.9595, 0.6557,
                    0.0357, 0.8491, 0.9340,
                    0.6787, 0.7577, 0.7431 };

int maxNeurons = 27;
*/

int main(int argc, char **argv) {

  VisualiseNeurons *vn = new VisualiseNeurons();
  vn->init(argc,argv);

  /*
  // The maxNeurons check is just temporary so we do not add more data 
  // than what is tabulated above.
  for(int i = 0; i < maxNeurons; i++) {
    vn->addNeuron(xCoord[i],yCoord[i],zCoord[i]);
  }
  */

  vn->start();

  std::cout << "Done." << std::endl;

  vn->finalize();

}
