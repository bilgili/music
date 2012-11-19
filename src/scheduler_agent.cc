#include <music/scheduler_agent.hh>

#define MUSIC_DEBUG
#include "music/debug.hh"


#if MUSIC_USE_MPI

#include <algorithm>
#include <iostream>
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
        if(conn.scheduledSend() > multCommObj_.commTimes[sId])
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
        multCommObj_.rNodes |= 1 << rId;
        return true;
      }
    return false;
  }

  MulticommAgent::~MulticommAgent()
  {
    delete filter1;
    delete filter2;
  }
  void
  MulticommAgent::initialize()
  {
    filter1 = new Filter1(*this);
    filter2 = new Filter2(*this);
  }
  bool
  MulticommAgent::tick(MUSIC::Clock &localTime)
  {
    std::vector< NextCommObject>::iterator comm;
    bool done;
    do
      {
        done = fillSchedule();
        for(comm = schedule.begin(); comm != schedule.end() && (*comm).time <= localTime.time(); comm++)
          (*comm).connector->tick();
        schedule.erase(schedule.begin(),comm);
      }while (done && schedule.empty());
    return done;
  }
  bool
  MulticommAgent::fillSchedule()
  {
    if(!schedule.empty())
      return true;
    Scheduler::SConnection last_sconn = scheduler_->last_sconn_;
    if(last_sconn.getConnector()->idFlag()){
        Clock time = last_sconn.nextReceive(); //last connection
        std::vector<Scheduler::SConnection> nextSConnections;
        do
          {
            nextSConnections.push_back(last_sconn);
            last_sconn = scheduler_->nextSConnection();
          }while(last_sconn.getConnector()->idFlag() && last_sconn.nextReceive() == time);
        fillSchedule( nextSConnections);
        scheduler_->last_sconn_ = last_sconn;
        return last_sconn.getConnector()->idFlag();
    }
    return false;
  }
  void
  MulticommAgent::fillSchedule(SConnectionV &candidates)
  {

    /* remove the following block when multiconnector will be introduced */
    for ( SConnectionV::iterator it=candidates.begin() ; it < candidates.end(); it++ )
      {
        if ( scheduler_->self_node == (*it).postNode ()->getId () //input
            || scheduler_->self_node == (*it).preNode ()->getId ()) //output
          {
            double nextComm
            = (scheduler_->self_node == (*it).postNode ()->getId ()
                ? (*it).nextReceive ().time ()
                    : (*it).nextSend ().time ());
            schedule.push_back(NextCommObject(nextComm,(*it).getConnector ()));
          //  MUSIC_LOGN (2,"Scheduled communication:"<< (*it).preNode ()->getId () <<"->"<< (*it).postNode ()->getId () << "at(" << (*it).nextSend ().time () << ", "<< (*it).nextReceive ().time () <<")");
          }
      }
    /* end of the block */

    time_ = candidates[0].nextReceive();

    SConnectionV::iterator cur_bound = candidates.begin();
    SConnectionV::iterator start_bound;
    SConnectionV::iterator end_bound;
    std::map<int, Clock> prevCommTime;
    do
      {
        start_bound = cur_bound;
        end_bound = NextMultiConnection(cur_bound, candidates.end(), prevCommTime);
        printMulticonn(time_, start_bound, end_bound);
      }
    while(end_bound != candidates.end());
  }
  SConnectionV::iterator
  MulticommAgent::NextMultiConnection(std::vector<Scheduler::SConnection>::iterator &cur_bound,
      SConnectionV::iterator last,
      std::map<int, Clock> &prevCommTime)
  {
    SConnectionV::iterator iter_bound = cur_bound;
    SConnectionV::iterator res_bound;
    bool merge = true;
    while(merge && iter_bound !=last)
      {

        commTimes.clear();
        rNodes=0;

        iter_bound = std::stable_partition (std::stable_partition (cur_bound, last, *filter1),
            last,
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
    res_bound = cur_bound;
    cur_bound = iter_bound;
    prevCommTime = commTimes;
    return  res_bound;
  }
  void
  MulticommAgent::printMulticonn(Clock time, std::vector<Scheduler::SConnection>::iterator first,
      std::vector<Scheduler::SConnection>::iterator last)
  {
    MUSIC_LOG0 ("Time: "<< time.time() << std::endl << "MultiConnector:");
    for( std::vector<Scheduler::SConnection>::iterator it = first;
        it != last;
        it++)
      {
        MUSIC_LOG0 ("("<< (*it).preNode ()->getId () <<" -> "<< (*it).postNode ()->getId () << ") at " << (*it).nextSend().time () << " -> "<< (*it).nextReceive ().time ());
      }
  }
  void
  MulticommAgent::finalize(std::set<int> &cnn_ports)
  {
    std::vector< NextCommObject>::iterator comm;
    bool done;
     do
       {
         done = fillSchedule();
         for(comm = schedule.begin(); comm != schedule.end(); comm++)
           {
             Connector* connector = (*comm).connector;
             if (connector == NULL
                 || (cnn_ports.find (connector->receiverPortCode ())
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
         schedule.clear();
         std::cerr << "hangs here" << std::endl;
       }while (done && !cnn_ports.empty());
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
    if(!last_sconn.getConnector()->idFlag())
      {
        if ( scheduler_->self_node == last_sconn.postNode ()->getId () //input
            || scheduler_->self_node == last_sconn.preNode ()->getId ()) //output
          {
            double nextComm
            = (scheduler_->self_node == last_sconn.postNode ()->getId ()
                ? last_sconn.nextReceive ().time()
                    : last_sconn.nextSend ().time ());
            schedule.time = nextComm;
            schedule.connector =  last_sconn.getConnector ();
            MUSIC_LOG0 ("Scheduled communication:"<< last_sconn.preNode ()->getId () <<"->"<< last_sconn.postNode ()->getId () << "at(" << last_sconn.nextSend ().time () << ", "<< last_sconn.nextReceive ().time () <<")");
          }
        scheduler_->last_sconn_ = scheduler_->nextSConnection();
        return true;
      }
    else
      return false;

  }
  bool
  UnicommAgent::tick(Clock& localTime)
  {
    bool done;
    while( (done = fillSchedule()) && schedule.time <= localTime.time())
      {
      schedule.connector->tick();
      schedule.reset();
      }
      return done;

  }
  void
  UnicommAgent::finalize(std::set<int> &cnn_ports)
  {

    while(fillSchedule() && !cnn_ports.empty())
      {
        Connector* connector = schedule.connector;
        schedule.reset();
        if (connector == NULL
            || (cnn_ports.find (connector->receiverPortCode ())
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

}
#endif
