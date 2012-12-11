/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2012 INCF
 *
 *  MUSIC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  MUSIC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_MULTIBUFFER_HH

#include "music/music-config.hh"

#include "music/connector.hh"

#include <vector>
#include <set>
#include <map>
#include <string>

#if MUSIC_USE_MPI

namespace MUSIC {

  class Updateable {
  public:
    virtual ~Updateable () { }
    virtual void update () = 0;
  };

  class MultiConnector;

  class MultiBuffer {
    /*
     * The buffer is organized as a sequence of BLOCKs.  Each BLOCK
     * starts with an ERRORFLAG (HeaderType).
     *
     * When ERRORFLAG is clear, it is followed by a sequence of
     * BUFFERs. Each BUFFER starts with current SIZE of data followed
     * by DATA.
     *
     * When the ERRORFLAG is set, it is followed by a sequence of
     * requested buffer sizes.
     *
     * A block with a set ERRORFLAG is prepared at and sent from
     * displacement 0.
     *
     * inputConnectorMap associates each Connector with an
     * InputSubconnectorInfo.  This data structure contains a vector
     * of BufferInfo, one for each rank in the corresponding
     * OutputConnector.
     *
     * outputConnectorMap associates each OutputConnector with an
     * OutputSubconnectorInfo containing a pointer to the BufferInfo
     * corresponding to this rank.
     */
  public:
    typedef char* BufferType;
    typedef std::map<unsigned int, unsigned int> GroupMap;
  private:
    static const unsigned int ERROR_FLAG = 1;
    static const unsigned int FINALIZE_FLAG = 2;
    typedef unsigned int HeaderType;

    static HeaderType* headerPtr (void* ptr)
    {
      return static_cast<HeaderType*> (ptr);
    }

  public:
    class BufferInfo {
      unsigned int start_; // points to beginning of DATA
      unsigned int size_;  // current size of BUFFER (max data size)
    public:
      unsigned int readDataSize (BufferType buffer) const
      {
	// SIZE is stored before DATA
	return headerPtr (buffer + start_) [-1];
      }
      void writeDataSize (BufferType buffer, unsigned int size)
      {
	headerPtr (buffer + start_) [-1] = size;
      }
      unsigned int start () const { return start_; }
      void setStart (unsigned int start) { start_ = start; }
      unsigned int size () const { return size_; }
      void setSize (unsigned int size) { size_ = size; }
    };

    typedef std::vector<BufferInfo> BufferInfos;
  private:
    typedef std::vector<BufferInfo*> BufferInfoPtrs;

  public:
    class Block {
    private:
      int rank_;
      unsigned int start_; // points to ERRORFLAG
      unsigned int size_;  // includes ERRORFLAG and buffer SIZEs
      BufferInfoPtrs bufferInfoPtr_;
    public:
      int rank () const { return rank_; }
      void setRank (int rank) { rank_ = rank; }
      unsigned int start () const { return start_; }
      void setStart (unsigned int start) { start_ = start; }
      unsigned int size () const { return size_; }
      void setSize (unsigned int size) { size_ = size; }
      unsigned int nBuffers () const { return bufferInfoPtr_.size (); }
      unsigned int headerSize () const
      {
	return sizeof (HeaderType) * (1 + nBuffers ());
      }
      bool errorFlag (BufferType buffer) const
      {
	//*fixme* Can set start_ to proper offset
	unsigned int offset = rank_ == MPI::COMM_WORLD.Get_rank () ? 0 : start_;
	return *headerPtr (buffer + offset) & MultiBuffer::ERROR_FLAG;
      }
      void clearBufferErrorFlag (BufferType buffer)
      {
	*headerPtr (buffer + start_) &= ~MultiBuffer::ERROR_FLAG;
      }
      unsigned int requestedDataSize (BufferType buffer, int i) const
      {
	unsigned int offset = rank_ == MPI::COMM_WORLD.Get_rank () ? 0 : start_;
	return headerPtr (buffer + offset)[1 + i];
      }
      BufferInfoPtrs::iterator begin () { return bufferInfoPtr_.begin (); }
      BufferInfoPtrs::iterator end () { return bufferInfoPtr_.end (); }
      BufferInfoPtrs::reverse_iterator rbegin ()
      {
	return bufferInfoPtr_.rbegin ();
      }
      BufferInfoPtrs::reverse_iterator rend ()
      {
	return bufferInfoPtr_.rend ();
      }
      void push_back (BufferInfo* bi) { bufferInfoPtr_.push_back (bi); }
    };

  private:
    // allocated by realloc in order to use a minimal amount of memory at resize
    BufferType buffer_;
    unsigned int size_;
    unsigned int errorBlockSize_;
    int localLeader_;

  public:
    typedef std::vector<Block> Blocks;
  private:
    Blocks block_;

  public:
    class InputSubconnectorInfo {
      InputSubconnector* subconnector_;
      BufferInfos bufferInfo_;
    public:
      InputSubconnectorInfo (InputSubconnector* s) : subconnector_ (s) { }
      InputSubconnector* subconnector () const { return subconnector_; }
      void setSize (unsigned int size) { bufferInfo_.resize (size); }
      BufferInfos::iterator begin () { return bufferInfo_.begin (); }
      BufferInfos::iterator end () { return bufferInfo_.end (); }
    };

    class OutputSubconnectorInfo {
      OutputSubconnector* subconnector_;
      BufferInfo* bufferInfo_;
    public:
      OutputSubconnectorInfo (OutputSubconnector* s, BufferInfo* bi)
	: subconnector_ (s), bufferInfo_ (bi) { }
      OutputSubconnector* subconnector () const { return subconnector_; }
      BufferInfo* bufferInfo () const { return bufferInfo_; }
    };

  private:
    //typedef std::vector<InputSubconnectorInfo> InputSubconnectorInfos;
    //InputSubconnectorInfos inputSubconnectorInfo_;
    typedef std::map<Connector*, OutputSubconnectorInfo> OutputConnectorMap;
    OutputConnectorMap outputConnectorMap_;

    typedef std::map<Connector*, InputSubconnectorInfo> InputConnectorMap;
    InputConnectorMap inputConnectorMap_;

    GroupMap groupMap_;

    std::vector<Updateable*> multiConnectors;

  public:
    BufferType buffer () const { return buffer_; }

  private:
    unsigned int computeSize ();

  public:
    MultiBuffer (MPI::Intracomm comm,
		 int leader,
		 std::vector<Connector*>& connectors);

    unsigned int getGroupSize (int leader) { return groupMap_[leader]; }

    void addMultiConnector (Updateable* multiConnector)
    {
      multiConnectors.push_back (multiConnector);
    }

    Blocks::iterator getBlock (int rank);
    Blocks::iterator blockEnd () { return block_.end (); }
    void dumpBlocks ();

    int localLeader () const { return localLeader_; }

    InputSubconnectorInfo* getInputSubconnectorInfo (Connector* connector);

    OutputSubconnectorInfo* getOutputSubconnectorInfo (Connector* connector);

    void clearErrorFlag ()
    {
      headerPtr (buffer_)[0] = 0;
    }

    void setErrorFlag ()
    {
      headerPtr (buffer_)[0] |= MultiBuffer::ERROR_FLAG;
    }

    void writeRequestedDataSize (int i, unsigned int size)
    {
      headerPtr (buffer_)[1 + i] = size;
    }

    void restructure ();
  };


  class MultiConnector : public Updateable {
  private:
    MultiBuffer* multiBuffer_;
    MultiBuffer::BufferType buffer_;
    int connectorCode_;
    std::vector<std::pair<int, int> > connectorIds_;

    typedef MultiBuffer::Block Block;
    typedef MultiBuffer::Blocks Blocks;
    typedef std::vector<Block*> BlockPtrs;
    BlockPtrs block_;

    typedef MultiBuffer::BufferInfo BufferInfo;
    typedef MultiBuffer::BufferInfos BufferInfos;

    typedef std::vector<MultiBuffer::OutputSubconnectorInfo*> OutputSubconnectorInfos;
    OutputSubconnectorInfos outputSubconnectorInfo_;

    typedef std::vector<MultiBuffer::InputSubconnectorInfo*> InputSubconnectorInfoPtrs;
    InputSubconnectorInfoPtrs inputSubconnectorInfo_;

    typedef std::map<int, bool> BlankMap;
    BlankMap* blankMap_;
    bool* blank_;
    std::string id_; // used for debugging
    int* recvcounts_;
    int* displs_;
    bool recvcountInvalid_;
    bool restructuring_;
    //int rank_;
    //int nRanks_;
    typedef MultiBuffer::GroupMap GroupMap;
    GroupMap* groupMap_;
    MPI::Group group_;
    MPI::Intracomm comm_;

    bool writeSizes ();
    void fillBuffers ();
    void processInput ();
    void mergeGroup (int leader, bool isInput);

    int rank () const { return comm_.Get_rank (); }
    int size () const { return comm_.Get_size (); }
    //void setErrorFlag (MultiBuffer::BufferType buffer);

  public:
    //MultiConnector () { }
    MultiConnector (MultiBuffer* multiBuffer);
    MultiConnector (MultiBuffer* multiBuffer,
		    std::vector<Connector*>& connectors);
    int connectorCode () const { return connectorCode_; }
    void add (Connector* connector);
    void initialize ();
    void tick ();
    bool isFinalized ();
    void finalize ();
    void update ();
  };

}

#endif // MUSIC_USE_MPI

#define MUSIC_MULTIBUFFER_HH
#endif
