// abdi ndnsim forwarding strategy header file for multipassing article

#ifndef NFD_DAEMON_FW_MULTIPASS_STRATEGY_HPP
#define NFD_DAEMON_FW_MULTIPASS_STRATEGY_HPP

#include "strategy.hpp"
#include "process-nack-traits.hpp"
#include "retx-suppression-exponential.hpp"
#include <iostream>
#include <string>

using namespace std;

namespace nfd {
namespace fw {


// PathStats is structure of pathes in database db and db_m
struct PathStats{
  int id;
  char prefix;
  double min_bw;
  int delay;
  int throughput;
  double deg;
  double degree;
  double qouta;
  double score;
  Face *outFace;
  Face *inFace;
};


/** \brief multipass forward strategy
 *
 *  This strategy first broadcasts Interest to learn a multi path towards data,
 *  then forwards subsequent Interests along the learned pathes
 *
 *  \see https://github.com/rezaghfi
 *
 *  \note This strategy is not EndpointId-aware
 */
class MultiPassStrategy : public Strategy
                        , public ProcessNackTraits<MultiPassStrategy>
{
public:
  explicit
  MultiPassStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
          const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
          const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveData (const shared_ptr<pit::Entry>& pitEntry,
					const FaceEndpoint &ingress, const Data &data) override;
private:
  friend ProcessNackTraits<MultiPassStrategy>;
  RetxSuppressionExponential m_retxSuppression;
  PathStats db[100] , db_m[10];
  int sufNumOfPath;
  bool isPathDiscoveryPhase;
  int multipass_num_max;
  int period_count;
PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_MUTIPASS_STRATEGY_HPP
