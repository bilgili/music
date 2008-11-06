// VisualiseNeurons.cpp written by Johannes Hjorth, hjorth@nada.kth.se


#include "VisualiseNeurons.h"

void VisualiseNeurons::printHelp() {
  std::cerr << "Usage: viewevents" 
            << " configfile -timestep <dt> -scalefactor <sf>" << std::endl
            << std::endl
            << "Config file is required" << std::endl
            << "Default timestep is " << DEFAULT_TIMESTEP << std::endl
            << "Scale factor is how many times slower visualisation "
            << "time is relative to real time. "
            << "If omitted the visualisation runs at full speed."
            << std::endl;

  exit(-1);
}

void VisualiseNeurons::getArgs(int argc, char* argv[]) {

  opterr = 0; // handle errors ourselves
  while (1)
    {
      static struct option long_options[] =
	{
	  {"timestep",  required_argument, 0, 't'},
	  {"scaletime",  required_argument, 0, 's'},
	  {"help",      no_argument,       0, 'h'},
	  {0, 0, 0, 0}
	};
      /* `getopt_long' stores the option index here. */
      int option_index = 0;

      // the + below tells getopt_long not to reorder argv
      int c = getopt_long (argc, argv, "+t:s:h", long_options, &option_index);

      /* detect the end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	case 't':
	  dt_ = atof (optarg); //*fixme* error checking
          std::cout << "Using dt = " << dt_ << std::endl;
	  continue;
	case 's':
          synchFlag_ = 1;
          scaleTime_ = atof(optarg); //*fixme* error checking
          std::cout << "Using scaletime: " << scaleTime_ << std::endl;
	  continue;
	case '?':
	  break; // ignore unknown options
	case 'h':
          printHelp(); // exits

	default:
	  abort ();
	}
    }

  if (argc < optind + 1 || argc > optind + 1) {
    printHelp(); // exits
  }

  confFile_ = string(argv[optind]);

}


void VisualiseNeurons::init(int argc, char **argv) {

  // Add this object to the static-wrapper
  objTable_.push_back(this);

  // Init music
  MUSIC::setup* setup = new MUSIC::setup(argc, argv);
  MPI::Intracomm comm = setup->communicator();
  rank_ = comm.Get_rank(); 

  if(rank_ > 0) {
    std::cerr << argv[0] << " only supports one process currently!" 
              << std::endl;
    exit(-1);
  }

  // Init glut
  glutInit(&argc,argv);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);


  // Store the stop time
  setup->config ("stoptime", &stopTime_);

  // Parse inparameters
  getArgs(argc,argv);

  readConfigFile(confFile_);

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

  runtime_ = new MUSIC::runtime (setup, dt_);

  // Music done.
  

  // GLUT
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
  double fov = (is3dFlag_) ? 0.7*maxDist_/2 : 0.5*maxDist_/2;

  // std::cout << "MAX DIST: " << maxDist_ << std::endl;
  glFrustum(-fov,fov,-fov,fov,maxDist_/2,3.1*maxDist_); // Visible range 50-800
  glTranslated(0,0,-2.2*maxDist_); //-600

  double camAng = (is3dFlag_) ? 30 : -30;
  glRotated(camAng,1,0,0);
  glMatrixMode(GL_MODELVIEW);

  // Create displaylist for neuron(s)
  GLUquadric* neuronQuad = gluNewQuadric();

  int nVert = (coords_.size() < 500) ? 10 : 3;

  neuronList_ = glGenLists(1);

  glNewList(neuronList_, GL_COMPILE);
  gluSphere(neuronQuad, 1, nVert*2, nVert);
  //gluSphere(neuronQuad, 1, 20, 10);
  glEndList();
}

void VisualiseNeurons::start() {
  if(rank_ == 0) {
    void *exitStatus;

    glutDisplayFunc(displayWrapper);
    glutTimerFunc(25,rotateTimerWrapper, 1);    

    pthread_create(&tickThreadID_, NULL, tickWrapper, &synchFlag_);

    glutMainLoop();
    pthread_join(tickThreadID_,&exitStatus);

  } else {
    std::cerr << "Only run start() on rank 0" << std::endl;
  }
}

void VisualiseNeurons::finalize() {
  runtime_->finalize ();

  delete runtime_;

  //  std::cout << "Rank " << rank_ 
  //          << ": Searching for VisualiseNeurons wrapper object";
  for(int i = 0; i < objTable_.size(); i++) {
    if(objTable_[i] == this) {
      //std::cout << "found.";
    }
    else {
      //std::cout << ".";
    }
  }
  //std::cout << std::endl;

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
    //float specReflection[] = { 0.2*col, 0.2*col, 0.2*col, 1 };
    //glMaterialfv(GL_FRONT, GL_SPECULAR, specReflection);

    glTranslated(coords_[i].x,coords_[i].y,coords_[i].z);

    double scale = coords_[i].r*(1+spikeScale_*col);
    glScaled(scale,scale,scale);
    glCallList(neuronList_);
    glPopMatrix();
  }

  char buffer[20];

  sprintf(buffer,"%.3f s",time_);
  //std::cout << "Buffer: " << buffer << std::endl;
  //std::cout << time_ << std::endl;

  glLoadIdentity();
  glColor3d(0.2,0.2,0.4);
  //  glRasterPos2d(-maxDist_*1.7,-maxDist_*1.75);
  //  glRasterPos3d(-maxDist_*2,-1.7*maxDist_,0*maxDist_);
  if(is3dFlag_) {
    glColor3d(1,1,1);
    glRasterPos3d(-maxDist_*0.9,-maxDist_*0.5,maxDist_);
  }
  else {
    glRasterPos3d(-maxDist_*0.8,maxDist_*0.8,0.3*maxDist_);
  }
  for(int i = 0; i < strlen(buffer); i++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,buffer[i]);
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

}

void VisualiseNeurons::operator () (double t, MUSIC::global_index id) {
  // For now: just print out incoming events
  //std::cout << "Event " << id << " detected at " << t 
  //          << " (vis time = " << time_ << ")" <<  std::endl;

  assert(0 <= id && id < volt_.size());


  assert(time_ - 2*dt_ < t); // time_ is old timestep
  //  assert(t < time_ + 2*dt_); // Check that spike is not too old...
  if(t < time_ + 2*dt_) {
    std::cerr << "Too old spike: " << t << " sim time: " << time_ << std::endl;
  }


  volt_[id] = 1;
  
}


void VisualiseNeurons::tick() {

  if(!done_){

    // What is real time before tick is called
    // gettimeofday(&tickStartTime_,NULL);


#if 0
    // Call music to get the latest simulation data
    std::cerr << "Size: " << volt_.size() << " ";
    std::cerr << "Entering tick (" << time_ <<")...";
#endif

    runtime_->tick();

#if 0
    std::cerr << "done(" << time_ <<")." << std::endl;
#endif    


    oldTime_ = time_;
    time_ = runtime_->time ();

    // Make sure the time steps are correct
    assert(dt_ - 1e-9 < time_ - oldTime_ 
           && time_ - oldTime_ < dt_ + 1e-9);


    // Should we make the visualisation go in realTime=scaleTime_*simTime?
    if(synchFlag_) {
      // Yes, how long did we spend in the tick?
      gettimeofday(&tickEndTime_,NULL);

      // How much of the time allocated for this timestep remains?
      double delayLeft = dt_*scaleTime_ 
        - (tickEndTime_.tv_sec - tickStartTime_.tv_sec)
        - (tickEndTime_.tv_usec - tickStartTime_.tv_usec) / 1000000.0;


      if(delayLeft > 0) {        

        // We reuse the timeval for the delay
        tickDelay_.tv_sec = 0;
        tickDelay_.tv_usec = (int) (delayLeft*1e6);

        //        std::cerr << "Delay left : " << tickDelay_.tv_usec 
        //                  << " milliseconds" << std::endl;

        // Delay so that enough time passes
        select(0,0,0,0,&tickDelay_);

      } else {
        // Whoops, simulation were too slow... print error.
        std::cerr << "t = " << time_ 
                  << ": Music's tick() took "
                  << -delayLeft*1e6 << " microseconds too long to execute" 
                  << std::endl;

      }

      // Set end of this tick as start of next tick
      gettimeofday(&tickStartTime_,NULL);
    }
    

    // Decay the volt/activity
    for(int i = 0; i < volt_.size(); i++) {
      volt_[i] *= 1-(time_-oldTime_)/tau_;
    }

    // Tell GLUT to update the screen
    glutPostRedisplay();

    // Have we reached the end?
    if(time_ >= stopTime_ - dt_/2.0) {
      done_ = 1;
    }
  } else {
    std::cout << "Last tick." << std::endl;
  }

}


void VisualiseNeurons::displayWrapper() {
  for(int i = 0; i < objTable_.size(); i++) {
    VisualiseNeurons *vn = objTable_[i];
    vn->display();
  }
}

void VisualiseNeurons::rotateTimerWrapper(int v) {
  VisualiseNeurons *vn = 0;

  for(int i = 0; i < objTable_.size(); i++) {
    vn = objTable_[i];
    if(vn->is3dFlag_) {    
      vn->rotateTimer();
    }
  }

  glutPostRedisplay();
  glFinish();

  if(vn && !(vn->done_)) {
    glutTimerFunc(100,rotateTimerWrapper, 1);
  }
}



void* VisualiseNeurons::tickWrapper(void *arg) {
  VisualiseNeurons *vn = 0;

  int allDone = 0;

  while(!allDone && objTable_.size() > 0) {

    allDone = 1;

    for(int i = 0; i < objTable_.size(); i++) {
      vn = objTable_[i];
      vn->tick();

      if(vn && !vn->done_) {
        allDone = 0;
      }
    }
  }

  std::cout << "Simulation done." << std::endl;

  // Simulation done.
  glutLeaveMainLoop();

  // Thread ends, return null
  return NULL;


}


void VisualiseNeurons::readConfigFile(string filename) {
  double x, y, z, r, dist;
  double minDist = 1e66, maxR = 0;

  int i = 0;

  std::cout << "Reading : " << filename << std::endl;

  datafile in(filename);

  if (!in) {
    std::cerr << "eventsource: could not open "
              << filename << std::endl;
    abort ();
  }


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
    dist = sqrt(x*x + y*y + z*z);
    maxDist_ = (dist > maxDist_) ? dist : maxDist_;

    // All neurons are not in one plane
    if(abs(x) > 1e-9 && abs(y) > 1e-9 && abs(z) > 1e-9) {
      is3dFlag_ = 1;
    }


    // Dist and R are used to calculate spikeScale_    
    if(dist > 0) {
      minDist = (dist < minDist) ? dist : minDist;
    }

    maxR = (r > maxR) ? r : maxR;

    // std::cout << "Neuron " << i << " at " << x << "," << y << "," << z 
    //           << " radie " << r << std::endl;
    i++;

    // Read in neuron coordinates
    in >> x >> y >> z >> r;

  }

  spikeScale_ = (minDist/(3*maxR))-1;
  spikeScale_ = (spikeScale_ > 0) ? spikeScale_ : 0;

  std::cout << "Read " << i << " neuron positions" << std::endl;
  std::cout << "Setting spike scalign to " << spikeScale_ << std::endl;
}






