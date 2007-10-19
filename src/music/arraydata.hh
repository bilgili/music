namespace MUSIC {

  class arraydata : public data_map {
  public:
    arraydata (void* buffer, MPI_Datatype type, index_map* map);
  };

}
