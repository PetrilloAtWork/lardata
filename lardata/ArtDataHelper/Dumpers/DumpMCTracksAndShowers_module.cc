/**
 * @file   lardata/ArtDataHelper/Dumpers/DumpMCTracksAndShowers_module.cc
 * @brief  Dumps the content of the `sim::MCTrack and `sim::MCShower` objects.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   March 26, 2025
 */

// LArSoft libraries
#include "lardataalg/MCDumpers/MCDumperUtils.h" // sim::ParticleName(), ...
#include "lardataobj/MCBase/MCStep.h"
#include "lardataobj/MCBase/MCTrack.h"
#include "lardataobj/MCBase/MCShower.h"

// art libraries
#include "art/Framework/Core/SharedAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/InputTag.h"

// support libraries
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Comment.h"
#include "fhiclcpp/types/Name.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "range/v3/view/enumerate.hpp"

// C//C++ standard libraries
#include <iomanip> // std::setfill(), std::setw()
#include <ostream> // std::setfill(), std::setw()
#include <string> // std::to_string()
#include <variant> // std::visit(), ...
#include <vector>

// -----------------------------------------------------------------------------
namespace sim {
  class DumpMCTracksAndShowers;
}
/**
 * @brief Prints the content of all `sim::MCTrack` and `sim::MCShower` objects.
 *
 * This analyzer prints the content of all the into the `sim::MCTrack` and
 * `sim::MCShower` objects from a single producer into a LogInfo/LogVerbatim
 * stream.
 * 
 * If the input file has no track or no shower data product, the output will
 * show that. If the input file has no tracks _and_ no shower data products,
 * an exception will be thrown.
 *
 *
 * Configuration parameters
 * =========================
 *
 * - *MCRecoTag* (input tag, mandatory): tag of data product containing the
 *   objects to dump.
 * - *SkipTracks* (flag; default: `false`): if set, no MC track will be printed.
 * - *SkipShowers* (flag; default: `false`): if set, no MC shower will be
 *   printed.
 * - *OutputCategory* (string, default: "DumpMCTracksAndShowers"): the category
 *   used for the output (useful for filtering).
 *
 */
class sim::DumpMCTracksAndShowers : public art::SharedAnalyzer {
public:
  struct Config {
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;

    fhicl::Atom<art::InputTag> MCRecoTag{
      Name{"MCRecoTag"},
      Comment{"tag of data product containing the objects to dump"}
      };

    fhicl::Atom<bool> SkipTracks{
      Name{"SkipTracks"},
      Comment{"do not dump MC tracks even if present"},
      false
      };
      
    fhicl::Atom<bool> SkipShowers{
      Name{"SkipShowers"},
      Comment{"do not dump MC showers even if present"},
      false
      };

    fhicl::Atom<std::string> OutputCategory{
      Name{"OutputCategory"},
      Comment{"the messagefacility category used for the output"},
      "DumpMCTracksAndShowers"};

  }; // struct Config

  using Parameters = art::SharedAnalyzer::Table<Config>;

  /// Constructor: reads the configuration.
  explicit DumpMCTracksAndShowers(Parameters const& params, const art::ProcessingFrame&);

  /// Does the printing.
  void analyze(art::Event const& event, const art::ProcessingFrame&) override;

private:
  
  // --- BEGIN --  Configuration  ----------------------------------------------
  art::InputTag const fMCRecoTag;    ///< Tag for input data products.
  bool const fSkipTracks;            ///< Whether to ignore MC tracks.
  bool const fSkipShowers;           ///< Whether to ignore MC showers.
  std::string const fOutputCategory; ///< Category for LogInfo output.
  // --- END ----  Configuration  ----------------------------------------------

  /// Dumps all the MC `Obj` from `fMCRecoTag` found in the event.
  /// @tparam Obj type of object to dump
  /// @tparam Descr description of the object used in the messages
  /// @return the number of dumped data products
  template <typename Obj, const char* Descr>
  unsigned int dumpMCobject(art::Event const& event) const;
  
}; // class sim::DumpMCTracksAndShowers


//------------------------------------------------------------------------------
//---  dumping implementation
//------------------------------------------------------------------------------
namespace sim {
  template <typename Obj>
  struct toStream;
  
  template <typename Obj>
  std::ostream& operator<< (std::ostream& out, toStream<Obj> const& obj);
  
}

namespace details {
  
  template <typename V>
  struct asVector;
  
  template <typename V>
  std::ostream& operator<< (std::ostream& out, asVector<V> const& v);
  
  void dump(std::ostream& out, sim::MCStep const& step, std::string const& indent = "", std::string const& firstIndent = "");
  
  template <typename Obj>
  void dumpMCobjBase(std::ostream& out, Obj const& obj, std::string const& indent = "", std::string const& firstIndent = "");
  
  void dump(std::ostream& out, sim::MCTrack const& track, std::string const& indent = "", std::string const& firstIndent = "");
  void dump(std::ostream& out, sim::MCShower const& shower, std::string const& indent = "", std::string const& firstIndent = "");
  
} // namespace details

//------------------------------------------------------------------------------
template <typename Obj>
struct sim::toStream {
  Obj const* obj;
  toStream(Obj const& obj): obj{ &obj } {}
}; // sim::toStream

template <typename Obj>
std::ostream& sim::operator<< (std::ostream& out, toStream<Obj> const& dumper)
  { details::dump(out, *(dumper.obj)); return out; }

//------------------------------------------------------------------------------
template <typename V>
struct details::asVector {
  V const* v;
  asVector(V const& v): v{ &v} {}
}; // sim::asVector

template <typename V>
std::ostream& details::operator<< (std::ostream& out, asVector<V> const& dumper) {
  out << "( " << dumper.v->X() << " , "<< dumper.v->Y() << " , "<< dumper.v->Z() << " )";
  return out;
}


//------------------------------------------------------------------------------
void details::dump(
  std::ostream& out, sim::MCStep const& step,
  std::string const& /* indent = "" */, std::string const& firstIndent /* = "" */)
{
  out << firstIndent << "at " << asVector(step) << " t=" << step.T()
    << ", cp=( " << step.E() << " ; " << step.Px() << " , " << step.Py() << " , " << step.Pz()
    << " )";
}


//------------------------------------------------------------------------------
template <typename Obj>
void details::dumpMCobjBase(
  std::ostream& out, Obj const& obj,
  std::string const& indent /* = "" */, std::string const& firstIndent /* = "" */)
{
  // GCC requires qualification of `toStream()` here, bypassing ADL, not sure why

  // we skip most mother and ancestor information
  out << firstIndent << "track ID=" << obj.TrackID()
    << " from " << sim::ParticleName(obj.PdgCode())
    << " created by " << obj.Process() << " at " << sim::toStream(obj.Start())
    << '\n' << indent
    << "from " << sim::ParticleName(obj.MotherTrackID()) << " [ID=" << obj.MotherTrackID()
    << "], originally from " << sim::ParticleName(obj.AncestorPdgCode()) << " [ID=" << obj.AncestorTrackID()
    << "] (" << sim::TruthOriginName(obj.Origin()) << ")"
    << '\n' << indent
    << " ends at " << sim::toStream(obj.End());

} // details::dumpMCobjBase()


//------------------------------------------------------------------------------
void details::dump(
  std::ostream& out, sim::MCTrack const& track,
  std::string const& indent /* = "" */, std::string const& firstIndent /* = "" */)
{
  constexpr unsigned int MaxColumns = 6;
  
  dumpMCobjBase(out, track, indent, firstIndent);
  out << " after " << track.dEdx().size() << " steps;";
  
  auto consistentVectors = [](auto const& dEdx, auto const& dQdxs){
      for (auto const& dQdx: dQdxs) if (dQdx.size() != dEdx.size()) return false;
      return true;
    };
  
  if (consistentVectors(track.dEdx(), track.dQdx())) {
    
    auto skipEntry = [](std::size_t i, auto const& dEdx, auto const& dQdxs){
      if (dEdx[i] != 0) return false;
      for (auto const& dQdx: dQdxs) if (dQdx[i] != 0) return false;
      return true;
      };

    out << " those with dE/dx or dQ/dx > 0:";
    for (std::size_t iStep = 0; iStep < track.dEdx().size(); ++iStep) {
      if (skipEntry(iStep, track.dEdx(), track.dQdx())) continue;
      out << '\n' << indent << " [" << iStep << "] E=" << track.dEdx()[iStep];
      for (auto const& [ iPlane, dQdx ]: ranges::view::enumerate(track.dQdx()))
        if (dQdx[iStep] != 0) out << " [P#" << iPlane << "] dQ/dx=" << dQdx[iStep];
    } // for step
  }
  else {
    out << " those with dE/dx > 0:";
    unsigned int column = 0;
    for (auto const& [ iStep, dEdx ]: track.dEdx() | ranges::view::enumerate) {
      if (dEdx == 0) continue;
      if (++column > MaxColumns) { out << '\n' << indent; column = 0; }
      out << " [" << iStep << "] " << dEdx;
    }
    
    out << '\n' << indent << " charge from " << track.dQdx().size() << " planes:";
    for (auto const& [ iPlane, dQdxs ]: track.dQdx() | ranges::view::enumerate) {
      unsigned int column = 0;
      out << '\n' << indent << " [P#" << iPlane << "] " << dQdxs.size() << " steps:";
      for (auto const& [ iStep, dQdx ]: dQdxs | ranges::view::enumerate) {
        if (dQdx == 0) continue;
        if (++column > MaxColumns) { out << '\n' << indent; column = 0; }
        out << " [" << iStep << "] dQ/dx=" << dQdx;
      }
    }
  }
  
} // details::dump(sim::MCTrack)


//------------------------------------------------------------------------------
void details::dump(std::ostream& out, sim::MCShower const& shower, std::string const& indent /* = "" */, std::string const& firstIndent /* = "" */)
{
  dumpMCobjBase(out, shower, indent, firstIndent);
  out << '\n' << indent << " with " << shower.DaughterTrackID().size() << " daughters (IDs:";
  for (unsigned int daughterID: shower.DaughterTrackID()) out << ' ' << daughterID;
  out << " )";
  
  out << '\n' << indent << " dE/dx=" << shower.dEdx() << " and direction "
    << asVector(shower.StartDir()) << "; profile: " << sim::toStream(shower.DetProfile());
  
  out << '\n' << indent << " charge from " << shower.Charge().size() << " planes:";
  for (auto const& [ iPlane, Q ]: shower.Charge() | ranges::view::enumerate)
    out << " [P#" << iPlane << "] " << Q;
  
  out << '\n' << indent << " dQ/dx from " << shower.dQdx().size() << " planes:";
  for (auto const& [ iPlane, dQdx ]: shower.Charge() | ranges::view::enumerate)
    out << " [#" << iPlane << "] " << dQdx;
  
} // details::dump(sim::MCShower)


//------------------------------------------------------------------------------
//---  module implementation
//------------------------------------------------------------------------------
sim::DumpMCTracksAndShowers::DumpMCTracksAndShowers(Parameters const& params, const art::ProcessingFrame&)
  : art::SharedAnalyzer{ params }
  , fMCRecoTag     { params().MCRecoTag() }
  , fSkipTracks    { params().SkipTracks() }
  , fSkipShowers   { params().SkipShowers() }
  , fOutputCategory{ params().OutputCategory() }
{
  
  async<art::InEvent>();
  
  //
  // configuration checks
  //
  if (fSkipShowers && fSkipTracks) {
    throw art::Exception{ art::errors::Configuration }
      << "At least one among tracks and showers must not be skipped.\n";
  }
  
  //
  // input declaration
  //
  if (!fSkipTracks)  mayConsume<std::vector<sim::MCTrack>> (fMCRecoTag);
  if (!fSkipShowers) mayConsume<std::vector<sim::MCShower>>(fMCRecoTag);
  
}


//------------------------------------------------------------------------------
void sim::DumpMCTracksAndShowers::analyze(art::Event const& event, const art::ProcessingFrame&)
{
  
  unsigned int nDumpedProducts = 0;
  
  
  if (!fSkipTracks) {
    static const char TracksDescr[] = "MC tracks";
    nDumpedProducts += dumpMCobject<sim::MCTrack, TracksDescr>(event);
  }
  if (!fSkipTracks) {
    static const char ShowersDescr[] = "MC showers";
    nDumpedProducts += dumpMCobject<sim::MCShower, ShowersDescr>(event);
  }
  
  if (nDumpedProducts == 0) {
    throw art::Exception{ art::errors::ProductNotFound }
      << "No MC track nor shower data product is available with tag '"
      << fMCRecoTag.encode() << "'!\n";
  }
  
} // sim::DumpMCTracksAndShowers::analyze()


// -----------------------------------------------------------------------------
template <typename Obj, const char* Descr>
unsigned int sim::DumpMCTracksAndShowers::dumpMCobject(art::Event const& event) const {
  
  //
  // get the data product
  //
  auto const& hMCObjs = event.getHandle<std::vector<Obj>>(fMCRecoTag);
  if (!hMCObjs) {
    mf::LogVerbatim{ fOutputCategory }
      << event.id() << " has no '" << fMCRecoTag.encode() << "'.";
    return 0;
  }
  
  if (hMCObjs->empty()) {
    mf::LogVerbatim{ fOutputCategory }
      << event.id() << " has no " << Descr << " in '" << fMCRecoTag.encode() << "'.";
  }
  else {
  
    mf::LogVerbatim log{ fOutputCategory };
    log << event.id() << " has " << hMCObjs->size() << " " << Descr << " in '" << fMCRecoTag.encode() << "':";
    
    unsigned int const padding = std::to_string(hMCObjs->size() - 1).length();
    
    for (auto const& [ iObj, obj ]: *hMCObjs | ranges::views::enumerate) {
      log << "\n[" << std::setfill('0') << std::setw(padding) << iObj << "] "
        << toStream(obj);
    }
    
  }
  
  return 1;
  
} // sim::DumpMCTracksAndShowers::dumpMCobject()


// -----------------------------------------------------------------------------
DEFINE_ART_MODULE(sim::DumpMCTracksAndShowers)

// -----------------------------------------------------------------------------
