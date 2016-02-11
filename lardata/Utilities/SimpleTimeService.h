/**
 * \file SimpleTimeService.h
 *
 * \ingroup SimpleTimeService
 * 
 * \brief Class def header for a class SimpleTimeService
 *
 * @author kterao
 */

/** \addtogroup SimpleTimeService

    @{*/
#ifndef SIMPLETIMESERVICE_H
#define SIMPLETIMESERVICE_H

#include <iostream>
#include "ElecClock.h"
#include "lardata/RawData/TriggerData.h"

namespace util {
  /**
     \class SimpleTimeService
     This class provides electronics various electronics clocks. Currently supports
     three types of clocks: TPC, Optical, and Trigger in order to support MicroBooNE experiments.

     Added another clock: External. Currently reserved for 35ton muon paddle system. -mthiesse, 29/6/2015
  */
  class SimpleTimeService{
    
  protected:
    
    /// Default constructor
    SimpleTimeService() 
      : fG4RefTime    (kDEFAULT_MC_CLOCK_T0),
	fFramePeriod  (kDEFAULT_FRAME_PERIOD),
	fTPCClock     (0,kDEFAULT_FRAME_PERIOD,kDEFAULT_FREQUENCY_TPC),
	fOpticalClock (0,kDEFAULT_FRAME_PERIOD,kDEFAULT_FREQUENCY_OPTICAL),
	fTriggerClock (0,kDEFAULT_FRAME_PERIOD,kDEFAULT_FREQUENCY_TRIGGER),
      fExternalClock(0,kDEFAULT_FRAME_PERIOD,kDEFAULT_FREQUENCY_EXTERNAL),
	fTriggerOffsetTPC     (kDEFAULT_TRIG_OFFSET_TPC),
	fTriggerTime  (0),
	fBeamGateTime (0)
    {}
    
    /// Default destructor
    ~SimpleTimeService(){};

    /// Electronics clock counting start time in G4 time frame [us]
    double fG4RefTime;

    /// Frame period
    double fFramePeriod;

    /// TPC clock
    ::util::ElecClock fTPCClock;

    /// Optical clock
    ::util::ElecClock fOpticalClock;

    /// Trigger clock
    ::util::ElecClock fTriggerClock;

    /// External clock
    ::util::ElecClock fExternalClock;

    /// Time offset from trigger to TPC readout start
    double fTriggerOffsetTPC;

    /// Trigger time in [us]
    double fTriggerTime;

    /// BeamGate time in [us]
    double fBeamGateTime;

    /// self pointer as a singleton
    static SimpleTimeService* _me;

  protected:


    /// Setter for trigger times
    virtual void SetTriggerTime(double trig_time, double beam_time)
    { 
      fTriggerTime  = trig_time;
      fBeamGateTime = beam_time;
      fTPCClock.SetTime(trig_time);
      fOpticalClock.SetTime(trig_time);
      fTriggerClock.SetTime(trig_time);
      fExternalClock.SetTime(trig_time);
    }

  public:

    static const SimpleTimeService* GetME()
    {
      if(!_me) _me = new SimpleTimeService;
      return _me;
    }

    /// Given Geant4 time [ns], returns relative time [us] w.r.t. electronics time T0 
    double G4ToElecTime(double g4_time) const {return g4_time * 1.e-3 - fG4RefTime; }

    /// TPC readout start time offset from trigger
    virtual double TriggerOffsetTPC() const { return fTriggerOffsetTPC; }

    /// Trigger electronics clock time in [us]
    double TriggerTime() const { return fTriggerTime; }

    /// Beam gate electronics clock time in [us]
    double BeamGateTime() const { return fBeamGateTime; }

    //
    // Getters of TPC ElecClock
    //
    /// Borrow a const TPC clock with time set to Trigger time [us]
    const ElecClock& TPCClock() const
    { return fTPCClock; }

    /// Create a TPC clock for a given time [us] from clock counting start
    ElecClock TPCClock(double time) const 
    { return ElecClock(time,fTPCClock.FramePeriod(),fTPCClock.Frequency());}

    /// Create a TPC clock for a given sample/frame number in TPC clock frequency
    ElecClock TPCClock(unsigned int sample,
		       unsigned int frame) const
    { ElecClock clock = TPCClock(); clock.SetTime(sample,frame); return clock; }

    //
    // Getters of Optical ElecClock
    //
    /// Borrow a const Optical clock with time set to Trigger time [us]
    const ElecClock& OpticalClock() const
    { return fOpticalClock; }

    /// Create a Optical clock for a given time [us] from clock counting start
    ElecClock OpticalClock(double time) const 
    { return ElecClock(time,fOpticalClock.FramePeriod(),fOpticalClock.Frequency());}

    /// Create a Optical clock for a given sample/frame number in Optical clock frequency
    ElecClock OpticalClock(unsigned int sample,
			   unsigned int frame) const
    { ElecClock clock = OpticalClock(); clock.SetTime(sample,frame); return clock; }

    //
    // Getters of Trigger ElecClock
    //
    /// Borrow a const Trigger clock with time set to Trigger time [us]
    const ElecClock& TriggerClock() const
    { return fTriggerClock; }

    /// Create a Trigger clock for a given time [us] from clock counting start    
    ElecClock TriggerClock(double time) const 
    { return ElecClock(time,fTriggerClock.FramePeriod(),fTriggerClock.Frequency());}

    /// Create a Trigger clock for a given sample/frame number in Trigger clock frequency
    ElecClock TriggerClock(unsigned int sample,
			   unsigned int frame) const
    { ElecClock clock = TriggerClock(); clock.SetTime(sample,frame); return clock; }

    //
    // Getters of External ElecClock
    //
    /// Borrow a const Trigger clock with time set to External Time [us]
    const ElecClock& ExternalClock() const
    { return fExternalClock; }

    /// Create a External clock for a given time [us] from clock counting start
    ElecClock ExternalClock(double time) const
    { return ElecClock(time,fExternalClock.FramePeriod(),fTriggerClock.Frequency());}

    /// Create a External clock for a given sample/frame number in External clock frequency
    ElecClock ExternalClock(unsigned int sample,
			    unsigned int frame) const
    { ElecClock clock = ExternalClock(); clock.SetTime(sample,frame); return clock; }

    //
    // Getters for time [us] w.r.t. trigger given information from waveform
    //

    /// Given TPC time-tick (waveform index), returns time [us] w.r.t. trigger time stamp
    double TPCTick2TrigTime(double tick) const
    { return fTPCClock.TickPeriod() * tick + TriggerOffsetTPC(); }
    /// Given TPC time-tick (waveform index), returns time [us] w.r.t. beam gate time
    double TPCTick2BeamTime(double tick) const
    { return fTPCClock.TickPeriod() * tick + TriggerOffsetTPC() + TriggerTime() - BeamGateTime(); }
    /// Given Optical time-tick (waveform index), sample and frame number, returns time [us] w.r.t. trigger time stamp
    double OpticalTick2TrigTime(double tick, size_t sample, size_t frame) const
    { return fOpticalClock.TickPeriod() * tick + fOpticalClock.Time(sample,frame) - TriggerTime();  }
    /// Given Optical time-tick (waveform index), sample and frame number, returns time [us] w.r.t. beam gate time stamp
    double OpticalTick2BeamTime(double tick, size_t sample, size_t frame) const
    { return fOpticalClock.TickPeriod() * tick + fOpticalClock.Time(sample,frame) - BeamGateTime(); }
    /// Given External time-tick (waveform index), sample and frame number, returns time [us] w.r.t. trigger time stamp
    double ExternalTick2TrigTime(double tick, size_t sample, size_t frame) const
    { return fExternalClock.TickPeriod() * tick + fExternalClock.Time(sample,frame) - TriggerTime(); }
    /// Given External time-tick (waveform index), sample and frame number, returns time [us] w.r.t. beam gate time stamp
    double ExternalTick2BeamTime(double tick, size_t sample, size_t frame) const
    { return fExternalClock.TickPeriod() * tick + fExternalClock.Time(sample,frame) - BeamGateTime(); }
    
    //
    // Getters for time [tdc] (electronics clock counting ... in double precision) 
    //

    /// Given TPC time-tick (waveform index), returns electronics clock count [tdc]
    double TPCTick2TDC(double tick) const
    { return ( (TriggerTime() + TriggerOffsetTPC()) / fTPCClock.TickPeriod() + tick ); }
    /// Given G4 time [ns], returns corresponding TPC electronics clock count [tdc]
    double TPCG4Time2TDC(double g4time) const
    { return G4ToElecTime(g4time) / fTPCClock.TickPeriod(); }
    /// Given Optical time-tick (waveform index), sample and frame number, returns time electronics clock count [tdc]
    double OpticalTick2TDC(double tick, size_t sample, size_t frame) const
    { return fOpticalClock.Ticks(sample,frame) + tick; }
    /// Given G4 time [ns], returns corresponding Optical electronics clock count [tdc]  
    double OpticalG4Time2TDC(double g4time) const
    { return G4ToElecTime(g4time) / fOpticalClock.TickPeriod(); }
    /// Given External time-tick (waveform index), sample and frame number, returns time electronics clock count [tdc]
    double ExternalTick2TDC(double tick, size_t sample, size_t frame) const
    { return fExternalClock.Ticks(sample,frame) + tick; }
    /// Given G4 time [ns], returns corresponding External electronics clock count [tdc]
    double ExternalG4Time2TDC(double g4time) const
    { return G4ToElecTime(g4time) / fExternalClock.TickPeriod(); }

    //
    // Getters for time [us] (electronics clock counting ... in double precision)
    //
    /// Given TPC time-tick (waveform index), returns electronics clock [us]
    double TPCTick2Time(double tick) const
    { return TriggerTime() + TriggerOffsetTPC() + tick * fTPCClock.TickPeriod(); }
    /// Given Optical time-tick (waveform index), sample and frame number, returns electronics clock [us]
    double OpticalTick2Time(double tick, size_t sample, size_t frame) const
    { return fOpticalClock.Time(sample,frame) + tick * fOpticalClock.TickPeriod(); }
    /// Given External time-tick (waveform index), sample and frame number, returns electronics clock [us]
    double ExternalTick2Time(double tick, size_t sample, size_t frame) const
    { return fExternalClock.Time(sample,frame) + tick * fExternalClock.TickPeriod(); }

    //
    // Getters for time [ticks] (waveform index number)
    //

    /// Given electronics clock count [tdc] returns TPC time-tick
    double TPCTDC2Tick(double tdc) const
    { return ( tdc - (TriggerTime() + TriggerOffsetTPC()) / fTPCClock.TickPeriod() ); }
    /// Given G4 time returns electronics clock count [tdc]
    double TPCG4Time2Tick(double g4time) const
    { return (G4ToElecTime(g4time) - (TriggerTime() + TriggerOffsetTPC())) / fTPCClock.TickPeriod(); }


  };
  
}
#endif
/** @} */ // end of doxygen group 

