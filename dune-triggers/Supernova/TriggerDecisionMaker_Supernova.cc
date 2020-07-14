#include "TriggerDecisionMaker_Supernova.hh"

#include <algorithm>
#include <limits>
#include <chrono>

using namespace DuneTriggers;
using pd_clock = std::chrono::duration<double, std::ratio<1, 50000000>>;

void TriggerDecisionMakerSupernova::operator()(const TriggerCandidate& cand,
                                               std::vector<TriggerDecision>& decisions) {
  int64_t time = cand.time_start;
  FlushOldCandidate(time); // get rid of old clusters in the buffer
  m_times_cluster.push_back(time);

  // Yay! we have a trigger!
  if (m_times_cluster.size() > m_threshold) {
    std::chrono::time_point<std::chrono::steady_clock> now;

    // set all bits to one (probably this will mean down the line: "record every part of the detector")
    uint32_t detid = 0xFFFFFFFF;
    
    TriggerDecision trigger {time - 500000000, // time_start (10 seconds before the start of the cluster)
                             cand.time_end, // time_end, but that should probably be _at least_ this number
                             int64_t(pd_clock(now.time_since_epoch()).count()), // this is now in dune time, with a cast to avoid narrowing warning
                             detid, // all the detector
                             0}; // we'll probably find that having 32 extra bytes can be useful some time.

    // Give the trigger word back
    decisions.push_back(trigger);
    return;
  }
  
}
