/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2007, 2008, 2009 INCF
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

#ifndef MUSIC_RUNTIME_HH
//#define MUSIC_DEBUG
#include "music/debug.hh"
#ifdef USE_MPI
#include <mpi.h>
#include <vector>
#include <queue>

#include "music/setup.hh"
#include "music/port.hh"
#include "music/clock.hh"
#include "music/connector.hh"
#include "music/scheduler.hh"

namespace MUSIC {

  /*
   * This is the Runtime object in the MUSIC API
   *
   * It is documented in section 4.4 of the MUSIC manual
   */

  class Runtime {
  public:
    Runtime (Setup* s, double h);
    ~Runtime ();
    
    MPI::Intracomm communicator ();

    void finalize ();

    void tick ();

    double time ();
    
  private:
    Clock localTime;
    Clock nextComm;
    std::string app_name;
    std::queue<Connector *> schedule;
    MPI::Intracomm comm;
    std::vector<Port*> ports;
    std::vector<TickingPort*> tickingPorts;
    std::vector<Connector*> connectors;
   // std::vector<Subconnector*> schedule;
    std::vector<PostCommunicationConnector*> postCommunication;
    std::vector<PreCommunicationConnector*> preCommunication;
    Scheduler *scheduler;
    static bool isInstantiated_;

    typedef std::vector<Connection*> Connections;
  //  typedef std::vector<OutputSubconnector*> OutputSubconnectors;
  //  typedef std::vector<InputSubconnector*> InputSubconnectors;
 //   OutputSubconnectors outputSubconnectors;
 //   InputSubconnectors inputSubconnectors;
    /* remedius
     * new type of subconnectors- CollectiveSubconnectors was introduced
     * in order to implement collective communication
     */
  //  typedef std::vector<CollectiveSubconnector*> CollectiveSubconnectors;
    //typedef std::vector<Subconnector*> Subconnectors;
    
    void takeTickingPorts (Setup* s);
    void connectToPeers (Connections* connections);
    void specializeConnectors (Connections* connections);
 //   void spatialNegotiation (OutputSubconnectors&, InputSubconnectors&, CollectiveSubconnectors&);
    void spatialNegotiation ();
/*    void buildSchedule (
			OutputSubconnectors&,
			InputSubconnectors&,
			CollectiveSubconnectors& );*/
    void takePreCommunicators ();
    void takePostCommunicators ();
    void buildTables (Setup* s);
    void temporalNegotiation (Setup* s, Scheduler *scheduler, Connections* connections);
    void initialize ();
    /* remedius
     * cast Subconnector* returned by spatialNegotiation() to an appropriate type of Subconnector:
     * OutputSubconnectors, InputSubconnectors or CollectiveSubconnectors.
     * Type separation is needed by buildSchedule() function.
     */
/*    template<class Target>
    class Subconnector2Target {
    public:
    	Target* operator ()( Subconnector* value ) const
    	{  return dynamic_cast<Target*>(value);  }
    };*/
   // int rankToNode(ApplicationMap* applicationMap);
 /*   class MDCriteriaSorter
    {
    	std::string app_name_;
    public:
    	MDCriteriaSorter(std::string app_name):app_name_(app_name){};
    	bool operator() (const Connector* c1, const Connector* c2);
    };*/
  };

}
#endif
#define MUSIC_RUNTIME_HH
#endif
