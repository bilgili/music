namespace MUSIC {

  class array_data : public data_map {
  public:
    array_data (void* buffer, MPI_Datatype type, index_map* map);
  };

}
