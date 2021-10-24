/*
 * topsis-strategy.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: reza
 */
#define BW	1000000
#define RoutingMetric 1

#include "topsis-strategy.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"
#include "topsis.hpp"


namespace nfd {
namespace fw {

NFD_LOG_INIT(TopsisStrategy);
NFD_REGISTER_STRATEGY(TopsisStrategy);

const time::milliseconds TopsisStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds TopsisStrategy::RETX_SUPPRESSION_MAX(250);

ns3::QueueSize qz;
/**
 * \brief constructor of topsis-strategy
 * @param forwarder instance for run strategy
 * @param name of strategy
 */
TopsisStrategy::TopsisStrategy(Forwarder& forwarder, const Name& name)
:Strategy(forwarder)
, m_measurements(getMeasurements())
, m_retxSuppression(RETX_SUPPRESSION_INITIAL,
        RetxSuppressionExponential::DEFAULT_MULTIPLIER,
        RETX_SUPPRESSION_MAX)
{
	this->countAfterReceiveInterest = 0;
	this->countAfterReceiveNack = 0;
	this->countBeforeSatisfyInterest = 0;

	ParsedInstanceName parsed = parseInstanceName(name);
	 if (!parsed.parameters.empty()) {
	    BOOST_THROW_EXCEPTION(std::invalid_argument("topsis strategy does not accept parameters"));
	 }
	 if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
	    BOOST_THROW_EXCEPTION(std::invalid_argument(
	      "topsis strategy does not support version " + to_string(*parsed.version)));
	 }
	 this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

/**
 * \brief create name for use in scenario
 * @return strategyName
 */
const Name&
TopsisStrategy::getStrategyName(){
	static Name strategyName("/localhost/nfd/strategy/topsis/%FD%01");
	return strategyName;
}

/**
 * \brief trigger after receiving Interest
 * @param inface
 * @param interest
 * @param pitEntry
 */
void
TopsisStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
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
 * \brief trigger after receiving Nack
 * @param inface
 * @param interest
 * @param pitEntry
 */
void
TopsisStrategy::afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry){

	this->countAfterReceiveNack ++;
}

/**
 * \brief trigger before satisfy Interest
 * @param pitEntry
 * @param data
 */
void
TopsisStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const Face& inFace, const Data& data){

	this->countBeforeSatisfyInterest++;
}

/**
 * \brief for calling topsis class methods
 * @param states
 * @param rows the number of faces in fibEntry
 * @return rows topsis choose best face row.
 */
int result(FaceStats* stats, int rows) {

	static const myRtt S_RTT_TIMEOUT = time::microseconds::max();
	static const myRtt S_RTT_NO_MEASUREMENT = S_RTT_TIMEOUT / 2;
	int col = 6;
	float** arr = new float*[256];
	for (int var = 0; var < rows; ++var) {
		arr[var] = new float[col];
		arr[var][0] = stats->availability;
		arr[var][1] = stats->bw;
		arr[var][2] = stats->load;
		arr[var][3] = stats->reliability;
		if(stats->rtt == asf::RttStats::RTT_NO_MEASUREMENT){
			arr[var][4] = (float)S_RTT_NO_MEASUREMENT.count();
		}else{
			arr[var][4] = (float)stats->rtt.count();
		}

		arr[var][5] = stats->routingMetric;
	}

    float weightArray[256] = { 1,6,2,3,4,5};

    char typeValue[256] = { '+', '+', '+', '+','+','+'};

    float **norm= Topsis::normalization(arr,rows,col);

    float **weightMatx = Topsis::weightMatrix(weightArray, col);

    float **multiWeightMatrix = Topsis::multiplexToWeight(norm, rows, col, weightMatx, col, col);

    float* aplus = Topsis::idealPositive(multiWeightMatrix, rows, col, typeValue);

    float* aminus = Topsis::idealNegative(multiWeightMatrix, rows, col, typeValue);

    float* disPos = Topsis::distanceFromPositive(multiWeightMatrix, rows, col, aplus);

    float* disNeg = Topsis::distanceFromNegative(multiWeightMatrix, rows, col, aminus);

    float* result = Topsis::finalRank(disNeg, disPos, rows);

    float bestValue=0;
    int bestRow=0;
    for (int l = 0; l < rows; ++l) {
        if(result[l] > bestValue){
            bestRow = l;
            bestValue = result[l];
        }
    }
    return bestRow;
}

/**
 * \brief the main topsis strategy for send interest
 * @param fibEntry
 * @param interest
 * @param inface
 * @return the best face for sending Interest
 */
Face*
TopsisStrategy::getBestFaceForForwarding(const fib::Entry& fibEntry, const Interest& interest, const Face& inFace){

	const fib::NextHopList& nexthops = fibEntry.getNextHops();
	FaceStats* stats = new FaceStats[10];
	int i=0;

	for (const fib::NextHop& hop : nexthops) {
		Face& hopFace = hop.getFace();
		if((hopFace.getId() == inFace.getId() && hopFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
			wouldViolateScope(inFace, interest, hopFace)) {
			continue;
		}
		float avail = float(countAfterReceiveInterest) / float(countAfterReceiveInterest + countAfterReceiveNack);
		float relay = float(countBeforeSatisfyInterest) / float(countAfterReceiveInterest);
		int queueSize = qz.GetValue();
		asf::FaceInfo* info = m_measurements.getFaceInfo(fibEntry, interest, hopFace.getId());
		if(info == nullptr){
			// set Facestate structure for topsis Rank if info is null
			stats[i] = {&hopFace, avail, relay, queueSize, BW, asf::RttStats::RTT_NO_MEASUREMENT, RoutingMetric};
		}else{
			// set Facestate structure for topsis Rank
			stats[i] = {&hopFace, avail, relay, queueSize, BW, info->getRtt(), RoutingMetric};
		}
		i++;
	}

	// return best row of states array
	int bestRow = result(stats, nexthops.size());
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
TopsisStrategy::sendNoRouteNack(const Face& inFace, const Interest& interest,
                             const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " noNextHop");
  lp::NackHeader nackHeader;
  nackHeader.setReason(lp::NackReason::NO_ROUTE);
  this->sendNack(pitEntry, inFace, nackHeader);
}

} /* namespace fw */
} /* namespace nfd */
