typedef struct {
  PyObject_HEAD
  /* Type-specific fields go here. */
  MUSIC::runtime* theRuntime;
} music_RuntimeObject;

int init_runtime_type(void);

void attach_runtime_type(PyObject* m);
