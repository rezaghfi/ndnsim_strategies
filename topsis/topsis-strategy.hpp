/*
 * topsis-strategy.hpp
 *
 *  Created on: Jun 26, 2019
 *      Author: reza
 */

#ifndef NDNSIM_NFD_DAEMON_FW_TOPSIS_STRATEGY_HPP_
#define NDNSIM_NFD_DAEMON_FW_TOPSIS_STRATEGY_HPP_

#include "strategy.hpp"
#include "asf-measurements.hpp"
#include "fw/retx-suppression-exponential.hpp"
// for queue size : calculating load
#include "/home/reza/ndnSIM/ns-3/src/network/utils/queue-size.h"

namespace nfd {
namespace fw {

typedef time::duration<double, boost::micro> myRtt;

/**
 * this is Structure of parameter to set for Topsis input
 */
struct FaceStats{
	Face* face;
	float availability;
	float reliability;
	int load;
	int bw;
	myRtt rtt;
	int routingMetric;
};

class TopsisStrategy: public Strategy {
public:
explicit
TopsisStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

static const Name&
getStrategyName();

void
afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

void
afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override;

void
beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                        const Face& inFace, const Data& data) override;
private:


Face*
  getBestFaceForForwarding(const fib::Entry& fibEntry, const Interest& interest, const Face& inFace);

void
sendNoRouteNack(const Face& inFace, const Interest& interest,
                             const shared_ptr<pit::Entry>& pitEntry);

private:

	int countBeforeSatisfyInterest;
	int countAfterReceiveInterest;
	int countAfterReceiveNack;
	asf::AsfMeasurements m_measurements;
	RetxSuppressionExponential m_retxSuppression;
	static const time::milliseconds RETX_SUPPRESSION_INITIAL;
	static const time::milliseconds RETX_SUPPRESSION_MAX;
};
} /* namespace fw */
} /* namespace nfd */

#endif /* NDNSIM_NFD_DAEMON_FW_TOPSIS_STRATEGY_HPP_ */
