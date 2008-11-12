// viewevents.cpp written by Johannes Hjorth, hjorth@nada.kth.se


#include "VisualiseNeurons.h"
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char **argv) {

  VisualiseNeurons *vn = new VisualiseNeurons();
  vn->run(argc,argv);

  std::cout << "Done." << std::endl;

  vn->finalize();

}
