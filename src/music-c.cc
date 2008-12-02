/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2008 CSC, KTH
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

#include "music.hh"

#include <string>
#include <cstring>

extern "C" {

#include "music-c.h"

/* Setup */

MUSICSetup *
MUSICCreateSetup (int *argc, char ***argv)
{
  return (MUSICSetup *) new MUSIC::Setup (*argc, *argv);
}


MUSICSetup *
MUSICCreateSetupThread (int *argc, char ***argv, int required, int *provided)
{
  return (MUSICSetup *) new MUSIC::Setup (*argc, *argv, required, provided);
}


/* Communicators */

MPI_Comm
MUSICSetupCommunicatorGlue (MUSICSetup *setup)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MPI_Comm) cxxSetup->communicator ();
}


/* Port creation */

MUSICContOutputPort *
MUSICPublishContOutput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICContOutputPort *) cxxSetup->publishContOutput (id);
}


MUSICContInputPort *
MUSICPublishContInput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICContInputPort *) cxxSetup->publishContInput(id);
}


MUSICEventOutputPort *
MUSICPublishEventOutput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICEventOutputPort *) cxxSetup->publishEventOutput(id);
}


MUSICEventInputPort *
MUSICPublishEventInput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICEventInputPort *) cxxSetup->publishEventInput(id);
}


MUSICMessageOutputPort *
MUSICPublishMessageOutput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICMessageOutputPort *) cxxSetup->publishMessageOutput(id);
}


MUSICMessageInputPort *
MUSICPublishMessageInput (MUSICSetup *setup, char *id)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICMessageInputPort *) cxxSetup->publishMessageInput(id);
}


void
MUSICDestroyContOutput (MUSICContOutputPort* Port)
{
  delete (MUSIC::ContOutputPort *) Port;
}


void
MUSICDestroyContInput (MUSICContInputPort* Port)
{
  delete (MUSIC::ContInputPort *) Port;
}


void
MUSICDestroyEventOutput (MUSICEventOutputPort* Port)
{
  delete (MUSIC::EventOutputPort *) Port;
}


void
MUSICDestroyEventInput (MUSICEventInputPort* Port)
{
  delete (MUSIC::EventInputPort *) Port;
}


void
MUSICDestroyMessageOutput (MUSICMessageOutputPort* Port)
{
  delete (MUSIC::MessageOutputPort *) Port;
}


void
MUSICDestroyMessageInput (MUSICMessageInputPort* Port)
{
  delete (MUSIC::MessageInputPort *) Port;
}


/* General port methods */

int
MUSICContOutputPortIsConnected (MUSICContOutputPort *Port)
{
  MUSIC::ContOutputPort* cxxPort = (MUSIC::ContOutputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICContInputPortIsConnected (MUSICContInputPort *Port)
{
  MUSIC::ContInputPort* cxxPort = (MUSIC::ContInputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICEventOutputPortIsConnected (MUSICEventOutputPort *Port)
{
  MUSIC::EventOutputPort* cxxPort = (MUSIC::EventOutputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICEventInputPortIsConnected (MUSICEventInputPort *Port)
{
  MUSIC::EventInputPort* cxxPort = (MUSIC::EventInputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICMessageOutputPortIsConnected (MUSICMessageOutputPort *Port)
{
  MUSIC::MessageOutputPort* cxxPort = (MUSIC::MessageOutputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICMessageInputPortIsConnected (MUSICMessageInputPort *Port)
{
  MUSIC::MessageInputPort* cxxPort = (MUSIC::MessageInputPort *) Port;
  return cxxPort->isConnected ();
}


int
MUSICContOutputPortHasWidth (MUSICContOutputPort *Port)
{
  MUSIC::ContOutputPort* cxxPort = (MUSIC::ContOutputPort *) Port;
  return cxxPort->hasWidth ();
}


int
MUSICContInputPortHasWidth (MUSICContInputPort *Port)
{
  MUSIC::ContInputPort* cxxPort = (MUSIC::ContInputPort *) Port;
  return cxxPort->hasWidth ();
}


int
MUSICEventOutputPortHasWidth (MUSICEventOutputPort *Port)
{
  MUSIC::EventOutputPort* cxxPort = (MUSIC::EventOutputPort *) Port;
  return cxxPort->hasWidth ();
}


int
MUSICEventInputPortHasWidth (MUSICEventInputPort *Port)
{
  MUSIC::EventInputPort* cxxPort = (MUSIC::EventInputPort *) Port;
  return cxxPort->hasWidth ();
}


int
MUSICContOutputPortWidth (MUSICContOutputPort *Port)
{
  MUSIC::ContOutputPort* cxxPort = (MUSIC::ContOutputPort *) Port;
  return cxxPort->width ();
}


int
MUSICContInputPortWidth (MUSICContInputPort *Port)
{
  MUSIC::ContInputPort* cxxPort = (MUSIC::ContInputPort *) Port;
  return cxxPort->width ();
}


int
MUSICEventOutputPortWidth (MUSICEventOutputPort *Port)
{
  MUSIC::EventOutputPort* cxxPort = (MUSIC::EventOutputPort *) Port;
  return cxxPort->width ();
}


int
MUSICEventInputPortWidth (MUSICEventInputPort *Port)
{
  MUSIC::EventInputPort* cxxPort = (MUSIC::EventInputPort *) Port;
  return cxxPort->width ();
}


/* Mapping */

/* No arguments are optional. */

void
MUSICContOutputPortMap (MUSICContOutputPort *Port,
			    MUSICContData *dmap,
			    int maxBuffered)
{
  MUSIC::ContOutputPort* cxxPort = (MUSIC::ContOutputPort *) Port;
  MUSIC::ContData* cxxDmap = (MUSIC::ContData *) dmap;
  cxxPort->map (cxxDmap, maxBuffered);
}


void
MUSICContInputPortMap (MUSICContInputPort *Port,
			   MUSICContData *dmap,
			   double delay,
			   int maxBuffered,
			   int interpolate)
{
  MUSIC::ContInputPort* cxxPort = (MUSIC::ContInputPort *) Port;
  MUSIC::ContData* cxxDmap = (MUSIC::ContData *) dmap;
  cxxPort->map (cxxDmap, delay, maxBuffered, interpolate);
}


void
MUSICEventOutputPortMapGlobalIndex (MUSICEventOutputPort *Port,
					  MUSICIndexMap *indices,
					  int maxBuffered)
{
  MUSIC::EventOutputPort* cxxPort = (MUSIC::EventOutputPort *) Port;
  MUSIC::IndexMap* cxxIndices = (MUSIC::IndexMap *) indices;
  cxxPort->map (cxxIndices, MUSIC::Index::GLOBAL, maxBuffered);
}


void
MUSICEventOutputPortMapLocalIndex (MUSICEventOutputPort *Port,
					  MUSICIndexMap *indices,
					  int maxBuffered)
{
  MUSIC::EventOutputPort* cxxPort = (MUSIC::EventOutputPort *) Port;
  MUSIC::IndexMap* cxxIndices = (MUSIC::IndexMap *) indices;
  cxxPort->map (cxxIndices, MUSIC::Index::LOCAL, maxBuffered);
}


typedef void MUSICEventHandler (double t, int id);

void
MUSICEventInputPortMapGlobalIndex (MUSICEventInputPort *Port,
					 MUSICIndexMap *indices,
					 MUSICEventHandler *handleEvent,
					 double accLatency,
					 int maxBuffered)
{
  MUSIC::EventInputPort* cxxPort = (MUSIC::EventInputPort *) Port;
  MUSIC::IndexMap* cxxIndices = (MUSIC::IndexMap *) indices;
  MUSIC::EventHandlerGlobalIndexProxy* cxxHandleEvent =
    cxxPort->allocEventHandlerGlobalIndexProxy (handleEvent);
  cxxPort->map (cxxIndices, cxxHandleEvent, accLatency, maxBuffered);
}


void
MUSICEventInputPortMapLocalIndex (MUSICEventInputPort *Port,
					 MUSICIndexMap *indices,
					 MUSICEventHandler *handleEvent,
					 double accLatency,
					 int maxBuffered)
{
  MUSIC::EventInputPort* cxxPort = (MUSIC::EventInputPort *) Port;
  MUSIC::IndexMap* cxxIndices = (MUSIC::IndexMap *) indices;
  MUSIC::EventHandlerLocalIndexProxy* cxxHandleEvent =
    cxxPort->allocEventHandlerLocalIndexProxy (handleEvent);
  cxxPort->map (cxxIndices, cxxHandleEvent, accLatency, maxBuffered);
}


void
MUSICMessageOutputPortMap (MUSICMessageOutputPort *Port,
			       int maxBuffered)
{
  MUSIC::MessageOutputPort* cxxPort = (MUSIC::MessageOutputPort *) Port;
  cxxPort->map (maxBuffered);
}


typedef void MUSICMessageHandler (double t, void *msg, size_t size);

void
MUSICMessageInputPortMap (MUSICMessageInputPort *Port,
			      MUSICMessageHandler *handleMessage,
			      double accLatency,
			      int maxBuffered)
{
  MUSIC::MessageInputPort* cxxPort = (MUSIC::MessageInputPort *) Port;
  MUSIC::MessageHandler* cxxHandleMessage = (MUSIC::MessageHandler *) handleMessage;
  cxxPort->map (cxxHandleMessage, accLatency, maxBuffered);
}


/* Index maps */

MUSICPermutationIndex *
MUSICCreatePermutationIndex (int *indices,
				int size)
{
  void* data = static_cast<void*> (indices);
  return (MUSICPermutationIndex *)
    new MUSIC::PermutationIndex (static_cast<MUSIC::GlobalIndex*> (data),
				  size);
}


void
MUSICDestroyPermutationIndex (MUSICPermutationIndex *Index)
{
  delete (MUSIC::PermutationIndex *) Index;
}


MUSICLinearIndex *
MUSICCreateLinearIndex (int baseIndex,
			   int size)
{
  return (MUSICLinearIndex *) new MUSIC::LinearIndex (baseIndex, size);
}


void
MUSICDestroyLinearIndex (MUSICLinearIndex *Index)
{
  delete (MUSIC::LinearIndex *) Index;
}


/* Data maps */

/* Exception: The map argument can take any type of index map. */

MUSICArrayData *
MUSICCreateArrayData (void *buffer,
			 MPI_Datatype type,
			 void *map)
{
  MUSIC::IndexMap* cxxMap = (MUSIC::IndexMap *) map;
  return (MUSICArrayData *) new MUSIC::ArrayData (buffer, type, cxxMap);
}


/* Exception: MUSIC_create_linear_array_data corresponds to
   c++ music::array_data::array_data (..., ..., ..., ...) */

MUSICArrayData *
MUSICCreateLinearArrayData (void *buffer,
				MPI_Datatype type,
				int baseIndex,
				int size)
{
}


void
MUSICDestroyArrayData (MUSICArrayData *ArrayData)
{
  delete (MUSIC::ArrayData *) ArrayData;
}


/* Configuration variables */

/* Exceptions: Result is char *
   Extra maxlen argument prevents buffer overflow.
   Result is terminated by \0 unless longer than maxlen - 1 */

int
MUSICConfigString (MUSICSetup *setup,
		     char *name,
		     char *result,
		     size_t maxlen)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  std::string cxxResult;
  int res = cxxSetup->config (string (name), &cxxResult);
  strncpy(result, cxxResult.c_str (), maxlen);
  return res;
}


int
MUSICConfigInt (MUSICSetup *setup, char *name, int *result)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return cxxSetup->config (string(name), result);
}


int
MUSICConfigDouble (MUSICSetup *setup, char *name, double *result)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return cxxSetup->config (string(name), result);
}


/* Runtime */

MUSICRuntime *
MUSICCreateRuntime (MUSICSetup *setup, double h)
{
  MUSIC::Setup* cxxSetup = (MUSIC::Setup *) setup;
  return (MUSICRuntime *) new MUSIC::Runtime (cxxSetup, h);
}


void
MUSICTick (MUSICRuntime *runtime)
{
  MUSIC::Runtime* cxxRuntime = (MUSIC::Runtime *) runtime;
  cxxRuntime->tick ();
}


double
MUSICTime (MUSICRuntime *runtime)
{
  MUSIC::Runtime* cxxRuntime = (MUSIC::Runtime *) runtime;
  return cxxRuntime->time ();
}


/* Finalization */

void
MUSICDestroyRuntime (MUSICRuntime *runtime)
{
  MUSIC::Runtime* cxxRuntime = (MUSIC::Runtime *) runtime;
  delete cxxRuntime;
}

}
