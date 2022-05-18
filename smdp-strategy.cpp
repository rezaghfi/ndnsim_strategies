/*
 * smdp-strategy.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: reza
 */
// bandwitdh byte/second
#define BW	1000000
#define RoutingMetric 1

#include "smdp-strategy.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"


namespace nfd {
namespace fw {

NFD_LOG_INIT(smdpStrategy);
NFD_REGISTER_STRATEGY(smdpStrategy);

const time::milliseconds smdpStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds smdpStrategy::RETX_SUPPRESSION_MAX(250);

ns3::QueueSize countFaceInterests;
/**
 * \brief constructor of smdp-strategy
 * @param forwarder instance for run strategy
 * @param name of strategy
 */
smdpStrategy::smdpStrategy(Forwarder& forwarder, const Name& name)
:Strategy(forwarder)
, m_retxSuppression(RETX_SUPPRESSION_INITIAL,
        RetxSuppressionExponential::DEFAULT_MULTIPLIER,
        RETX_SUPPRESSION_MAX)
{
	ParsedInstanceName parsed = parseInstanceName(name);
	 if (!parsed.parameters.empty()) {
	    BOOST_THROW_EXCEPTION(std::invalid_argument("smdp strategy does not accept parameters"));
	 }
	 if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
	    BOOST_THROW_EXCEPTION(std::invalid_argument(
	      "smdp strategy does not support version " + to_string(*parsed.version)));
	 }
	 this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

/**
 * \brief create name for use in scenario
 * @return strategyName
 */
const Name&
smdpStrategy::getStrategyName(){
	static Name strategyName("/localhost/nfd/strategy/smdpstrategy/%FD%01");
	return strategyName;
}

/**
 * \brief trigger after receiving Interest
 * @param inface
 * @param interest
 * @param pitEntry
 */
void
smdpStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry){
  /* todo interest can three kinds:
   * NEW, FORWARD, SUppress
	*/
  RetxSuppressionResult suppressResult = m_retxSuppression.decidePerPitEntry(*pitEntry);

  switch (suppressResult) {
  case RetxSuppressionResult::NEW:
  case RetxSuppressionResult::FORWARD:
	break;
  case RetxSuppressionResult::SUPPRESS:
	NFD_LOG_DEBUG("SUPPRESS Interest:" << interest << " from=" << inFace.getId() << "DROPED");
	return;
  }

  // find fibentry with pitEntry Pointer
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  // if vector size is zero, send NO_ROUTE NACK and return
  if (nexthops.size() == 0) {
	sendNoRouteNack(inFace, interest, pitEntry);
	this->rejectPendingInterest(pitEntry);
	return;
  }

  // call function to set output face to choose best face
  Face* faceToUse = getBestFaceForForwarding(fibEntry, interest, inFace);

  if (faceToUse == nullptr) {
		sendNoRouteNack(inFace, interest, pitEntry);
		this->rejectPendingInterest(pitEntry);
		return;
	  }

  this->sendInterest(pitEntry, *faceToUse, interest);
  NFD_LOG_DEBUG(interest << " from=" << inFace.getId()
                         << " pitEntry-to=" << faceToUse->getId());
  this->countAfterReceiveInterest++;

}

/**
 * \brief for calling class methods
 * @param states
 * @param rows the number of faces in fibEntry
 * @return rows  choose best face row.
 */
int smdpresult(FaceStats* stats, int rows) {
	
	int bestValue=0;
    int bestRow=0;
    for (int l = 0; l < rows; ++l) {
        if(stats[l].reward > bestValue){
            bestRow = l;
            bestValue = stats[l].reward;
        }
    }
    return bestRow;
}

/**
 * \brief the main strategy for send interest
 * @param fibEntry
 * @param interest
 * @param inface
 * @return the best face for sending Interest
 */
Face*
smdpStrategy::getBestFaceForForwarding(const fib::Entry& fibEntry, const Interest& interest, const Face& inFace){

	const fib::NextHopList& nexthops = fibEntry.getNextHops();
	FaceStats* stats = new FaceStats[10];
	// code for smdp 
	int class = 0;
	int this->request_class = 0;
	int bw_demand = 0;
	int rejection_cost = 0;
	int i = 0;
	ForwarderCounters m_counters;

	for (const fib::NextHop& hop : nexthops) {
		Face& hopFace = hop.getFace();
		// set ndnsim counter
		m_counters = hopFace.getCounter();
		// set face pointer 
		stats[i].outFace = &hopFace;
		//available bw
		stats[i].availBW = bw.nInbytes + bw.nOutBytes;
		// set unsatisfied interest
		stats[i].unsatisfiedInterest = m_counters.nUnsatisfiedInterests;

		i++; 
	}
	// return best row of states array
	int bestRow = smdpresult(stats, nexthops.size());
	// set output face for send interest
	Face* faceToUse = stats[bestRow].face;

	return faceToUse;

}

/**
 * \brief send the NoRoute packet to faces
 * @param inface
 * @param interest
 * @param pitEntity
 */
void
smdpStrategy::sendNoRouteNack(const Face& inFace, const Interest& interest,
                             const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::NO_ROUTE);
  this->sendNack(pitEntry, inFace, nackHeader);
}

} /* namespace fw */
} /* namespace nfd */
