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

#include "music/multibuffer.hh"

#include "music/debug.hh"

#include <sstream>

extern "C" {
#include <assert.h>
}

#if MUSIC_USE_MPI

namespace MUSIC {

  /********************************************************************
   *
   * MultiBuffer
   *
   ********************************************************************/

  MultiBuffer::MultiBuffer (MPI::Intracomm comm,
			    int localLeader,
			    std::vector<Connector*>& connectors)
    : localLeader_ (localLeader)
  {
    //bool hang = true;
    //while (hang) ;
    MPI::Group worldGroup = MPI::COMM_WORLD.Get_group ();
    MPI::Group localGroup = comm.Get_group ();
    int localSize = localGroup.Get_size ();

    // maps leaders to vectors mapping local ranks to COMM_WORLD ranks
    RankMap* rankMap = new RankMap ();
    setupRankMap (comm.Get_rank (), rankMap);

    for (std::vector<Connector*>::iterator c = connectors.begin ();
	 c != connectors.end ();
	 ++c)
      {
	// supports multicommunication?
	if (!(*c)->idFlag ())
	  continue;
	
	CollectiveConnector* connector
	  = dynamic_cast<CollectiveConnector*> (*c);

	// We create InputSubconnectorInfo:s also for output connectors.
	// In this case s will be NULL.
	InputSubconnector* s
	  = dynamic_cast<InputSubconnector*> (connector->subconnector ());
	std::pair<InputConnectorMap::iterator, bool> res
	  = inputConnectorMap_.insert
	  (InputConnectorMap::value_type (connector,
					  InputSubconnectorInfo (s)));
	InputSubconnectorInfo& isi = res.first->second;

	// decide group leader and size
	int outputLeader;
	int outputSize;
	int inputLeader;
	int inputSize;
	if (connector->isInput ())
	  {
	    outputLeader = connector->remoteLeader ();
	    outputSize = connector->remoteNProcs ();
	    inputLeader = localLeader;
	    inputSize = localSize;
	  }
	else
	  {
	    outputLeader = localLeader;
	    outputSize = localSize;
	    inputLeader = connector->remoteLeader ();
	    inputSize = connector->remoteNProcs ();
	  }
	
	// remember group sizes
	groupMap_[inputLeader] = inputSize;
	groupMap_[outputLeader] = outputSize;

	// setup BufferInfo array
	isi.setSize (outputSize);

	if (!connector->isInput ())
	  {
	    // create OutputSubconnectorInfo
	    OutputSubconnector* s
	      = dynamic_cast<OutputSubconnector*> (connector->subconnector ());
	    BufferInfo* bi = &*(isi.begin ()
				+ (MPI::COMM_WORLD.Get_rank () - outputLeader));
	    outputConnectorMap_.insert
	      (OutputConnectorMap::value_type (connector,
					       OutputSubconnectorInfo (s, bi)));
	  }

	// setup Block array
	Blocks::iterator pos = getBlock (outputLeader);
	std::vector<int>& ranks = (*rankMap) [outputLeader];
	if (pos == block_.end () || pos->rank () != outputLeader)
	  {
	    // outputLeader not found in block_
	    // fill it in, creating one new Block for each rank
	    // with one BufferInfo per Block
	    int i = 0;
	    for (BufferInfos::iterator bi = isi.begin ();
		 bi != isi.end ();
		 ++bi, ++i)
	      {
		int rank = ranks[i];
		pos = getBlock (rank);
		int offset = pos - block_.begin ();
		block_.insert (pos, Block ());
		pos = block_.begin () + offset;
		pos->setRank (rank);
		pos->push_back (&*bi);
	      }
	  }
	else
	  {
	    // outputLeader's group of ranks already had Blocks in block_
	    // Insert one BufferInfo per Block
	    int i = 0;
	    for (BufferInfos::iterator bi = isi.begin ();
		 bi != isi.end ();
		 ++bi, ++i)
	      {
		getBlock (ranks[i])->push_back (&*bi);
	      }
	  }

	pos = getBlock (inputLeader);
	ranks = (*rankMap) [inputLeader];
	if (pos == block_.end () || pos->rank () != inputLeader)
	  {
	    // inputLeader's group of ranks were not represented in block_
	    // Create empty Block:s for them
	    int i = pos - block_.begin ();
	    block_.insert (pos, inputSize, Block ());
	    pos = block_.begin () + i;
	    for (int i = 0; i < inputSize; ++i)
	      {
		int rank = ranks[i];
		pos = getBlock (rank);
		int offset = pos - block_.begin ();
		block_.insert (pos, Block ());
		pos = block_.begin () + offset;
		pos->setRank (rank);
	      }
	  }

      }

    delete rankMap;

    // setup Block and BufferInfo fields

    //unsigned int start = 0;
    unsigned int start = 20; //*fixme*

    // reserve space for error block staging area
    for (Blocks::iterator b = block_.begin (); b != block_.end (); ++b)
      start = std::max (start, b->headerSize ());
    errorBlockSize_ = start;

    for (Blocks::iterator b = block_.begin (); b != block_.end (); ++b)
      {
	b->setStart (start);
	start += sizeof (HeaderType); // error flag
	unsigned int size = b->headerSize ();
	if (size < errorBlockSize_)
	  {
	    start += errorBlockSize_ - size; // add some slack
	    size = errorBlockSize_;
	  }
	b->setSize (size);
	for (BufferInfoPtrs::iterator bi = b->begin (); bi != b->end (); ++bi)
	  {
	    start += sizeof (HeaderType); // data size field
	    (*bi)->setStart (start);
	    (*bi)->setSize (0);
	  }
      }

    // allocate buffer
    size_ = start;
    buffer_ = BufferType (malloc (size_));
    
    // clear error flag in staging area
    clearErrorFlag ();

    // write header fields into buffer
    for (Blocks::iterator b = block_.begin (); b != block_.end (); ++b)
      {
	b->clearBufferErrorFlag (buffer_);
	for (BufferInfoPtrs::iterator bi = b->begin (); bi != b->end (); ++bi)
	  (*bi)->writeDataSize (buffer_, 0);
      }
  }


  void
  MultiBuffer::setupRankMap (int localRank, RankMap* rankMap)
  {
    int worldRank = MPI::COMM_WORLD.Get_rank ();
    int worldSize = MPI::COMM_WORLD.Get_size ();
    std::vector<RankInfo> rankInfos (worldSize);
    rankInfos[worldRank] = RankInfo (localLeader_, localRank, worldRank);
    MPI::COMM_WORLD.Allgather (MPI::IN_PLACE, 0, MPI::DATATYPE_NULL,
			       &rankInfos.front (),
			       sizeof (RankInfo) / sizeof (int),
			       MPI::INT);
    for (std::vector<RankInfo>::iterator ri = rankInfos.begin ();
	 ri != rankInfos.end ();
	 ++ri)
      {
	std::vector<int>& ranks = (*rankMap)[ri->leader];
	if (ranks.size () <= static_cast<unsigned int> (ri->localRank))
	  ranks.resize (ri->localRank + 1);
	ranks[ri->localRank] = ri->worldRank;
      }
  }


  unsigned int
  MultiBuffer::computeSize ()
  {
    // compute required total size
    unsigned int summedSize = 0;
    unsigned int thisRankSize = 0;
    int thisRank = MPI::COMM_WORLD.Get_rank ();
    for (Blocks::iterator b = block_.begin (); b != block_.end (); ++b)
      {
	unsigned int size;
	if (!b->errorFlag (buffer_))
	  size = b->size ();
	else
	  {
	    // this block requires more space
	    unsigned int blockSize = sizeof (HeaderType); // error flag
	    int i = 0;
	    for (BufferInfoPtrs::iterator bi = b->begin ();
		 bi != b->end ();
		 ++bi, ++i)
	      {
		blockSize += sizeof (HeaderType); // size field
		unsigned int requested = b->requestedDataSize (buffer_, i);
		if (requested > (*bi)->size ())
		  {
		    blockSize += requested;
		  }
		else
		  blockSize += (*bi)->size ();
	      }
	    size = std::max (errorBlockSize_, blockSize);
	  }
	summedSize += size;
	if (b->rank () == thisRank)
	  thisRankSize = size;
      }
    return thisRankSize + summedSize;
  }


  void
  MultiBuffer::restructure ()
  {
    unsigned int size = computeSize ();
    // resize multi-buffer
    if (size > size_)
      {
	buffer_ = BufferType (realloc (buffer_, size));
	size_ = size;
      }

    // relocate buffers
    unsigned int newStart = size;
    for (Blocks::reverse_iterator b = block_.rbegin ();
	 b != block_.rend ();
	 ++b)
      {
	if (!b->errorFlag (buffer_))
	  {
	    // move entire block with one memmove
	    newStart -= b->size ();
	    unsigned int oldStart = b->start ();
	    unsigned int offset = newStart - oldStart;
	    if (offset == 0)
	      break; // we know that there are no further error flags set
	    memmove (buffer_ + newStart, buffer_ + oldStart, b->size ());
	    b->setStart (newStart);
	    for (BufferInfoPtrs::iterator bi = b->begin ();
		 bi != b->end ();
		 ++bi)
	      (*bi)->setStart ((*bi)->start () + offset);
	  }
	else
	  {
	    unsigned int lastStart = newStart;
	    // at least one of the buffers requires more storage
	    int i = b->nBuffers () - 1;
	    for (BufferInfoPtrs::reverse_iterator bi
		   = b->rbegin ();
		 bi != b->rend ();
		 ++bi, --i)
	      {
		unsigned int oldPos = (*bi)->start () - sizeof (HeaderType);
		unsigned int oldSize = (*bi)->size ();
		unsigned int requested = b->requestedDataSize (buffer_, i);
		if (requested > oldSize)
		  (*bi)->setSize (requested);
		newStart -= (*bi)->size ();
		(*bi)->setStart (newStart);
		newStart -= sizeof (HeaderType);
		// only touch buffer contents if the block belongs to
		// current rank => error staging area is used for
		// requested buffer sizes and buffer contains output
		// data
		if (b->rank () == MPI::COMM_WORLD.Get_rank ()
		    && newStart > oldPos)
		  memmove (buffer_ + newStart,
			   buffer_ + oldPos,
			   oldSize + sizeof (HeaderType));
	      }
	    newStart -= sizeof (HeaderType); // error flag
	    unsigned int blockSize = lastStart - newStart;
	    if (blockSize < errorBlockSize_)
	      {
		newStart -= errorBlockSize_ - blockSize;
		blockSize = errorBlockSize_;
	      }
	    b->setStart (newStart);
	    b->setSize (blockSize);
	    b->clearBufferErrorFlag (buffer_);
	  }
      }
    clearErrorFlag ();

    // update all existing multiConnectors
    for (std::vector<Updateable*>::iterator c = multiConnectors.begin ();
	 c != multiConnectors.end ();
	 ++c)
      (*c)->update ();
  }

  
  MultiBuffer::Blocks::iterator
  MultiBuffer::getBlock (int rank)
  {
    // binary search (modified from Wikipedia)
    int imin = 0;
    int imax = block_.size () - 1;
    while (imax >= imin)
      {
	/* calculate the midpoint for roughly equal partition */
	int imid = (imin + imax) / 2;
 
	// determine which subarray to search
	if (block_[imid].rank () < rank)
	  // change min index to search upper subarray
	  imin = imid + 1;
	else if (block_[imid].rank () > rank)
	  // change max index to search lower subarray
	  imax = imid - 1;
	else
	  // block found at index imid
	  return block_.begin () + imid;
      }
    // rank not found---return position where rank should be inserted
    return block_.begin () + imin;
  }

  MultiBuffer::InputSubconnectorInfo*
  MultiBuffer::getInputSubconnectorInfo (Connector* connector)
  {
    InputConnectorMap::iterator pos = inputConnectorMap_.find (connector);
    assert (pos != inputConnectorMap_.end ());
    return &pos->second;
  }


  MultiBuffer::OutputSubconnectorInfo*
  MultiBuffer::getOutputSubconnectorInfo (Connector* connector)
  {
    OutputConnectorMap::iterator pos = outputConnectorMap_.find (connector);
    assert (pos != outputConnectorMap_.end ());
    return &pos->second;
  }


  void
  MultiBuffer::dumpBlocks ()
  {
    for (Blocks::iterator b = block_.begin (); b != block_.end (); ++b)
      std::cout << b->rank () << std::endl;
  }


  /********************************************************************
   *
   * MultiConnector
   *
   ********************************************************************/

  MultiConnector::MultiConnector (MultiBuffer* multiBuffer)
    : multiBuffer_ (multiBuffer),
      recvcountInvalid_ (false)
  {
    connectorCode_ = ConnectorInfo::allocPortCode ();
    buffer_ = multiBuffer_->buffer ();
    groupMap_ = new GroupMap;
    blankMap_ = new BlankMap;
    multiBuffer_->addMultiConnector (this);
  }

  //*fixme* code repetition
  MultiConnector::MultiConnector (MultiBuffer* multiBuffer,
				  std::vector<Connector*>& connectors)
    : multiBuffer_ (multiBuffer),
      recvcountInvalid_ (false)
  {
    buffer_ = multiBuffer_->buffer ();
    groupMap_ = new GroupMap;
    blankMap_ = new BlankMap;
    multiBuffer_->addMultiConnector (this);
    for (std::vector<Connector*>::iterator c = connectors.begin ();
	 c != connectors.end ();
	 ++c)
      add (*c);
    initialize ();
  }

  void
  MultiConnector::mergeGroup (int leader, bool isInput)
  {
    if (groupMap_->find (leader) == groupMap_->end ())
      {
	(*groupMap_)[leader] = multiBuffer_->getGroupSize (leader);
	(*blankMap_)[leader] = isInput;
      }
    else
      // for now:
      //assert ((*blankMap_)[leader] == isInput);
      (*blankMap_)[leader] &= isInput;
  }

  void
  MultiConnector::add (Connector* connector)
  {
    if (connector->isInput ())
      {
	// The following has to be done here so that multiConnector
	// groups are created the same way in all applications.
	//
	// BUT: Do we need to sort the connectors? We do if the
	// scheduler can deliver connectors in different order in
	// different applications.
	connectorIds_.push_back (std::make_pair (connector->remoteLeader (),
						 multiBuffer_->localLeader ()));
	mergeGroup (multiBuffer_->localLeader (), true);
	mergeGroup ((connector)->remoteLeader (), false);
	inputSubconnectorInfo_.push_back (multiBuffer_
					  ->getInputSubconnectorInfo (connector));
      }
    else
      {
	connectorIds_.push_back (std::make_pair (multiBuffer_->localLeader (),
						 connector->remoteLeader ()));
	mergeGroup ((connector)->remoteLeader (), true);
	mergeGroup (multiBuffer_->localLeader (), false);
	outputSubconnectorInfo_.push_back (multiBuffer_
					   ->getOutputSubconnectorInfo (connector));
      }
  }

  void
  MultiConnector::initialize ()
  {
    std::ostringstream ostr;
    ostr << "Rank " << MPI::COMM_WORLD.Get_rank () << ": Create ";
    {
      int nRanges = groupMap_->size ();
      int (*range)[3] = new int[nRanges][3];
      int i = 0;
      for (GroupMap::iterator g = groupMap_->begin ();
	   g != groupMap_->end ();
	   ++g)
	{
	  int leader = g->first;
	  int size = g->second;
	  ostr << leader << ", ";
	  range[i][0] = leader;
	  range[i][1] = leader + size - 1;
	  range[i][2] = 1;
	  ++i;
	}
      group_ = MPI::COMM_WORLD.Get_group ().Range_incl (nRanges, range);
      delete[] range;
    }
    ostr << std::endl;
    std::cout << ostr.str () << std::flush;

    sort (connectorIds_.begin (), connectorIds_.end ());
    std::ostringstream idstr_;
    std::vector<std::pair<int, int> >::iterator cid = connectorIds_.begin ();
    idstr_ << cid->first << cid->second;
    ++cid;
    for (; cid != connectorIds_.end (); ++cid)
      idstr_ << ':' << cid->first << cid->second;
    id_ = "mc" + idstr_.str ();

    if (group_.Get_size () < MPI::COMM_WORLD.Get_size ())
      comm_ = MPI::COMM_WORLD.Create (group_);
    else
      comm_ = MPI::COMM_WORLD;
    MPI::COMM_WORLD.Barrier ();

    std::vector<int> ranks (size ());
    std::vector<int> indices (size ());
    for (int rank = 0; rank < size (); ++rank)
      ranks[rank] = rank;
    MPI::Group::Translate_ranks (group_, size (), &ranks[0],
				 MPI::COMM_WORLD.Get_group (), &indices[0]);
    for (int rank = 0; rank < size (); ++rank)
      {
	Blocks::iterator b = multiBuffer_->getBlock (indices[rank]);
#ifdef MUSIC_DEBUG
	if (b == multiBuffer_->blockEnd () && MPI::COMM_WORLD.Get_rank () == 0)
	  {
	    std::cout << "asked for rank " << indices[rank] << " among:" << std::endl;
	    multiBuffer_->dumpBlocks ();
	  }
#endif
	assert (b != multiBuffer_->blockEnd ());
	block_.push_back (&*b);
      }

    recvcounts_ = new int[size ()];
    displs_ = new int[size ()];

    blank_ = new bool[size ()];
    int i = 0;
    for (BlankMap::iterator b = blankMap_->begin ();
	 b != blankMap_->end ();
	 ++b)
      {
	int leader = b->first;
	int size = (*groupMap_)[leader];
	int limit = i + size;
	for (; i < limit; ++i)
	  blank_[i] = b->second;
      }
    delete blankMap_;
    delete groupMap_;

    restructuring_ = true;
    update ();
    restructuring_ = false;
  }


  void
  MultiConnector::update ()
  {
    buffer_ = multiBuffer_->buffer ();
    for (OutputSubconnectorInfos::iterator osi
	   = outputSubconnectorInfo_.begin ();
	 osi != outputSubconnectorInfo_.end ();
	 ++osi)
      {
	unsigned int dataSize = (*osi)->subconnector ()->dataSize ();
	BufferInfo* bi = (*osi)->bufferInfo ();
	bi->writeDataSize (buffer_, dataSize);
	(*osi)->subconnector ()->setOutputBuffer (buffer_
						  + bi->start (),
						  bi->size ());
      }
    for (int r = 0; r < size (); ++r)
      {
	Block* block = block_[r];
	int newRecvcount = blank_[r] ? 0 : block->size ();
	if (!restructuring_ && r == rank () && newRecvcount > recvcounts_[r])
	  // Another MultiConnector has caused MultiBuffer
	  // restructuring.  We need to postpone modifying recvcount
	  // until our partner knows the new size.
	  recvcountInvalid_ = true;
	else
	  recvcounts_[r] = newRecvcount;
	displs_[r] = block->start ();
      }
  }


  bool
  MultiConnector::writeSizes ()
  {
    bool dataFits = true;
    for (OutputSubconnectorInfos::iterator osi
	   = outputSubconnectorInfo_.begin ();
	 osi != outputSubconnectorInfo_.end ();
	 ++osi)
      {
	(*osi)->subconnector ()->nextBlock ();
	unsigned int dataSize = (*osi)->subconnector ()->dataSize ();
	if (!recvcountInvalid_)
	  (*osi)->bufferInfo ()->writeDataSize (buffer_, dataSize);
	if (dataSize > (*osi)->bufferInfo ()->size ())
	    dataFits = false;
      }
    if (recvcountInvalid_ // another MultiConnector caused restructuring
	|| !dataFits)
      {
	displs_[rank ()] = 0; // error block staging area
	multiBuffer_->setErrorFlag ();
	int i = 0;
	for (OutputSubconnectorInfos::iterator osi
	       = outputSubconnectorInfo_.begin ();
	     osi != outputSubconnectorInfo_.end ();
	     ++osi, ++i)
	  {
	    unsigned int size = (*osi)->bufferInfo ()->size ();
	    unsigned int dataSize = (*osi)->subconnector ()->dataSize ();
	    multiBuffer_->writeRequestedDataSize (i, std::max (size, dataSize));
	  }
	return false;
      }
    return true;
  }


  void
  MultiConnector::fillBuffers ()
  {
    for (OutputSubconnectorInfos::iterator osi
	   = outputSubconnectorInfo_.begin ();
	 osi != outputSubconnectorInfo_.end ();
	 ++osi)
      (*osi)->subconnector ()->fillOutputBuffer ();
  }


  void
  MultiConnector::processInput ()
  {
    for (InputSubconnectorInfoPtrs::iterator isi
	   = inputSubconnectorInfo_.begin ();
	 isi != inputSubconnectorInfo_.end ();
	 ++isi)
      {
	InputSubconnector* subconnector = (*isi)->subconnector ();
	for (BufferInfos::iterator bi = (*isi)->begin ();
	     bi != (*isi)->end ();
	     ++bi)
	  subconnector->processData (buffer_ + bi->start (),
				     bi->readDataSize (buffer_));
      }
  }


  void
  dumprecvc (std::string id, int* recvc, int n)
  {
    std::ostringstream ostr;
    ostr << "Rank " << MPI::COMM_WORLD.Get_rank () << ": "
	 << id << ": Allgather "
	 << *recvc;
    for (int i = 1; i < n; ++i)
      ostr << ", " << recvc[i];
    ostr << std::endl;
    std::cout << ostr.str () << std::flush;
  }


  void
  MultiConnector::tick ()
  {
    if (writeSizes ())
      // Data will fit
      fillBuffers ();
    dumprecvc (id_, recvcounts_, comm_.Get_size ());
    comm_.Allgatherv (MPI::IN_PLACE, 0, MPI::DATATYPE_NULL,
		      buffer_, recvcounts_, displs_, MPI::BYTE);
    for (BlockPtrs::iterator b = block_.begin ();
	 b != block_.end ();
	 ++b)
      if ((*b)->errorFlag (buffer_))
	{
	  restructuring_ = true;
	  multiBuffer_->restructure ();
	  restructuring_ = false;
	  recvcountInvalid_ = false;
	  fillBuffers ();
	  dumprecvc (id_, recvcounts_, comm_.Get_size ());
	  comm_.Allgatherv (MPI::IN_PLACE, 0, MPI::DATATYPE_NULL,
			    buffer_, recvcounts_, displs_, MPI::BYTE);
	  break;
	}
    processInput ();
  }


  bool
  MultiConnector::isFinalized ()
  {
    return true;
  }


  void
  MultiConnector::finalize ()
  {
  }

}

#endif // MUSIC_USE_MPI
