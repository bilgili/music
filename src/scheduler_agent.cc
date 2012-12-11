#include <music/scheduler_agent.hh>

//#define MUSIC_DEBUG
#include "music/debug.hh"


#if MUSIC_USE_MPI

#include <algorithm>
#include <iostream>
#include <cassert>
#ifdef MUSIC_DEBUG
#include <cstdlib>
#endif

namespace MUSIC {
  SchedulerAgent::SchedulerAgent(Scheduler *scheduler): scheduler_(scheduler)
  {

  }
  MulticommAgent::MulticommAgent(Scheduler *scheduler) : SchedulerAgent(scheduler)
  {

  }
  MulticommAgent::Filter1::Filter1(MulticommAgent &multCommObj):multCommObj_(multCommObj)
  {

  }
  bool
  MulticommAgent::Filter1::operator()(const Scheduler::SConnection &conn)
  {
    if(conn.scheduledSend() <= multCommObj_.time_ )
      {
        int sId = conn.preNode()->getId();
        int rId = conn.postNode()->getId();
        multCommObj_.rNodes |= 1 << rId;
        //take most later scheduled send time for each node
        multCommObj_.commTimes[rId] = multCommObj_.time_;
        if(conn.scheduledSend() >= multCommObj_.commTimes[sId])
          multCommObj_.commTimes[sId] = conn.scheduledSend();
        return true;
      }
    return false;
  }
  MulticommAgent::Filter2::Filter2(MulticommAgent &multCommObj):multCommObj_(multCommObj)
  {

  }
  bool
  MulticommAgent::Filter2::operator()(const Scheduler::SConnection &conn)
  {
    int sId = conn.preNode()->getId();
    int rId = conn.postNode()->getId();
    std::map<int,Clock>::iterator it;
    it = multCommObj_.commTimes.find(sId);
    if(it == multCommObj_.commTimes.end() || !(multCommObj_.rNodes & (1 << sId)))
      {
        multCommObj_.commTimes[sId]=conn.scheduledSend();
        multCommObj_.commTimes[rId]=multCommObj_.time_;
        multCommObj_.rNodes |= 1 << rId;
        return true;
      }
    return false;
  }

  MulticommAgent::~MulticommAgent()
  {
    for (std::vector<MultiConnector*>::iterator connector
        = multiConnectors.begin ();
        connector != multiConnectors.end ();
        ++connector)
      if (*connector != NULL)
        delete *connector;
    delete multiBuffer_;
    delete filter1;
    delete filter2;
  }
  void
  MulticommAgent::initialize()
  {
    filter1 = new Filter1(*this);
    filter2 = new Filter2(*this);
  }

  void 
  MulticommAgent::createMultiConnectors (Clock& localTime,
      MPI::Intracomm comm,
      int leader,
      std::vector<Connector*>& connectors)
  {
    multiBuffer_ = new MultiBuffer (comm, leader, connectors);
    multiConnectors.resize (Connector::idRange ());
    multiProxies = new std::vector<bool>;
    multiProxies->resize (Connector::proxyIdRange ());

    unsigned int savedSelfNode = scheduler_->selfNode ();
    for (unsigned int self_node = 0;
        self_node < scheduler_->nNodes ();
        ++self_node)
      {
        scheduler_->setSelfNode (self_node);

        create (localTime);

        localTime.ticks (-1);

        std::vector<Connector*> cCache;
        for (int i = 0; i < 100; ++i)
          {
            localTime.tick ();
            create (localTime);
          }
        //finalize
        schedule.clear ();

        localTime.reset ();
        // Now reset node and connection clocks to starting values
        scheduler_->reset();
      }

    scheduler_->setSelfNode (savedSelfNode);
    delete multiProxies;
  }

  std::vector<Connector*>
  MulticommAgent::connectorsFromMultiId (unsigned int multiId)
  {
    std::vector<Connector*> connectors;
    for (unsigned int flag = 1; multiId != 0; flag <<= 1)
      if (multiId & flag)
        {
          connectors.push_back (Connector::connectorFromIdFlag (flag));
          multiId &= ~flag;
        }
    return connectors;
  }

  bool
  MulticommAgent::tick(MUSIC::Clock &localTime)
  {
    std::vector< MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule();
        for (comm = schedule.begin();
	     comm != schedule.end() && (*comm).time <= localTime.time();
	     comm++)
	  {
	    unsigned int multiId = (*comm).multiId ();
	    if (multiId != 0)
	      multiConnectors[multiId]->tick ();
	  }
        schedule.erase(schedule.begin(),comm);
    // we continue looping while:
    // 1. we haven't found the communication from the future:   !schedule.empty()
    // 2. or the next SConnection can't be processed by the current agent: continue_= false
      } while (continue_ && schedule.empty());
    return !schedule.empty();
  }

  bool
  MulticommAgent::create (MUSIC::Clock &localTime)
  {
    std::vector< MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule();
        for (comm = schedule.begin();
	     comm != schedule.end () && (*comm).time <= localTime.time();
	     comm++)
	  {
	    unsigned int multiId = (*comm).multiId ();
	    if (multiId != 0)
	      {
		if (multiConnectors[multiId] == NULL)
		  {
		    std::vector<Connector*> connectors
		      = connectorsFromMultiId (multiId);
		    multiConnectors[multiId]
		      = new MultiConnector (multiBuffer_, connectors);
		  }
	      }
	    else
	      {
		unsigned int proxyId = (*comm).proxyId ();
		if (proxyId != 0 && !(*multiProxies)[proxyId])
		  {
		    MPI::COMM_WORLD.Create (MPI::GROUP_EMPTY);
		    MPI::COMM_WORLD.Barrier ();		
		    (*multiProxies)[proxyId] = true;
		  }
	      }
	  }
        schedule.erase(schedule.begin(),comm);
      } while (continue_ && schedule.empty());
    return !schedule.empty();
  }

  bool
  MulticommAgent::fillSchedule()
  {
    if(!schedule.empty())
      return true;
    Scheduler::SConnection last_sconn = scheduler_->last_sconn_;
    if(!last_sconn.getConnector()->idFlag())
      return false;
    Clock time = last_sconn.nextReceive(); //last connection
    std::vector<Scheduler::SConnection> nextSConnections;
    //the condition for choosing candidates for lumping:
    //all connections should have the same scheduled receiving time.
    do
      {
        nextSConnections.push_back(last_sconn);
        last_sconn = scheduler_->nextSConnection();
      }while(last_sconn.getConnector()->idFlag() && last_sconn.nextReceive() == time);
    NextMultiConnection(nextSConnections);
    scheduler_->last_sconn_ = last_sconn;
    return last_sconn.getConnector()->idFlag();
  }
  void
  MulticommAgent::NextMultiConnection(SConnectionV &candidates)
  {

    time_ = candidates[0].nextReceive();
    std::map<int, Clock> prevCommTime;
    SConnectionV::iterator iter_bound = candidates.begin();
    SConnectionV::iterator cur_bound = candidates.begin();
    do
      {
        SConnectionV::iterator start_bound = cur_bound;
        cur_bound = iter_bound;
        bool merge = true;

        while(merge && iter_bound !=candidates.end())
          {

            commTimes.clear();
            rNodes=0;

            iter_bound = std::stable_partition (std::stable_partition (cur_bound, candidates.end(), *filter1),
                candidates.end(),
                *filter2);

            //postpone sends in the multiconn for consistency
            for(SConnectionV::iterator it = cur_bound;
                it != iter_bound;
                it++)
                (*it).postponeNextSend(commTimes[(*it).preNode()->getId()]);

            //merge multiconn with the previous multiconn:
            // (to lump together connections called sequentially)
            for(std::map<int,Clock>::iterator it = prevCommTime.begin();
                merge && it != prevCommTime.end();
                it++)
              if(commTimes.count((*it).first)>0
                  && (*it).second != commTimes[(*it).first])
                merge = false;

            if(merge)
              {
                cur_bound = iter_bound;
                prevCommTime.insert(commTimes.begin(), commTimes.end());
              }
          }
        std::map<int, Clock>::iterator id_iter;
        if ((id_iter = prevCommTime.find (scheduler_->self_node))
            != prevCommTime.end ())
          scheduleMulticonn((*id_iter).second, start_bound,cur_bound);
        prevCommTime = commTimes;
      }
    while(cur_bound != candidates.end());
  }
  void
  MulticommAgent::scheduleMulticonn(Clock &time,SConnectionV::iterator first, SConnectionV::iterator last)
  {
    unsigned int multiId = 0;
    unsigned int proxyId = 0;
    MUSIC_LOG0 ("SelfNode: " << scheduler_->self_node << " Time: "<< time.time() << std::endl << "MultiConnector:");
    for (std::vector<Scheduler::SConnection>::iterator it = first;
        it != last;
        it++)
      {
        if ((*it).getConnector ()->isProxy ())
          proxyId |= (*it).getConnector ()->idFlag ();
        else
          multiId |= (*it).getConnector ()->idFlag ();
        MUSIC_LOG0 ("("<< (*it).preNode ()->getId () <<" -> "<< (*it).postNode ()->getId () << ") at " << (*it).nextSend().time () << " -> "<< (*it).nextReceive ().time ());
      }
    schedule.push_back (MultiCommObject (time.time (),
        multiId,
        proxyId));
  }


  void
  MulticommAgent::preFinalize (std::set<int> &cnn_ports)
  {
    for (std::vector<MultiConnector*>::iterator m = multiConnectors.begin ();
	 m != multiConnectors.end ();
	 ++m)
      if (*m != NULL)
	cnn_ports.insert ((*m)->connectorCode ());
  }


  void
  MulticommAgent::finalize (std::set<int> &cnn_ports)
  {
    std::vector< MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
	for (comm = schedule.begin ();
	     comm != schedule.end () && !cnn_ports.empty ();
	     ++comm)
	  {
	    unsigned int multiId = (*comm).multiId ();
	    if (multiId != 0)
	      {
		MultiConnector* m = multiConnectors[multiId];
		if (cnn_ports.find (m->connectorCode ())
		    == cnn_ports.end ())
		  continue;
		if (m->isFinalized ())
		  // an output port was finalized in tick ()
		  {
		    cnn_ports.erase (m->connectorCode ());
		    continue;
		  }
		// finalize () needs to come after isFinalized check
		// since it can itself set finalized state
		m->finalize ();
		m->tick ();
	      }
          }
        schedule.clear ();
      } while (continue_ && !cnn_ports.empty ());
  }


  UnicommAgent::UnicommAgent(Scheduler *scheduler):SchedulerAgent(scheduler)
  {
  }
  bool
  UnicommAgent::fillSchedule()
  {
    if(!schedule.empty())
      return true;
    Scheduler::SConnection last_sconn = scheduler_->last_sconn_;
    if(last_sconn.getConnector()->idFlag())
      return false;
    if ( scheduler_->self_node == last_sconn.postNode ()->getId () //input
        || scheduler_->self_node == last_sconn.preNode ()->getId ()) //output
      {
        double nextComm
        = (scheduler_->self_node == last_sconn.postNode ()->getId ()
            ? last_sconn.nextReceive ().time()
                : last_sconn.nextSend ().time ());
        schedule.time = nextComm;
        schedule.connector =  last_sconn.getConnector ();
        MUSIC_LOGN (0,"Scheduled communication:"<< last_sconn.preNode ()->getId () <<"->"<< last_sconn.postNode ()->getId () << "at(" << last_sconn.nextSend ().time () << ", "<< last_sconn.nextReceive ().time () <<")");
      }
    scheduler_->last_sconn_ = scheduler_->nextSConnection();
    return !last_sconn.getConnector()->idFlag();
  }
  bool
  UnicommAgent::tick(Clock& localTime)
  {
    bool continue_;
    do{
        continue_ = fillSchedule();
        if(!schedule.empty() && schedule.time <= localTime.time())
          {
            schedule.connector->tick();
            schedule.reset();
          }
    }
    while(continue_ && schedule.empty());
    return !schedule.empty();
  }
  void
  UnicommAgent::finalize(std::set<int> &cnn_ports)
  {
    bool continue_;
    do{
        continue_ = fillSchedule();
        if(!schedule.empty())
          {
            Connector* connector = schedule.connector;
            schedule.reset();
            if ((cnn_ports.find (connector->receiverPortCode ())
                == cnn_ports.end ()))
              continue;
            if (connector->isFinalized ())
              // an output port was finalized in tick ()
              {
                cnn_ports.erase (connector->receiverPortCode ());
                continue;
              }
            // finalize () needs to come after isFinalized check
            // since it can itself set finalized state
            connector->finalize ();
          }
    }
    while(continue_ &&  !cnn_ports.empty());
  }

}
#endif
