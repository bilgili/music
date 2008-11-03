// VisualiseNeurons.cpp written by Johannes Hjorth, hjorth@nada.kth.se


#include "VisualiseNeurons.h"

void VisualiseNeurons::init(int argc, char **argv) {

  // Add this object to the static-wrapper
  objTable_.push_back(this);


  // Init glut
  glutInit(&argc,argv);

  // Init music
  MUSIC::setup* setup = new MUSIC::setup(argc, argv);
  MPI::Intracomm comm = setup->communicator();
  rank_ = comm.Get_rank(); 

  if(rank_ > 0) {
    std::cerr << argv[0] << " only supports one process currently!" 
              << std::endl;
    exit(-1);
  }

  if(argc < 2) {
    std::cerr << argv[0] << " requires a config-file as inparameter." 
              << std::endl;
    exit(-1);
  }

  string confFile(argv[1]);

  readConfigFile(confFile);


  MUSIC::event_input_port* evport = setup->publish_event_input("plot");


  if (!evport->is_connected()) {
    if (rank_ == 0)
      std::cerr << "eventlog port is not connected" << std::endl;
    exit (1);
  }

  if(evport->width() != coords_.size()) {
    std::cerr << "Size mismatch: port width " << evport->width()
              << " number of neurons to plot " << coords_.size()
              << std::endl;
  }



  MUSIC::linear_index indexmap(0, evport->width());
  evport->map (&indexmap, this, 0.0);

  double stoptime;
  setup->config ("stoptime", &stoptime);

  runtime_ = new MUSIC::runtime (setup, TIMESTEP);

  // Music done.


  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(50, 50);
  glutCreateWindow("Neuron population");

  // Initialise
  glEnable(GL_DEPTH_TEST);
  glClearColor (0.0,0.0,0.0,1.0);

  GLfloat pos[4] = { -100, 100, 200, 0.0 };
  GLfloat light[4] = { 1.0, 1.0, 1.0, 1.0};

  // Lightsources
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  light);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  // Default materials
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);

  // Setup camera
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double zTrans = (4*minZ_ < -4*maxZ_) ? 4*minZ_ : -4*maxZ_;
  double zDist = abs(zTrans) + 2*((maxZ_ > -minZ_) ? maxZ_ : -minZ_);

  glFrustum(-40,40,-40,40,50,zDist); // Visible range 50-800
  glTranslated(0,0,zTrans); //-600
  glRotated(30,1,0,0);
  glMatrixMode(GL_MODELVIEW);

  // Create displaylist for neuron(s)
  GLUquadric* neuronQuad = gluNewQuadric();

  neuronList_ = glGenLists(1);
  glNewList(neuronList_, GL_COMPILE);
  gluSphere(neuronQuad, 1, 20, 10);
  glEndList();
}

void VisualiseNeurons::start() {
  if(rank_ == 0) {
    glutDisplayFunc(displayWrapper);
    glutTimerFunc(25,rotateTimerWrapper, 1);
    glutTimerFunc(1,tickWrapper, 1);

    glutMainLoop();
  } else {
    std::cerr << "Only run start() on rank 0" << std::endl;
  }
}

void VisualiseNeurons::finalize() {
  runtime_->finalize ();

  delete runtime_;

  std::cout << "Rank " << rank_ 
            << ": Searching for VisualiseNeurons wrapper object";
  for(int i = 0; i < objTable_.size(); i++) {
    if(objTable_[i] == this) {
      std::cout << "found.";
    }
    else {
      std::cout << ".";
    }
  }
  std::cout << std::endl;

  // Should delete all other objects also.

}


void VisualiseNeurons::display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glRotated(rotAngle_,0.1*sin(rotAngle_/100),1,0);

  for(int i = 0; i < coords_.size(); i++) {
    // Here we translate coordinate system and draw a neuron
    glPushMatrix();

    //double vMin = -100e-3;
    //double vMax = 100e-3;

    double col = volt_[i]; //(volt_[i] - vMin)/(vMax - vMin);

    // maxCol is [1 0.9 0]
    //    GLdouble red =   0.25 + 0.75*col;
    //    GLdouble green = 0.53 + 0.37*col;
    //    GLdouble blue =  0.10 - 0.10*col;
    GLdouble red   = baseLineCol_.r + (excitedCol_.r-baseLineCol_.r)*col;
    GLdouble green = baseLineCol_.g + (excitedCol_.g-baseLineCol_.g)*col;
    GLdouble blue  = baseLineCol_.b + (excitedCol_.b-baseLineCol_.b)*col;

    glColor3d(red,green,blue);
    glTranslated(coords_[i].x,coords_[i].y,coords_[i].z);
    glScaled(coords_[i].r, coords_[i].r, coords_[i].r);
    glCallList(neuronList_);
    glPopMatrix();
  }

  glutSwapBuffers();

}



void VisualiseNeurons::addNeuron(double x, double y, double z, double r) {
  neuronCoord p;
  p.x = x; p.y = y; p.z = z; p.r = r;
  coords_.push_back(p);
  volt_.push_back(0);
}

void VisualiseNeurons::rotateTimer() {
  rotAngle_ += 0.5;

  if(rotAngle_ >= 36000) {
    rotAngle_ -= 36000;
  }

  glutPostRedisplay();

}

void VisualiseNeurons::operator () (double t, MUSIC::global_index id) {
  // For now: just print out incoming events
  std::cout << "Event " << id << " detected at " << t << std::endl;
  
  volt_[id] = 1;
  
}


void VisualiseNeurons::tick() {

  runtime_->tick ();

  oldTime_ = time_;
  time_ = runtime_->time ();

  // This function should call the music tick() if needed

  for(int i = 0; i < volt_.size(); i++) {
    volt_[i] *= 1-(time_-oldTime_)/tau_;
  }

}


void VisualiseNeurons::displayWrapper() {
  for(int i = 0; i < objTable_.size(); i++) {
    VisualiseNeurons *vn = objTable_[i];
    vn->display();
  }
}

void VisualiseNeurons::rotateTimerWrapper(int v) {
  for(int i = 0; i < objTable_.size(); i++) {
    VisualiseNeurons *vn = objTable_[i];
    vn->rotateTimer();
  }

  glutTimerFunc(25,rotateTimerWrapper, 1);

}

void VisualiseNeurons::tickWrapper(int v) {
  for(int i = 0; i < objTable_.size(); i++) {
    VisualiseNeurons *vn = objTable_[i];
    vn->tick();
  }

  glutTimerFunc(1,tickWrapper, 1);

}

void VisualiseNeurons::readConfigFile(string filename) {
  double x, y, z, r;

  int i = 0;

  std::cout << "Reading : " << filename << std::endl;

  datafile in(filename);

  // !!! WHY?!!
  // VisualiseNeurons.cpp:246: undefined reference to `datafile::skip_header()'
  //in.skip_header();

  // Read in neuron base colour
  in >> baseLineCol_.r >> baseLineCol_.g >> baseLineCol_.b;

  // Read in neuron excited colour
  in >> excitedCol_.r >> excitedCol_.g>> excitedCol_.b;

  // Read in neuron coordinates
  in >> x >> y >> z >> r;

  while(!in.eof()) {
    addNeuron(x,y,z,r);

    // std::cout << "Neuron " << i << " at " << x << "," << y << "," << z 
    //           << " radie " << r << std::endl;
    i++;

    // Read in neuron coordinates
    in >> x >> y >> z >> r;

    maxX_ = (x > maxX_) ? x : maxX_;
    maxY_ = (y > maxY_) ? y : maxY_;
    maxZ_ = (z > maxZ_) ? z : maxZ_;

    minX_ = (x < minX_) ? x : minX_;
    minY_ = (y < minY_) ? y : minY_;
    minZ_ = (z < minZ_) ? z : minZ_;

  }

  std::cout << "Read " << i << " neuron positions" << std::endl;
}






