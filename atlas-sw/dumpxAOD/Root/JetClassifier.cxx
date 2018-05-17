#include "Root/JetClassifier.h"

// EDM
#include "xAODJet/Jet.h"

// Externals
#include "lwtnn/LightweightGraph.hh"
#include "lwtnn/NanReplacer.hh"
#include "lwtnn/parse_json.hh"

// C++ includes
#include <stdexcept>
#include <map>
#include <string>
#include <cmath>

JetClassifier::JetClassifier(std::istream& stream):
  m_rnnip_pu("rnnip_pu"),
  m_rnnip_pb("rnnip_pb"),
  m_jf_sig("JetFitter_significance3d"),
  m_nn_light("nn_light"),
  m_nn_charm("nn_charm"),
  m_nn_bottom("nn_bottom"),
  m_graph(nullptr),
  m_replacer(nullptr)
{
  lwt::GraphConfig config = lwt::parse_json_graph(stream);
  m_graph.reset(new lwt::LightweightGraph(config));
  if (config.inputs.size() != 1) {
    throw std::logic_error("only one input node allowed");
  }
  m_replacer.reset(
    new lwt::NanReplacer(config.inputs.at(0).defaults, lwt::rep::all));
}

void JetClassifier::decorate(const xAOD::Jet& jet) const {

  // access input variables
  const SG::AuxElement* btag = jet.btagging();
  double rnnip_log_ratio = m_rnnip_pb(*btag) / m_rnnip_pu(*btag);
  double jf_sig = m_jf_sig(*btag);

  // build them into a map
  std::map<std::string, std::map<std::string, double> > inputs {
    {"btag_variables", m_replacer->replace({
          {"jf_sig_log1p", std::log1p(jf_sig)},
          {"rnnip_log_ratio", rnnip_log_ratio} })}};

  // calculate output scores
  auto out_classes = m_graph->compute(inputs);

  // store outputs in the jet
  m_nn_light(*btag) = out_classes.at("light");
  m_nn_charm(*btag) = out_classes.at("charm");
  m_nn_bottom(*btag) = out_classes.at("bottom");
}
