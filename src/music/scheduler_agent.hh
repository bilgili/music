#ifndef MUSIC_SCHEDULER_AGENT_HH

#include "music/music-config.hh"
#include "music/scheduler.hh"
#if MUSIC_USE_MPI

namespace MUSIC {
  class Scheduler;
  typedef  std::vector< Scheduler::SConnection> SConnectionV;
  class SchedulerAgent
  {
  protected:
    Scheduler *scheduler_;
    SchedulerAgent(Scheduler *scheduler);

  public:
    virtual ~SchedulerAgent(){};
    virtual void initialize()=0;
    virtual bool fillSchedule( std::vector<std::pair<double, Connector *> > &schedule,Scheduler::SConnection &last_sconn) = 0;
  };

  class MulticommAgent: public virtual SchedulerAgent
  {
    std::map<int, Clock> commTimes;
    int rNodes;
    Clock time_;


    class Filter1
    {
      MulticommAgent &multCommObj_;
    public:
      Filter1(MulticommAgent &multCommObj);
      bool operator()(const Scheduler::SConnection &conn);
    };
    class Filter2
    {
      MulticommAgent &multCommObj_;
    public:
      Filter2(MulticommAgent &multCommObj);
      bool operator()(const Scheduler::SConnection &conn);
    };
  public:
    MulticommAgent(Scheduler *scheduler);
    ~MulticommAgent();
    void initialize();
    bool fillSchedule(std::vector<std::pair<double, Connector *> > &schedule, Scheduler::SConnection &last_sconn);
  private:
    void fillSchedule( std::vector<std::pair<double, Connector *> > &schedule,SConnectionV &candidates);
    SConnectionV::iterator  NextMultiConnection(SConnectionV::iterator &last_bound,
        SConnectionV::iterator last,
        std::map<int, Clock> &prevCommTime);
    void printMulticonn(Clock time,SConnectionV::iterator first, SConnectionV::iterator last);

    friend class Filter1;
    friend class Filter2;
    Filter1 *filter1;
    Filter2 *filter2;
  };
  class UnicommAgent: public virtual SchedulerAgent
  {
  public:
    UnicommAgent(Scheduler *scheduler);
    ~UnicommAgent(){};
    void initialize(){};
    bool fillSchedule(std::vector<std::pair<double, Connector *> > &schedule, Scheduler::SConnection &last_sconn);
  };
}
#endif

#define MUSIC_SCHEDULER_AGENT_HH
#endif
