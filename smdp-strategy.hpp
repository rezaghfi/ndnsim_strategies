

#ifndef NDNSIM_NFD_DAEMON_FW_SMDP_STRATEGY_HPP_
#define NDNSIM_NFD_DAEMON_FW_SMDP_STRATEGY_HPP_

#include "strategy.hpp"
#include "asf-measurements.hpp"
#include "fw/retx-suppression-exponential.hpp"


namespace nfd {
namespace fw {

typedef time::duration<double, boost::micro> myRtt;

/**
 * this is Structure of parameter to set for input
 */
struct FaceStats{
	Face* face;
	int request_class;
	int reward;
	int unSatisfiedInterest;
};

class SMDPStrategy: public Strategy {
public:
explicit

SMDPStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

static const Name& getStrategyName();

void afterReceiveInterest(const Face& inFace, const Interest& interest,
                    const shared_ptr<pit::Entry>& pitEntry) override;

private:

Face*
  getBestFaceForForwarding(const fib::Entry& fibEntry, 
  					const Interest& interest, const Face& inFace);

void
sendNoRouteNack(const Face& inFace, const Interest& interest,
                    const shared_ptr<pit::Entry>& pitEntry);

private:

	RetxSuppressionExponential m_retxSuppression;
	static const time::milliseconds RETX_SUPPRESSION_INITIAL;
	static const time::milliseconds RETX_SUPPRESSION_MAX;
};
} /* namespace fw */
} /* namespace nfd */

#endif /* NDNSIM_NFD_DAEMON_FW_SMDP_STRATEGY_HPP_ */
