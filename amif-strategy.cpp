/**
 * @author: reza gholamalitabar
 * @date: 12 tir 1400 10:02
*/
#include "amif-strategy.hpp"
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
    string current_prefix = "/prefix";
    int counter = 0;
    int alfa = 0.5;
    int beta = 0.5;
    // kilo bytes
    int max_bw = 1000;
    // micro second
    int max_delay = 1000;
    int min_bw;
    nbw = 0, ndelay = 0, nthroughput;

    NFD_LOG_INIT(AMIFStrategy);
    NFD_REGISTER_STRATEGY(AMIFStrategy);

    // const time::milliseconds AMIFStrategy::ROUTE_RENEW_LIFETIME(10_min);

    AMIFStrategy::AMIFStrategy(Forwarder& forwarder, const Name& name) : Strategy(forwarder) {
      ParsedInstanceName parsed = parseInstanceName(name);
      if (!parsed.parameters.empty()) {
        NDN_THROW(std::invalid_argument("AMIFStrategy does not accept parameters"));
      }
      if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
        NDN_THROW(std::invalid_argument(
          "AMIFStrategy does not support version " + to_string(*parsed.version)));
      }
      this->setInstanceName(makeInstanceName(name, getStrategyName()));
      this->multipass_num_max = 4;
      this->sufNumOfPath = 5;
      this->period_count = 0;
      this->pathToSendDataRound = 0;
    }

    const Name& AMIFStrategy::getStrategyName() {
      static Name strategyName("/localhost/nfd/strategy/amif/%FD%01");
      return strategyName;
    }
    double getdisjointness(int i, PathStats db_m[10]) {
      double shared = (1 / (1 + shared_node(db[i], db_m[j]));
      double getdisjointness = 0;
      for (int j = 0; j < db_m_index; j++) {
        getdisjointness += shared;
      }
    }
    return getdisjointness;
  }
  //------------------------------------------- AFTER -- RECEIVE -- INTEREST -------------------------------------
  void AMIFStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry) {
    const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
    const fib::NextHopList& nexthops = fibEntry.getNextHops();
    bool pathDiscoveryPhase = interest.getTag<lp::PathDiscoveryPhaseTag>() != nullptr;
    auto inRecordInfo = pitEntry->getInRecord(ingress.face)->insertStrategyInfo<InRecordInfo>().first;
    //!! check db 
    for (int i = 0;i < db_index;i++) {
      if (db[i].prefix == current_prefix) {
        counter++;
      }
      if (counter >= this->sufNumOfPath) {
        pathDiscoveryPhase = false;
        break;
      }
    }
    if (pathDiscoveryPhase) {
      //!! Discovery Path Phase for incoming intrest
      inRecordInfo->pathDiscoveryPhase = true;
      interest.setTag(make_shared<lp::PathDiscoveryPhaseTag>(lp::EmptyValue{}));
      if (nexthops.empty()) {
        // broadcast it if no matching FIB entry exists
        broadcastInterest(interest, ingress.face, pitEntry);
      }
      else {
        // multicast it with "discoveryPhase" mark if matching FIB entry exists
        multicastInterest(interest, ingress.face, pitEntry, nexthops);
      }
    }
    else {
      //!! Selection Path Phase for incoming intrest
      inRecordInfo->pathDiscoveryPhase = false;
      if (nexthops.empty()) { // return NACK if no matching FIB entry exists
        NFD_LOG_DEBUG("NACK discoveryPhase Interest=" << interest << " from=" << ingress << " noNextHop");
        lp::NackHeader nackHeader;
        nackHeader.setReason(lp::NackReason::NO_ROUTE);
        this->sendNack(pitEntry, ingress, nackHeader);
        this->rejectPendingInterest(pitEntry);
      }
      else {
        // selection phase if matching FIB entry exists
        // set db_m in this phase
        double disjoin = 1;
        for (int i = 0; i < db_index; i++) {
          if (current_prefix != db[i].prefix) {
            continue;
          }
          //ndelay
          ndelay = interest.getDelay() / max_delay;
          // nbw
          min_bw = interest.getBW();
          if (interest.getBW() < min_bw) {
            min_bw = interest.getBW();
          }
          nbw = min_bw / max_bw;
          db[i].degree = (alfa * nbw) / (beta * ndelay);
        }
        // sort db with degree
        PathStats temp;
        for (int i = 0; i <= db_index; i++) {
          if (db[i].prefix == current_prefix) {
            if (db[i + 1].degree < db[i].degree || db[i + 1].prefix != current_prefix) {
              temp = db[i + 1];
              db[i + 1] = db[i];
              db[i] = temp;
            }
          }
        }
        db_m_index++;
        db_m[db_m_index] = db[db_index];
        // remove db[db_index];
        db_index--;
        int prefixCount = 0;
        while (prefixCount < this->multipass_num_max) {
          for (int i = 0; i < db_index; i++) {
            if (current_prefix != db[i].prefix) {
              continue;
            }
            //ndelay
            ndelay = interest.getDelay() / max_delay;
            // nbw
            min_bw = interest.getBW();
            if (interest.getBW() < min_bw) {
              min_bw = interest.getBW();
            }
            nbw = min_bw / max_bw;
            // set disjoin
            disjoin = getdisjointness(i, db_m);
            db[i].deg = disjoin * (alfa * db[i].nbw) / (beta * db[i].ndelay);
          }
          // sort db with deg
          PathStats temp;
          for (int i = 0; i <= db_index; i++) {
            if (db[i + 1].deg < db[i].deg || db[i + 1].prefix != current_prefix) {
              temp = db[i + 1];
              db[i + 1] = db[i];
              db[i] = temp;
            }
          }
          prefixCount++;
          db_m_index++;
          db_m[db_m_index] = db[db_index];
          // remove db[db_index];
          db_index--;
        }
      }
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

        NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());

        if (suppressResult == RetxSuppressionResult::FORWARD) {
          m_retxSuppression.incrementIntervalForOutRecord(*pitEntry->getOutRecord(outFace));
        }
        ++nEligibleNextHops;
      }
      this->sendInterest(pitEntry, FaceEndpoint(db_m[db_m_index].inFace, 0), interest);
      db_m[db_m_index].inFace = db_m[db_m_index].outFace;
    }
  }
  //------------------------------------------- AFTER -- RECEIVE -- DATA -------------------------------------
  void AMIFStrategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& ingress, const Data& data) {

    auto outRecord = pitEntry->getOutRecord(ingress.face);
    if (outRecord == pitEntry->out_end()) {
      NFD_LOG_DEBUG("Data " << data.getName() << " from=" << ingress << " no out-record");
      return;
    }
    OutRecordInfo* outRecordInfo = outRecord->getStrategyInfo<OutRecordInfo>();
    if (!outRecordInfo || outRecordInfo->pathDiscoveryPhase) {
      //!! Discovery Path Phase.
      NFD_LOG_DEBUG("broadcast data is going back!!");
      //db_index++;
      this->db[db_index].min_bw = data.getBW;
      this->db[db_index].delay = data.getDelay;
      this->db[db_index].id = data.getId;
      this->db[db_index].prefix = data.getPrefix();
      sendDataToAll(pitEntry, ingress, data);
    }
    else {
      //!! Data Distribution phase

      this->beforeSatisfyInterest(pitEntry, ingress, data);

      for (int i = 0; i <= db_m_index; i++) {
        if (db_m[i].prefix != current_prefix) {
          continue;
        }
        if (db_m[i].qouta > 0) {
          sendData(pitEntry, data, db_m[i].outFace);
          db_m[i].qouta--;
          return;
        }
      }

      if (this->period_count == 500) {

        this->period_count = 0;
        for (int i = 0; i <= db_m_index; i++) {
          if (db_m[i].prefix != current_prefix) {
            continue;
          }
          //ndelay
          ndelay = data.getDelay() / max_delay;
          // nbw
          min_bw = data.getBW();
          if (data.getBW() < min_bw) {
            min_bw = data.getBW();
          }
          nbw = min_bw / max_bw;
          //nThroughput
          nthroughput = data.getThroughput;
          db_m[i].score = (nthroughput + nbw) / ndelay;
          db_m[i].qouta = db_m[i].score;
        }

      }
      else {
        for (int i = 0; i <= db_m_index; i++) {
          if (db_m[i].prefix != current_prefix) {
            continue;
          }
          db_m[i].qouta = db_m[i].deg;
        }
      }
      // sort db_m with qouta
      PathStats temp;
      for (int i = 0; i <= db_m_index; i++) {
        if (db_m[i + 1].qouta > db_m[i].qouta || db_m[i + 1].prefix != current_prefix) {
          temp = db_m[i + 1];
          db_m[i + 1] = db_m[i];
          db_m[i] = temp;
        }
      }
      sendData(pitEntry, data, db_m[i].outFace);
      this->period_count++;
    }
  }

  // !! Maintenance 
  void onDroppedInterest(const FaceEndpoint& egress, const Interest& interest) {
    if (egress.linkFailure == true) {
      // delete path
      for (int i = 0; i < db_m_index; i++) {
        if (egress.id == db_m[i]) {
          // delete path i from db_m
          for (int j = i; j <= db_m_index; j++) {
            db_m[i] = db_m[i + 1];
          }
          db_m_index--;
        }
        if (egress.id == db[i]) {
          // delete path i from db
          for (int j = i; j <= db_index; j++) {
            db[i] = db[i + 1];
          }
          db_index--;
        }
      }
    }
  }
  // remove db[db_index];
  db_index--;
  // int id = searchFaceInPath(egress);
  // if (data.dataCounterSend > thereshod) {
  //   // 
    
  //   return;
  // }
}
}

void AMIFStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack, const shared_ptr<pit::Entry>& pitEntry) {
  NFD_LOG_DEBUG("Nack for " << nack.getInterest() << " from=" << ingress
    << " reason=" << nack.getReason());
  if (nack.getReason() == lp::NackReason::NO_ROUTE) { // remove FIB entries
    BOOST_ASSERT(this->lookupFib(*pitEntry).hasNextHops());
    NFD_LOG_DEBUG("Send NACK to all downstreams");
    this->sendNacks(pitEntry, nack.getHeader());
    renewRoute(nack.getInterest().getName(), ingress.face.getId(), 0_ms);
  }
}

void AMIFStrategy::broadcastInterest(const Interest& interest, const Face& inFace, const shared_ptr<pit::Entry>& pitEntry) {
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

void AMIFStrategy::multicastInterest(const Interest& interest, const Face& inFace, const shared_ptr<pit::Entry>& pitEntry, const fib::NextHopList& nexthops) {
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



} // namespace fw
} // namespace nfd