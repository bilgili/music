#ifndef MUSIC_SCHEDULER_AGENT_HH

#include "music/music-config.hh"
#include "music/scheduler.hh"
#include "music/multibuffer.hh"
#if MUSIC_USE_MPI

namespace MUSIC {
  class Scheduler;
  typedef  std::vector< Scheduler::SConnection> SConnectionV;
  class SchedulerAgent
  {
  protected:
    Scheduler *scheduler_;
    SchedulerAgent(Scheduler *scheduler);
    class CommObject
    {
    public:
      CommObject():time(-1.0){};
      CommObject(double time_):time(time_){};
      virtual ~CommObject(){}
      double time;
      void reset(){time = -1.0;}
      bool empty(){return time < 0;}
    };
    virtual bool fillSchedule() = 0;
  public:
    virtual ~SchedulerAgent(){};
    virtual void initialize()=0;
    virtual bool tick(Clock& localTime)=0;
    virtual void finalize (std::set<int> &cnn_ports) = 0;
  };

  class MulticommAgent: public virtual SchedulerAgent
  {
    std::map<int, Clock> commTimes;
    int rNodes;
    Clock time_;
    MultiBuffer* multiBuffer_;
    std::vector<MultiConnector*> multiConnectors;

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

    class MultiCommObject: public CommObject
    {
    public:
      unsigned int connector;
      MultiCommObject(double time_, unsigned int connector_):CommObject(time_),connector(connector_){};
    };

    std::vector<MultiCommObject> schedule;
  public:
    MulticommAgent(Scheduler *scheduler);
    ~MulticommAgent();
    void initialize();
    void createMultiConnectors (Clock& localTime,
                                  MPI::Intracomm comm,
                                  int leader,
                                  std::vector<Connector*>& connectors);
    bool create (Clock& localTime);
    bool tick(Clock& localTime);
    void finalize (std::set<int> &cnn_ports);

  private:
    std::vector<bool>* multiProxies;    
    std::vector<Connector*> connectorsFromMultiId (unsigned int multiId);
    bool fillSchedule();
    void fillSchedule( SConnectionV &candidates);
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
    class UniCommObject: public CommObject
    {
    public:
      Connector *connector;
      UniCommObject():CommObject(){};
      UniCommObject(double time_, Connector *connector_):CommObject(time_),connector(connector_){};
    };
    UniCommObject schedule;
    bool fillSchedule();
  public:
    UnicommAgent(Scheduler *scheduler);
    ~UnicommAgent(){};
    void initialize(){};
    bool tick(Clock& localTime);
    void finalize (std::set<int> &cnn_ports);
  };
}
#endif

#define MUSIC_SCHEDULER_AGENT_HH
#endif
