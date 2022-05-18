/**
 * @author: reza gholamalitabar
 * @date: 12 tir 1400 10:02
*/

#ifndef NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP
#define NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP

#include "fw/strategy.hpp"
#include <ndn-cxx/lp/prefix-announcement-header.hpp>
#include <string>

using namespace std;

namespace nfd {
  namespace fw {

    // PathStats is structure of pathes in database db and db_m
    struct PathStats {
      int id;
      string prefix;
      double min_bw;
      int delay;
      int throughput;
      double ndelay;
      double nbw;
      double deg;
      double degree;
      double qouta;
      double score;
      Face* outFace;
      Face* inFace;
    };

    /** \brief multipass forward strategy
     *  This strategy first broadcasts Interest to learn a multi path towards data,
     *  then forwards subsequent Interests along the learned pathes
     *  \see https://github.com/rezaghfi/ndnSIM
     *
     *  \note This strategy is not EndpointId-aware
     */
    class MultiPassStrategy : public Strategy
    {
      public:
      explicit
        MultiPassStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

      static const Name&
        getStrategyName();

      // StrategyInfo on pit::InRecord
      class InRecordInfo : public StrategyInfo
      {
        public:
        static constexpr int
          getTypeId()
        {
          return 6666;
        }

        public:
        bool pathDiscoveryPhase = true;
        bool pathSelectionPhase = false;
        bool dataDistributionPhase = false;
        bool maintenancePhase = false;
      };

      // StrategyInfo on pit::OutRecord
      class OutRecordInfo : public StrategyInfo
      {
        public:
        static constexpr int
          getTypeId()
        {
          return 7777;
        }

        public:
        bool pathDiscoveryPhase = true;
        bool pathSelectionPhase = false;
        bool dataDistributionPhase = false;
        bool maintenancePhase = false;
      };

      public: // triggers

      void afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry) override;

      void afterReceiveData(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& ingress, const Data& data) override;

      void afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack, const shared_ptr<pit::Entry>& pitEntry) override;

      void onDroppedInterest(const FaceEndpoint& egress, const Interest& interest) override;

      private: // operations

        /** \brief Send an Interest to all possible faces
         *
         *  This function is invoked when the forwarder has no matching FIB entries for
         *  an incoming discovery Interest, which will be forwarded to faces that
         *    - do not violate the Interest scope
         *    - are non-local
         *    - are not the face from which the Interest arrived, unless the face is ad-hoc
         */
      void
        broadcastInterest(const Interest& interest, const Face& inFace,
          const shared_ptr<pit::Entry>& pitEntry);

      /** \brief Send an Interest to \p nexthops
       */
      void
        multicastInterest(const Interest& interest, const Face& inFace,
          const shared_ptr<pit::Entry>& pitEntry,
          const fib::NextHopList& nexthops);

      /** \brief Find a Prefix Announcement for the Data on the RIB thread, and forward
       *         the Data with the Prefix Announcement on the main thread
       */
      void
        asyncProcessData(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data);

      /** \brief Check whether a PrefixAnnouncement needs to be attached to an incoming Data
       *
       *  The conditions that a Data packet requires a PrefixAnnouncement are
       *    - the incoming Interest was discovery and
       *    - the outgoing Interest was non-discovery and
       *    - this forwarder does not directly connect to the consumer
       */
      static bool
        needPrefixAnn(const shared_ptr<pit::Entry>& pitEntry);

      /** \brief Add a route using RibManager::slAnnounce on the RIB thread
       */
      void
        addRoute(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace,
          const Data& data, const ndn::PrefixAnnouncement& pa);

      /** \brief renew a route using RibManager::slRenew on the RIB thread
       */
      void
        renewRoute(const Name& name, FaceId inFaceId, time::milliseconds maxLifetime);

      private:
      PathStats db[1000], db_m[100];
      int sufNumOfPath;
      bool isPathDiscoveryPhase;
      int multipass_num_max;
      int period_count;
      static const time::milliseconds ROUTE_RENEW_LIFETIME;
    };

  } // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP