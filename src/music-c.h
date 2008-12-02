#ifndef MUSIC_C_H

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

#include "music/music-config.hh"

#if MUSIC_HAVE_SIZE_T
#include <sys/types.h>
#else
typedef int size_t;
#endif

/* Setup */

typedef struct MUSICSetup MUSICSetup;

MUSICSetup *MUSICCreateSetup (int *argc, char ***argv);
MUSICSetup *MUSICCreateSetupThread (int *argc,
					char ***argv,
					int required,
					int* provided);

/* Communicators */

#ifndef BUILDING_MUSIC_LIBRARY
MPI_Intracomm MUSICSetupCommunicator (MUSICSetup *setup);
#endif

/* Ports */

typedef struct MUSICContOutputPort MUSICContOutputPort;
typedef struct MUSICContInputPort MUSICContInputPort;
typedef struct MUSICEventOutputPort MUSICEventOutputPort;
typedef struct MUSICEventInputPort MUSICEventInputPort;
typedef struct MUSICMessageOutputPort MUSICMessageOutputPort;
typedef struct MUSICMessageInputPort MUSICMessageInputPort;

MUSICContOutputPort *MUSICPublishContOutput (MUSICSetup *setup, char *id);
MUSICContInputPort *MUSICPublishContInput (MUSICSetup *setup, char *id);
MUSICEventOutputPort *MUSICPublishEventOutput (MUSICSetup *setup, char *id);
MUSICEventInputPort *MUSICPublishEventInput (MUSICSetup *setup, char *id);
MUSICMessageOutputPort *MUSICPublishMessageOutput (MUSICSetup *setup, char *id);
MUSICMessageInputPort *MUSICPublishMessageInput (MUSICSetup *setup, char *id);

void MUSICDestroyContOutput (MUSICContOutputPort* port);
void MUSICDestroyContInput (MUSICContInputPort* port);
void MUSICDestroyEventOutput (MUSICEventOutputPort* port);
void MUSICDestroyEventInput (MUSICEventInputPort* port);
void MUSICDestroyMessageOutput (MUSICMessageOutputPort* port);
void MUSICDestroyMessageInput (MUSICMessageInputPort* port);

/* General port methods */

int MUSICContOutputPortIsConnected (MUSICContOutputPort *port);
int MUSICContInputPortIsConnected (MUSICContInputPort *port);
int MUSICEventOutputPortIsConnected (MUSICEventOutputPort *port);
int MUSICEventInputPortIsConnected (MUSICEventInputPort *port);
int MUSICMessageOutputPortIsConnected (MUSICMessageOutputPort *port);
int MUSICMessageInputPortIsConnected (MUSICMessageInputPort *port);
int MUSICContOutputPortHasWidth (MUSICContOutputPort *port);
int MUSICContInputPortHasWidth (MUSICContInputPort *port);
int MUSICEventOutputPortHasWidth (MUSICEventOutputPort *port);
int MUSICEventInputPortHasWidth (MUSICEventInputPort *port);
int MUSICContOutputPortWidth (MUSICContOutputPort *port);
int MUSICContInputPortWidth (MUSICContInputPort *port);
int MUSICEventOutputPortWidth (MUSICEventOutputPort *port);
int MUSICEventInputPortWidth (MUSICEventInputPort *port);

/* Mapping */

/* Data maps */

typedef struct MUSICDataMap MUSICDataMap;
typedef struct MUSICContData MUSICContData;
typedef struct MUSICArrayData MUSICArrayData;

/* Index maps */

typedef struct MUSICIndexMap MUSICIndexMap;
typedef struct MUSICPermutationIndex MUSICPermutationIndex;
typedef struct MUSICLinearIndex MUSICLinearIndex;


/* No arguments are optional. */

void MUSICContOutputPortMap (MUSICContOutputPort *port,
				 MUSICContData *dmap,
				 int maxBuffered);

void MUSICContInputPortMap (MUSICContInputPort *port,
				MUSICContData *dmap,
				double delay,
				int maxBuffered,
				int interpolate);

void MUSICEventOutputPortMap (MUSICEventOutputPort *port,
				  MUSICIndexMap *indices,
				  int maxBuffered);

typedef void MUSICEventHandler (double t, int id);

void MUSICEventInputPortMapGlobalIndex (MUSICEventInputPort *port,
					      MUSICIndexMap *indices,
					      MUSICEventHandler *handleEvent,
					      double accLatency,
					      int maxBuffered);

void MUSICEventInputPortMapLocalIndex (MUSICEventInputPort *port,
					     MUSICIndexMap *indices,
					     MUSICEventHandler *handleEvent,
					     double accLatency,
					     int maxBuffered);

void MUSICMessageOutputPortMap (MUSICMessageOutputPort *port,
				    int maxBuffered);

typedef void MUSICMessageHandler (double t, void *msg, size_t size);

void MUSICMessageInputPortMap (MUSICMessageInputPort *port,
				   MUSICMessageHandler *handleMessage,
				   double accLatency,
				   int maxBuffered);

/* Index maps */

MUSICPermutationIndex *MUSICCreatePermutationIndex (int *indices,
							 int size);

void MUSICDestroyPermutationIndex (MUSICPermutationIndex *Index);

MUSICLinearIndex *MUSICCreateLinearIndex (int baseIndex,
					       int size);

void MUSICDestroyLinearIndex (MUSICLinearIndex *Index);

/* Exception: The map argument can take any type of index map. */

MUSICArrayData *MUSICCreateArrayData (void *buffer,
					   MPI_Datatype type,
					   void *map);

/* Exception: MUSIC_create_linear_array_data corresponds to
   c++ music::array_data::array_data (..., ..., ..., ...) */

MUSICArrayData *MUSICCreateLinearArrayData (void *buffer,
						  MPI_Datatype type,
						  int baseIndex,
						  int size);

void MUSICDestroyArrayData (MUSICArrayData *arrayData);

/* Configuration variables */

/* Exceptions: Result is char *
   Extra maxlen argument prevents buffer overflow.
   Result is terminated by \0 unless longer than maxlen - 1 */

int MUSICConfigString (MUSICSetup *setup,
			 char *name,
			 char *result,
			 size_t maxlen);

int MUSICConfigInt (MUSICSetup *setup, char *name, int *result);

int MUSICConfigDouble (MUSICSetup *setup, char *name, double *result);

/* Runtime */

typedef struct MUSICRuntime MUSICRuntime;

MUSICRuntime *MUSICCreateRuntime (MUSICSetup *setup, double h);

void MUSICTick (MUSICRuntime *runtime);

double MUSICTime (MUSICRuntime *runtime);

/* Finalization */

void MUSICDestroyRuntime (MUSICRuntime *runtime);


#define MUSIC_C_H
#endif /* MUSIC_C_H */
