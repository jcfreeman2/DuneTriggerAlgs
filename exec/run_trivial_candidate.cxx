#include <chrono>
#include <random>
#include <iostream>
#include <thread>

#include <boost/circular_buffer.hpp>

#include "trivial/TriggerCandidateMaker_trivial.hh"
#include "NaiveTriggerQueue.h"
#include "NaiveTriggerCandidateConsumer.h"


using pd_clock = std::chrono::duration<double, std::ratio<1, 50000000>>;

std::default_random_engine generator;

std::uniform_int_distribution<double>  rdm_channel(0, 2560);
std::normal_distribution<double>       rdm_adc(20, 5);
std::normal_distribution<double>       rdm_time_over_threshold(100, 20); // nanosec
std::normal_distribution<double>       rdm_start_time(0, 20); // nanosec
std::uniform_real_distribution<double> rdm_peak_time(0,1);

auto time_ref = std::chrono::steady_clock::now(); // a time reference such that numbers aren't so big (start of DUNE time?)


TriggerPrimitive GetRandomTP(std::chrono::time_point<std::chrono::steady_clock>& now) {
  TriggerPrimitive tp;
  
  double start = rdm_start_time(generator);
  double tot   = rdm_time_over_threshold(generator);
  double peak  = rdm_peak_time(generator)*tot;

  std::chrono::nanoseconds tot_time((int)tot);
  std::chrono::nanoseconds peak_time((int)peak);
  std::chrono::nanoseconds start_time((int)start);

  auto tp_start_time = now-time_ref+start_time;
  
  tp.time_start          = pd_clock(tp_start_time).count();
  tp.time_over_threshold = pd_clock(tot_time).count();
  tp.time_peak           = pd_clock(tp_start_time+peak_time).count();
  tp.channel             = rdm_channel(generator);
  tp.adc_integral        = rdm_adc(generator);
  tp.adc_peak            = rdm_adc(generator);
  tp.detid               = tp.channel;
  
  return tp;
}



int main() {
  generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
  int n_second_generating = 2;
  // millisec or kHz
  std::exponential_distribution<double> tp_rate(3);
  std::normal_distribution<int> n_tp(1,0);
  std::normal_distribution<double> electronic_delay(0, 0);
  
  TriggerCandidateMakerTrivial tcmt;
  NaiveTriggerQueue tq(tcmt);
  NaiveTriggerCandidateConsumer tcc(tq);
  
  tq.Start();
  tcc.Start();  
  std::thread worker_thread(&NaiveTriggerQueue::Worker, &tq);
  std::thread consumer_thread(&NaiveTriggerCandidateConsumer::Worker, &tcc);
  int n_tps_total=0;
  int n_cluster=0;
    
  std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
  while(true) {
    auto now = std::chrono::steady_clock::now();
    int n = n_tp(generator);
    n_tps_total+=n;
    std::vector <TriggerPrimitive> tps;
    n_cluster += n>0;
    
    for (int i=0; i<n; ++i) {
      tps.push_back(GetRandomTP(now));
    }
    
    uint64_t wait = electronic_delay(generator);
    std::chrono::milliseconds millisec_wait(wait);
    std::this_thread::sleep_for(millisec_wait);

    tq.AddToQueue(tps);
    
    wait = tp_rate(generator);
    millisec_wait = std::chrono::milliseconds(wait);
    std::this_thread::sleep_for(millisec_wait);
    
    if (now-start_time > std::chrono::seconds(n_second_generating)){
      break;
    }
  }
  std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
  
  std::cout << "TPs created: " << n_tps_total << "\n";
  std::cout << "\"True\" cluster created: " << n_cluster << "\n";
  auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  std::cout << "TP rate: " << n_tps_total / (double)dt.count() << " MHz\n";

  tq.SoftStop();
  tcc.Stop();
  
  worker_thread.join();
  consumer_thread.join();

}