// ndn-grid-topo-plugin.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/simple-topology.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> producer = Names::Find<Node>("Node6");
  NodeContainer consumerNodes;
  consumerNodes.Add(Names::Find<Node>("Node0"));

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerWindow");
  consumerHelper.SetPrefix(prefix);
  // start interest speed from 20
  consumerHelper.SetAttribute("Window", StringValue("20"));
   // Expected size of the Data payload
  consumerHelper.SetAttribute("PayloadSize", StringValue("2000"));
   // Amount of data to be requested
  consumerHelper.SetAttribute("Size", StringValue("3"));
  consumerHelper.Install(consumerNodes);


  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("4194304"));
  producerHelper.Install(producer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  // simulation time is 50 seconds
  Simulator::Stop(Seconds(50.0));

    // tracer --------- it for rate trace and drop trace
  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(0.5));
  L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));
  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}