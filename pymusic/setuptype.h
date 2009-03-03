typedef struct {
  PyObject_HEAD
  /* Type-specific fields go here. */
  MUSIC::setup* theSetup;
} music_SetupObject;


int init_setup_type(void);

void attach_setup_type(PyObject* m);
