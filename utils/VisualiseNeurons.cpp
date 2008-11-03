#include "VisualiseNeurons.h"

void VisualiseNeurons::init(int argc, char **argv) {

  // Init glut
  glutInit(&argc,argv);

  // Init music
  MUSIC::setup* setup = new MUSIC::setup(argc, argv);
  MPI::Intracomm comm = setup->communicator();
  int rank = comm.Get_rank(); 

  if(rank > 0) {
    std::cerr << argv[0] << " only supports one process currently!" 
              << std::endl;
    exit(-1);
  }

  MUSIC::event_input_port* evport = setup->publish_event_input("plot");

  if (!evport->is_connected()) {
    if (rank == 0)
      std::cerr << "eventlog port is not connected" << std::endl;
    exit (1);
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
  glFrustum(-40,40,-40,40,50,800); // Visible range
  glTranslated(0,0,-600);
  glRotated(30,1,0,0);
  glMatrixMode(GL_MODELVIEW);

  // Create displaylist for neuron(s)
  GLUquadric* neuronQuad = gluNewQuadric();

  neuronList_ = glGenLists(1);
  glNewList(neuronList_, GL_COMPILE);
  gluSphere(neuronQuad, 30, 20, 10);
  glEndList();
}

void VisualiseNeurons::start() {
  glutDisplayFunc(display);
  glutTimerFunc(25,rotateTimer, 1);
  glutTimerFunc(1,tick, 1);

  glutMainLoop();
}

void VisualiseNeurons::finalize() {
  runtime_->finalize ();

  delete runtime_;

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
    GLdouble red =   0.25 + 0.75*col;
    GLdouble green = 0.53 + 0.37*col;
    GLdouble blue =  0.10 - 0.10*col;

    glColor3d(red,green,blue);

    glTranslated(coords_[i].x,coords_[i].y,coords_[i].z);
    glCallList(neuronList_);
    glPopMatrix();
  }

  glutSwapBuffers();

}



void VisualiseNeurons::addNeuron(double x, double y, double z) {
  point3d p;
  p.x = x; p.y = y; p.z = z;
  coords_.push_back(p);
  volt_.push_back(0);
}

void VisualiseNeurons::rotateTimer(int v) {
  rotAngle_ += 0.5;

  if(rotAngle_ >= 36000) {
    rotAngle_ -= 36000;
  }

  glutPostRedisplay();
  glutTimerFunc(25,rotateTimer, 1);
 

}

void VisualiseNeurons::operator () (double t, MUSIC::global_index id) {
  // For now: just print out incoming events
  std::cout << "Event " << id << " detected at " << t << std::endl;
  
  volt_[id] = 1;
  
}


void VisualiseNeurons::tick(int v) {

  runtime_->tick ();

  oldTime_ = time_;
  time_ = runtime_->time ();

  // This function should call the music tick() if needed

  for(int i = 0; i < volt_.size(); i++) {
    volt_[i] *= (time_-oldTime_)/tau_;
  }

  glutTimerFunc(1,tick, 1);

}


