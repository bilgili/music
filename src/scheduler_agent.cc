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

namespace MUSIC
{
  SchedulerAgent::SchedulerAgent (Scheduler *scheduler) :
      scheduler_ (scheduler)
  {

  }


  MulticommAgent::MulticommAgent (Scheduler *scheduler) :
      SchedulerAgent (scheduler)
  {

  }


  MulticommAgent::Filter1::Filter1 (MulticommAgent &multCommObj) :
      multCommObj_ (multCommObj)
  {

  }


  /*    remedius
   *  This filter is applied to the set of selected connectors in the fillSchedule().
   *  It partitioned the set of connectors into two groups:
   *  returns True: connectors that can be lumped:if scheduled send times <= multCommObj_.time_.
   *  This condition is sufficient because each conflicting send time can be postponed to the most latter scheduled for the same node.
   *  The most latter possible time is a multCommObj_.time_ (the receive time).
   *  returns False: connectors that go to the Filter2 (connectors that possibly can be added to the current group)
   */
  bool
  MulticommAgent::Filter1::operator() (SConnectionP &conn)
  {
    if (conn.second.sSend <= multCommObj_.time_)
      {
        int sId = conn.first.pre ().data ().color;
        int rId = conn.first.post ().data ().color;
        //keeps track of all receive nodes
        multCommObj_.rNodes |= 1 << rId;
        //guarantees that if there are both send and receive are scheduled to the same node,
        // the receive time will be chosen.
        multCommObj_.commTimes[rId] = multCommObj_.time_;
        // take the most later time
        if (conn.second.sSend >= multCommObj_.commTimes[sId])
          multCommObj_.commTimes[sId] = conn.second.sSend;
        return true;
      }
    return false;
  }


  MulticommAgent::Filter2::Filter2 (MulticommAgent &multCommObj) :
      multCommObj_ (multCommObj)
  {

  }


  /*    remedius
   * This filter is applied to the rest of the selected connectors, that were classified as False by the Filter1.
   * It adds those of the rest connectors(which scheduled send times > multCommObj_.time_.) that either don't have a conflict
   * with the already selected connectors by Filter1 or if the conflict is solvable.
   * The conflict is not solvable if both send and receive are scheduled for the same node.
   */
  bool
  MulticommAgent::Filter2::operator() (SConnectionP &conn)
  {
    int sId = conn.first.pre ().data ().color;
    int rId = conn.first.post ().data ().color;
    std::map<int, Clock>::iterator it;
    it = multCommObj_.commTimes.find (sId);
    if (it == multCommObj_.commTimes.end ()
        || ! (multCommObj_.rNodes & (1 << sId)))
      {
        multCommObj_.commTimes[sId] = conn.second.sSend;
        multCommObj_.commTimes[rId] = multCommObj_.time_;
        multCommObj_.rNodes |= 1 << rId;
        return true;
      }
    return false;
  }


  MulticommAgent::~MulticommAgent ()
  {
    for (std::vector<MultiConnector*>::iterator connector =
        multiConnectors.begin (); connector != multiConnectors.end ();
        ++connector)
      if (*connector != NULL)
        delete *connector;
    delete multiBuffer_;
    delete filter1;
    delete filter2;
  }


  void
  MulticommAgent::initialize ()
  {
    filter1 = new Filter1 (*this);
    filter2 = new Filter2 (*this);
  }


  void
  MulticommAgent::createMultiConnectors (Clock& localTime, MPI::Intracomm comm,
      int leader, std::vector<Connector*>& connectors)
  {
    multiBuffer_ = new MultiBuffer (comm, leader, connectors);
    multiConnectors.resize (Connector::idRange ());
    multiProxies = new std::vector<bool>;
    multiProxies->resize (Connector::proxyIdRange ());

    for (unsigned int self_node = 0; self_node < scheduler_->nApplications ();
        ++self_node)
      {

        scheduler_->reset (self_node);

        if (!create (localTime))
          scheduler_->pushForward ();

        localTime.ticks (-1);

        for (int i = 0; i < N_PLANNING_CYCLES; ++i)
          {
            localTime.tick ();
            if (!create (localTime))
              scheduler_->pushForward ();

          }

        //finalize
        schedule.clear ();

        localTime.reset ();
      }

    scheduler_->reset (-1);
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
  MulticommAgent::create (MUSIC::Clock &localTime)
  {
    std::vector<MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
        for (comm = schedule.begin ();
            comm != schedule.end () && (*comm).time <= localTime.time ();
            ++comm)
          {
            unsigned int multiId = (*comm).multiId ();
            if (multiId != 0)
              {
                if (multiConnectors[multiId] == NULL)
                  {
                    std::vector<Connector*> connectors = connectorsFromMultiId (
                        multiId);
                    multiConnectors[multiId] = new MultiConnector (multiBuffer_,
                        connectors);
                  }
              }
            else
              {
                unsigned int proxyId = (*comm).proxyId ();
                if (proxyId != 0 && ! (*multiProxies)[proxyId])
                  {
                    //  std::cout << "Rank " << MPI::COMM_WORLD.Get_rank ()
                    //      << ": Proxy " << proxyId << std::endl;
                    MPI::COMM_WORLD.Create (MPI::GROUP_EMPTY);
                    MPI::COMM_WORLD.Barrier ();
                    (*multiProxies)[proxyId] = true;
                  }
              }
          }
        schedule.erase (schedule.begin (), comm);
      }
    while (continue_ && schedule.empty ());
    return !schedule.empty ();
  }


  bool
  MulticommAgent::tick (MUSIC::Clock &localTime)
  {

    std::vector<MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
        for (comm = schedule.begin ();
            comm != schedule.end () && (*comm).time <= localTime.time ();
            ++comm)
          {

            unsigned int multiId = (*comm).multiId ();
            if (multiId != 0)
              {
                assert (multiConnectors[multiId] != NULL);

                multiConnectors[multiId]->tick ();

              }
          }
        schedule.erase (schedule.begin (), comm);
        // we continue looping while:
        // 1. we haven't found the communication from the future:   !schedule.empty()
        // 2. or the next SConnection can't be processed by the current agent: continue_= false
      }
    while (continue_ && schedule.empty ());
    return !schedule.empty ();
  }


  bool
  MulticommAgent::fillSchedule ()
  {
    if (!schedule.empty ())
      return true;
    Scheduler::SConnection last_sconn = scheduler_->getLastSConnection ();
    if (!last_sconn.data ().connector->idFlag ())
      return false;
    Clock nextReceive = last_sconn.data ().nextReceive; //last connection
    SConnectionPV nextSConnections;
    do
      {
        if (last_sconn.data ().isLoopConnected
            || scheduler_->isLocalConnection (last_sconn))
          nextSConnections.push_back (
              std::make_pair (last_sconn, last_sconn.data ()));

        last_sconn = scheduler_->pushForward ();
      }
    while (last_sconn.data ().connector->idFlag () //those that request multicommunication
    && (last_sconn.data ().nextReceive == nextReceive)); //those that have scheduled receive at current time
    //  || (!last_sconn.data ().isLoopConnected //
    //  && !scheduler_->isLocalConnection (last_sconn))));

    if (nextSConnections.size () > 0)
      NextMultiConnection (nextSConnections);

    return last_sconn.data ().connector->idFlag ();
  }


  void
  MulticommAgent::NextMultiConnection (SConnectionPV &candidates)
  {


    time_ = candidates[0].second.nextReceive;
    // std::map<int, Clock> prevCommTime;
    SConnectionPV::iterator iter_bound = candidates.begin ();
    SConnectionPV::iterator cur_bound = candidates.begin ();
    do
      {
        cur_bound = iter_bound;

        commTimes.clear ();
        rNodes = 0;

        // after the first iteration inner partitioning (Filter1) will always return .begin()
        iter_bound = std::stable_partition (
            std::stable_partition (cur_bound, candidates.end (), *filter1),
            candidates.end (), *filter2);

        //postpone sends in the multiconn for consistency
        for (SConnectionPV::iterator it = cur_bound; it != iter_bound; ++it)
          (*it).second.postponeNextSend (
              commTimes[ (*it).first.pre ().data ().color]);

        std::map<int, Clock>::iterator id_iter;
        if ( (id_iter = commTimes.find (scheduler_->self_node ()))
            != commTimes.end ())
          scheduleMulticonn ( (*id_iter).second, cur_bound, iter_bound);
      }
    while (iter_bound != candidates.end ());
  }


  void
  MulticommAgent::scheduleMulticonn (Clock &time, SConnectionPV::iterator first,
      SConnectionPV::iterator last)
  {
    unsigned int multiId = 0;
    unsigned int proxyId = 0;
    MUSIC_LOGR ("SelfNode: " << scheduler_->self_node() << " Time: "<< time.time() << std::endl << "MultiConnector:");
    for (SConnectionPV::iterator it = first; it != last;
        ++it)
      {
        if ( (*it).second.connector->isProxy ())
          proxyId |= (*it).second.connector->idFlag ();
        else
          multiId |= (*it).second.connector->idFlag ();
        MUSIC_LOGR ("("<< (*it).preNode ()->getId () <<" -> "<< (*it).postNode ()->getId () << ") at " << (*it).nextSend().time () << " -> "<< (*it).nextReceive ().time ());
      }
    schedule.push_back (MultiCommObject (time.time (), multiId, proxyId));
  }


  void
  MulticommAgent::preFinalize (std::set<int> &cnn_ports)
  {
    for (std::vector<MultiConnector*>::iterator m = multiConnectors.begin ();
        m != multiConnectors.end (); ++m)
      if (*m != NULL)
        cnn_ports.insert ( (*m)->connectorCode ());
  }


  void
  MulticommAgent::finalize (std::set<int> &cnn_ports)
  {

    std::vector<MultiCommObject>::iterator comm;
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
        for (comm = schedule.begin ();
            comm != schedule.end () && !cnn_ports.empty (); ++comm)
          {
            unsigned int multiId = (*comm).multiId ();
            if (multiId != 0)
              {
                MultiConnector* m = multiConnectors[multiId];
                if (cnn_ports.find (m->connectorCode ()) == cnn_ports.end ())
                  continue;
                if (m->isFinalized ())
                // an output port was finalized in tick ()
                  {
                    cnn_ports.erase (m->connectorCode ());
                    // Sometimes the scheduler fails to generate all
                    // multiConnectors during finalization.  Here, we
                    // weed out multiConnectors that were finalized as
                    // a consequence of the finalization of m.
                    for (std::vector<MultiConnector*>::iterator m =
                        multiConnectors.begin (); m != multiConnectors.end ();
                        ++m)
                      if (*m != NULL
                          && (cnn_ports.find ( (*m)->connectorCode ())
                              != cnn_ports.end ()) && (*m)->isFinalized ())
                        cnn_ports.erase ( (*m)->connectorCode ());
                    continue;
                  }
                // finalize () needs to come after isFinalized check
                // since it can itself set finalized state
                m->finalize ();
                m->tick ();
              }
          }
        schedule.clear ();
      }
    while (continue_ && !cnn_ports.empty ());
  }


  UnicommAgent::UnicommAgent (Scheduler *scheduler) :
      SchedulerAgent (scheduler)
  {
  }


  bool
  UnicommAgent::fillSchedule ()
  {
    if (!schedule.empty ())
      return true;
    Scheduler::SConnection last_sconn = scheduler_->getLastSConnection ();
    if (last_sconn.data ().connector->idFlag ())
      return false;
    if (scheduler_->isLocalConnection (last_sconn))
      {
        double nextComm = (
            scheduler_->self_node () == last_sconn.post ().data ().color ?
                last_sconn.data ().nextReceive.time () :
                last_sconn.data ().nextSend.time ());
        schedule.time = nextComm;
        schedule.connector = last_sconn.data ().connector;
        MUSIC_LOGR (" :Scheduled communication:"<< last_sconn.preNode ()->getId () <<"->"
            << last_sconn.postNode ()->getId ()<< "at("
            << last_sconn.nextSend ().time () << ", "
            << last_sconn.nextReceive ().time () <<")");

      }
    scheduler_->pushForward ();
    return true;
  }


  bool
  UnicommAgent::tick (Clock& localTime)
  {
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
        if (!schedule.empty () && schedule.time <= localTime.time ())
          {
            schedule.connector->tick ();
            schedule.reset ();
          }
      }
    while (continue_ && schedule.empty ());
    return !schedule.empty ();
  }


  void
  UnicommAgent::finalize (std::set<int> &cnn_ports)
  {
    bool continue_;
    do
      {
        continue_ = fillSchedule ();
        if (!schedule.empty ())
          {
            Connector* connector = schedule.connector;
            schedule.reset ();
            if ( (cnn_ports.find (connector->receiverPortCode ())
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
    while (continue_ && !cnn_ports.empty ());
  }

}
#endif
