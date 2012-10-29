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
  MulticommAgent::fillSchedule(std::vector<std::pair<double, Connector *> > &schedule,
      Scheduler::SConnection &last_sconn )
  {
    if(true) //nextSConnections[0].idFlag()) //if needs multicommunication ?
      {
        Clock time = last_sconn.nextReceive(); //last connection
        std::vector<Scheduler::SConnection> nextSConnections;
        do
          {
            nextSConnections.push_back(last_sconn);
            last_sconn = scheduler_->nextSConnection();
          }while(last_sconn.nextReceive() == time);//while(//conn.getConnector()->idFlag() &&
        fillSchedule(schedule, nextSConnections);
        return true;
      }
    else
      return false;
  }
  void
  MulticommAgent::fillSchedule( std::vector<std::pair<double, Connector *> > &schedule,SConnectionV &candidates)
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
            schedule.push_back
            (std::pair<double, Connector*>(nextComm,
                (*it).getConnector ()));
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
  UnicommAgent::UnicommAgent(Scheduler *scheduler):SchedulerAgent(scheduler)
  {

  }
  bool
  UnicommAgent::fillSchedule(std::vector<std::pair<double, Connector *> > &schedule,
      Scheduler::SConnection &last_sconn)
  {

    if(last_sconn.getConnector()->idFlag())
      {
        if ( scheduler_->self_node == last_sconn.postNode ()->getId () //input
            || scheduler_->self_node == last_sconn.preNode ()->getId ()) //output
          {
            double nextComm
            = (scheduler_->self_node == last_sconn.postNode ()->getId ()
                ? last_sconn.nextReceive ().time()
                    : last_sconn.nextSend ().time ());
            schedule.push_back
            (std::pair<double, Connector*>(nextComm,
                last_sconn.getConnector ()));
            MUSIC_LOG0 ("Scheduled communication:"<< last_sconn.preNode ()->getId () <<"->"<< last_sconn.postNode ()->getId () << "at(" << last_sconn.nextSend ().time () << ", "<< last_sconn.nextReceive ().time () <<")");
          }
        last_sconn = scheduler_->nextSConnection();
        return true;
      }
    else
      return false;

  }

}
#endif
