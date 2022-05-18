// abdi ndnsim forwarding strategy source file for multipassing article

#include "multipass-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"

namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY(MultiPassStrategy);
NFD_LOG_INIT(MultiPassStrategy);
const time::milliseconds MultiPassStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds MultiPassStrategy::RETX_SUPPRESSION_MAX(250);

int db_index = 0;
int db_m_index = 0;
char current_prefix = 'c';
int node = 0;
int counter = 0;
int alfa = 0.5;
int beta = 0.5;
// kilo bytes
int max_bw = 1000;
// micro second
int max_delay = 1000;
int min_bw;

MultiPassStrategy::MultiPassStrategy(Forwarder& forwarder, const Name& name) : Strategy(forwarder), ProcessNackTraits(this) , m_retxSuppression(RETX_SUPPRESSION_INITIAL, RetxSuppressionExponential::DEFAULT_MULTIPLIER, RETX_SUPPRESSION_MAX){
          
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("MultiPassStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "MultiPassStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
  this->multipass_num_max = 4;
  this->sufNumOfPath = 5;
  this->period_count = 100;
}

const Name& MultiPassStrategy::getStrategyName(){
  static Name strategyName("/localhost/nfd/strategy/multipass-strategy/%FD%03");
  return strategyName;
}

void MultiPassStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry){
  
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  // check db 
  for(int i=0;i < db_index;i++){
    if(db[i].prefix == current_prefix){
      counter++;
    }
  }

  isPathDiscoveryPhase = (counter < this->sufNumOfPath);
  
  // path discovery phase
  if(isPathDiscoveryPhase){
    
    int nEligibleNextHops = 0;
    bool isSuppressed = false;

    for (const auto& nexthop : nexthops) {
      Face& outFace = nexthop.getFace();
      RetxSuppressionResult suppressResult = m_retxSuppression.decidePerUpstream(*pitEntry, outFace);

      if (suppressResult == RetxSuppressionResult::SUPPRESS) {
        NFD_LOG_DEBUG(interest << " from=" << ingress << " to=" << outFace.getId() << " suppressed");
        isSuppressed = true;
        continue;
      }

      if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
          wouldViolateScope(ingress.face, interest, outFace)) {
        continue;
      }
      //!! به درخواست فیلد آیدی نود مسیر را اضافه می کنیم
      //shared_ptr<interest> interest2 = make_shared<interest>();
      //interest2 = &interest;
      //interest2->setNodes(node);
      this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
      NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());

      if (suppressResult == RetxSuppressionResult::FORWARD) {
        m_retxSuppression.incrementIntervalForOutRecord(*pitEntry->getOutRecord(outFace));
      }
      ++nEligibleNextHops;
    }

    if (nEligibleNextHops == 0 && !isSuppressed) {
      NFD_LOG_DEBUG(interest << " from=" << ingress << " noNextHop");

      lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, ingress, nackHeader);
      this->rejectPendingInterest(pitEntry);
    }
  }
  // selection path phase
  if(!isPathDiscoveryPhase){
    double nbw, ndelay, disjoin;
    for(int i=0 ; i<db_index ; i++){
      //set nbw
      //set ndelay
      //!! این قسمت کد باید پهنای باند و تاخیر کل مسیر چگونه بدست می آید؟
      db[i].degree = (alfa*nbw)/(beta*ndelay);
    }
    PathStats temp;
    // !! مرتب سازی باید براساس prefix انجام شود.
    for(int i=0 ; i <= db_index ; i++){
      if (db[i+1].degree < db[i].degree){
        temp = db[i+1];
        db[i+1] = db[i];
        db[i] = temp;
      }
    }
    db_m_index++;
    db_m[db_m_index] = db[db_index];
    // remove db[0];
    db_index--;
    
    while( db_m_index < this->multipass_num_max){
      for(int i=0 ; i<db_index ; i++){
        //set nbw
        //set ndelay
        // set disjoin
      //!! این قسمت کد باید پهنای باند و تاخیر کل مسیر چگونه بدست می آید؟
      db[i].deg = disjoin*(alfa*nbw)/(beta*ndelay);

      }
      PathStats temp;
       // !! مرتب سازی باید براساس prefix انجام شود.
      for(int i=0 ; i <= db_index ; i++){
        if (db[i+1].deg < db[i].deg){
          temp = db[i+1];
          db[i+1] = db[i];
          db[i] = temp;
          
        }
      }
      db_m_index++;
      db_m[db_m_index] = db[db_index];
      // remove db[0];
      db_index--;
    }
    // ارسال به بهترین فیس در لیست
    //const auto& out_face;
    //Face& outFace = out_face.getFace();
    // this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
  }

}

void MultiPassStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack, const shared_ptr<pit::Entry>& pitEntry){

  this->processNack(ingress.face, nack, pitEntry);
}


void MultiPassStrategy::afterReceiveData (const shared_ptr< pit::Entry > &pitEntry, const FaceEndpoint& ingress, const Data &data){

  NFD_LOG_DEBUG("afterReceiveData pitEntry=" << pitEntry->getName()
                << " in=" << ingress << " data=" << data.getName());
  if(isPathDiscoveryPhase){
    // path discovery phase
      NFD_LOG_DEBUG("broadcast data is going back!!");
      //db_index++;
      this->db[db_index].min_bw = 1;
      this->db[db_index].delay = 1;  
      this->db[db_index].id = 1;
      this->db[db_index].prefix = 'p';
      // nbw

      db[db_index].min_bw = data.getBW();

      if(data.getBW() < min_bw){
        min_bw = data.getBW();
      }
      

      this->sendDataToAll(pitEntry, ingress, data);
  }
  // data distrbution phase
  if(!isPathDiscoveryPhase){
    double deg_sum = 0;
    // !!‌ اضافه کردن پارامتر کلاس
    for(int i=0; i <= db_m_index; i++){
        deg_sum += db_m[i].deg;
    }
    for(int i=0; i <= db_m_index; i++){
        db_m[i].qouta = db_m[i].deg / deg_sum;
    }
    this->beforeSatisfyInterest(pitEntry, ingress, data);

    // !! نحوه توزیع داده 
    int i = 0;
    if(i==0){
      //sendData(pitEntry, data, db_m[i].outFace);
      sendData(pitEntry, data, ingress);
    }
    if (i==1){
      sendData(pitEntry, data, ingress);
    }
    if(i==2){
      sendData(pitEntry, data, ingress);
    }
    if(i==3){
      sendData(pitEntry, data, ingress);
    }  
    if(this->period_count == 100){
        // 
      double score_sum = 0;
      // !!‌ اضافه کردن پارامتر کلاس
      for(int i=0; i <= db_m_index; i++){
        // cal Nbw
        // cal NT
        // cal Ndelay

        db_m[i].score = 0;
        score_sum += db_m[i].score;
      }
      for(int i=0; i <= db_m_index; i++){
          db_m[i].score = db_m[i].score / score_sum;
      }
    }
    this->period_count++;
  }
}

} // namespace fw
} // namespace nfd
