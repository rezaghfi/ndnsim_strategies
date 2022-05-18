/**
 * @author: reza gholamalitabar
 * @date: 2021-09-04 - 19:05
*/

#include "multipass-strategy.hpp"
#include "algorithm.hpp"

#include "common/global.hpp"
#include "common/logger.hpp"
#include "rib/service.hpp"

#include <ndn-cxx/lp/empty-value.hpp>
#include <ndn-cxx/lp/prefix-announcement-header.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include <boost/range/adaptor/reversed.hpp>

namespace nfd {
namespace fw {

int db_index = 0;
int db_m_index = 0;
char current_prefix = '/prefix';
int node = 0;
int counter = 0;
int alfa = 0.5;
int beta = 0.5;
// kilo bytes
int max_bw = 1000;
// micro second
int max_delay = 1000;
int min_bw;

NFD_LOG_INIT(MultiPassStrategy);
NFD_REGISTER_STRATEGY(MultiPassStrategy);

const time::milliseconds MultiPassStrategy::ROUTE_RENEW_LIFETIME(10_min);

MultiPassStrategy::MultiPassStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)   {
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

const Name&
MultiPassStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/multipass-strategy/%FD%01");
  return strategyName;
}

void
MultiPassStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                           const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
    // check db 
  for(int i=0;i < db_index;i++){
    if(db[i].prefix == current_prefix){
      counter++;
    }
  }
  bool pathDiscoveryPhase = interest.getTag<lp::PathDiscoveryPhaseTag>() != nullptr;
  auto inRecordInfo = pitEntry->getInRecord(ingress.face)->insertStrategyInfo<InRecordInfo>().first;
  // discovery path for incoming intrest
  if (pathDiscoveryPhase) { 
    inRecordInfo->pathDiscoveryPhase = true;
    if (nexthops.empty()) { // broadcast it if no matching FIB entry exists
      broadcastInterest(interest, ingress.face, pitEntry);
    }
    else { // multicast it with "discoveryPhase" mark if matching FIB entry exists
      if(counter < this->sufNumOfPath){
         interest.setTag(make_shared<lp::PathDiscoveryPhaseTag>(lp::EmptyValue{}));
      }
      //interest.setTag(make_shared<lp::PathDiscoveryPhaseTag>(lp::EmptyValue{}));
      multicastInterest(interest, ingress.face, pitEntry, nexthops);
    }
  }
  // selection phase for incoming intrest
  else { 
    inRecordInfo->pathDiscoveryPhase = false;
    if (nexthops.empty()) { // return NACK if no matching FIB entry exists
      NFD_LOG_DEBUG("NACK discoveryPhase Interest=" << interest << " from=" << ingress << " noNextHop");
      lp::NackHeader nackHeader;
      nackHeader.setReason(lp::NackReason::NO_ROUTE);
      this->sendNack(pitEntry, ingress, nackHeader);
      this->rejectPendingInterest(pitEntry);
    }
    // selection phase if matching FIB entry exists
    else { 
      // set db_m in this phase
      double nbw = 0, ndelay = 0, disjoin = 0;
      for(int i=0 ; i<db_index ; i++){
        //set nbw
        //set ndelay
        
        db[i].degree = (alfa*nbw)/(beta*ndelay);
      }
      PathStats temp;
      
      for(int i=0 ; i <= db_index ; i++){
        if(db[i].prefix == current_prefix){
          if (db[i+1].degree < db[i].degree){
          temp = db[i+1];
          db[i+1] = db[i];
          db[i] = temp;
          }
        }
      }
      //db_m_index++;
      db_m[db_m_index] = db[db_index];
      // remove db[db_index];
      //db_index--;
      
      while( db_m_index < this->multipass_num_max){
        for(int i=0 ; i<db_index ; i++){
          //set nbw
          //set ndelay
          // set disjoin
       
        db[i].deg = disjoin*(alfa*db[i].nbw)/(beta*db[i].ndelay);
        }
        PathStats temp;
        for(int i=0 ; i <= db_index ; i++){
          if(db[i].prefix == current_prefix){
            if (db[i+1].degree < db[i].degree){
            temp = db[i+1];
            db[i+1] = db[i];
            db[i] = temp;
            }
          }
        }
        db_m_index++;
        db_m[db_m_index] = db[db_index];
        // remove db[db_index];
        //db_index--;
      }
      multicastInterest(interest, ingress.face, pitEntry, nexthops);
    }
  }
}

void
MultiPassStrategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                                       const FaceEndpoint& ingress, const Data& data)
{
  int nodeOfPath = 0;
  auto outRecord = pitEntry->getOutRecord(ingress.face);
  if (outRecord == pitEntry->out_end()) {
    NFD_LOG_DEBUG("Data " << data.getName() << " from=" << ingress << " no out-record");
    return;
  }

  OutRecordInfo* outRecordInfo = outRecord->getStrategyInfo<OutRecordInfo>();
  //path discovery phase
  if (!outRecordInfo || outRecordInfo->pathDiscoveryPhase) { // outgoing Interest was discoveryPhase
    NFD_LOG_DEBUG("broadcast data is going back!!");
    //db_index++;
    this->db[db_index].min_bw = 1;
    this->db[db_index].delay = 1;  
    this->db[db_index].id = 1;
    //this->db[db_index].prefix = data.getPrefix();
    int n = nodeOfPath;
    //ndelay
    //db[db_index].avg_bw += data.getDelay()/n;
    // nbw
    db[db_index].min_bw = data.getBW();
    if(data.getBW() < min_bw){
      min_bw = data.getBW();
    }
    auto paTag = data.getTag<lp::PrefixAnnouncementTag>();
    if (paTag != nullptr) {
      addRoute(pitEntry, ingress.face, data, *paTag->get().getPrefixAnn());
    }
    else { // Data contains no PrefixAnnouncement, upstreams do not support multiPass strategy
    }
    
    sendDataToAll(pitEntry, ingress, data);
  }
  //distribution phase
  else { 
    
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
    asyncProcessData(pitEntry, ingress.face, data);
  
  }
}

void
MultiPassStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                                       const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("Nack for " << nack.getInterest() << " from=" << ingress
                << " reason=" << nack.getReason());
  if (nack.getReason() == lp::NackReason::NO_ROUTE) { // remove FIB entries
    BOOST_ASSERT(this->lookupFib(*pitEntry).hasNextHops());
    NFD_LOG_DEBUG("Send NACK to all downstreams");
    this->sendNacks(pitEntry, nack.getHeader());
    renewRoute(nack.getInterest().getName(), ingress.face.getId(), 0_ms);
  }
}

void
MultiPassStrategy::broadcastInterest(const Interest& interest, const Face& inFace,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  for (auto& outFace : this->getFaceTable() | boost::adaptors::reversed) {
    if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(inFace, interest, outFace) || outFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
      continue;
    }
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    pitEntry->getOutRecord(outFace)->insertStrategyInfo<OutRecordInfo>().first->pathDiscoveryPhase = true;
    NFD_LOG_DEBUG("send discovery Interest=" << interest << " from="
                  << inFace.getId() << " to=" << outFace.getId());
  }
}

void
MultiPassStrategy::multicastInterest(const Interest& interest, const Face& inFace,
                                        const shared_ptr<pit::Entry>& pitEntry,
                                        const fib::NextHopList& nexthops)
{
  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();
    if ((outFace.getId() == inFace.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(inFace, interest, outFace)) {
      continue;
    }
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    pitEntry->getOutRecord(outFace)->insertStrategyInfo<OutRecordInfo>().first->pathDiscoveryPhase = true;
    NFD_LOG_DEBUG("send interest in discovery phase =" << interest << " from="
                  << inFace.getId() << " to=" << outFace.getId());
  }
}

void
MultiPassStrategy::asyncProcessData(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data)
{
  // Given that this processing is asynchronous, the PIT entry's expiry timer is extended first
  // to ensure that the entry will not be removed before the whole processing is finished
  // (the PIT entry's expiry timer was set to 0 before dispatching)
  this->setExpiryTimer(pitEntry, 1_s);

  runOnRibIoService([pitEntryWeak = weak_ptr<pit::Entry>{pitEntry}, inFaceId = inFace.getId(), data, this] {
    rib::Service::get().getRibManager().slFindAnn(data.getName(),
      [pitEntryWeak, inFaceId, data, this] (optional<ndn::PrefixAnnouncement> paOpt) {
        if (paOpt) {
          runOnMainIoService([pitEntryWeak, inFaceId, data, pa = std::move(*paOpt), this] {
            auto pitEntry = pitEntryWeak.lock();
            auto inFace = this->getFace(inFaceId);
            if (pitEntry && inFace) {
              NFD_LOG_DEBUG("found PrefixAnnouncement=" << pa.getAnnouncedName());
              data.setTag(make_shared<lp::PrefixAnnouncementTag>(lp::PrefixAnnouncementHeader(pa)));
              this->sendDataToAll(pitEntry, FaceEndpoint(*inFace, 0), data);
              this->setExpiryTimer(pitEntry, 0_ms);
            }
            else {
              NFD_LOG_DEBUG("PIT entry or Face no longer exists");
            }
          });
        }
    });
  });
}

bool
MultiPassStrategy::needPrefixAnn(const shared_ptr<pit::Entry>& pitEntry)
{
  bool hasDiscoveryInterest = false;
  bool directToConsumer = true;

  auto now = time::steady_clock::now();
  for (const auto& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getExpiry() > now) {
      InRecordInfo* inRecordInfo = inRecord.getStrategyInfo<InRecordInfo>();
      if (inRecordInfo && inRecordInfo->pathDiscoveryPhase) {
        hasDiscoveryInterest = true;
      }
      if (inRecord.getFace().getScope() != ndn::nfd::FACE_SCOPE_LOCAL) {
        directToConsumer = false;
    }
    }
  }
  return hasDiscoveryInterest && !directToConsumer;
}

void
MultiPassStrategy::addRoute(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace,
                               const Data& data, const ndn::PrefixAnnouncement& pa)
{
  runOnRibIoService([pitEntryWeak = weak_ptr<pit::Entry>{pitEntry}, inFaceId = inFace.getId(), data, pa] {
    rib::Service::get().getRibManager().slAnnounce(pa, inFaceId, ROUTE_RENEW_LIFETIME,
      [] (RibManager::SlAnnounceResult res) {
        NFD_LOG_DEBUG("Add route via PrefixAnnouncement with result=" << res);
      });
  });
}

void
MultiPassStrategy::renewRoute(const Name& name, FaceId inFaceId, time::milliseconds maxLifetime)
{
  // renew route with PA or ignore PA (if route has no PA)
  runOnRibIoService([name, inFaceId, maxLifetime] {
    rib::Service::get().getRibManager().slRenew(name, inFaceId, maxLifetime,
      [] (RibManager::SlAnnounceResult res) {
        NFD_LOG_DEBUG("Renew route with result=" << res);
      });
  });
}

} // namespace fw
} // namespace nfd
