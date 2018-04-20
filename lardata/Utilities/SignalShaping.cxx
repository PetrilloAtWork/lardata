//////////////////////////////////////////////////////////////////////
///
/// \file   SignalShaping.cxx
///
/// \brief  Generic signal shaping class.
///
/// \author H. Greenlee 
///
////////////////////////////////////////////////////////////////////////

#include "cetlib/exception.h"
#include "lardata/Utilities/SignalShaping.h"
#include <cmath>
#include <algorithm>


//----------------------------------------------------------------------
// Constructor.
//
util::SignalShaping::SignalShaping()
  : fMinConvKernelFrac(1.e-6)
  , fResponseLocked(false)
  , fFilterLocked  (false)
  , fNorm (true)
{}


//----------------------------------------------------------------------
// Destructor.
//
util::SignalShaping::~SignalShaping()
{}

// void util::SignalShaping::ResetDecon()
// {
//   fResponseLocked = false;
//   fFilterLocked = false;
//   fResponse.clear();
//   fConvKernel.clear();
//   fFilter.clear();
//   fDeconvKernel.clear();
//   //Set deconvolution polarity to + as default
//   fDeconvKernelPolarity = +1;
// }


//----------------------------------------------------------------------
// Reset this class to its default-constructed state.
void util::SignalShaping::Reset()
{
    fResponseLocked = false;
    fFilterLocked = false;
    fResponse.clear();
    fConvKernel.clear();
    fFilter.clear();
    fDeconvKernel.clear();
    //Set deconvolution polarity to + as default
    fDeconvKernelPolarity = +1;
}


//----------------------------------------------------------------------
// Add a time domain response function.
void util::SignalShaping::AddResponseFunction(const std::vector<double>& resp, bool ResetResponse )
{
    // Make sure configuration is not locked.
    if(fResponseLocked) throw cet::exception("SignalShaping") << "Configuration locked.\n";

    // Get FFT service.
    art::ServiceHandle<util::LArFFT> fft;
  
    int nticks = fft->FFTSize();

    // Copy new response function into fResponse attribute, and pad or
    // truncate to correct size.
    fResponse = resp;
    fResponse.resize(nticks, 0.);

    // Is this the first response function?
    if ( fConvKernel.size() == 0 || ResetResponse )
    {
        // This is the first response function.
        // Just calculate the fourier transform.
        fConvKernel.resize(nticks/2 + 1);
        fft->DoFFT(fResponse, fConvKernel);
    }
    else
    {
        // Not the first response function.
        // Calculate the fourier transform of new response function.
        std::vector<TComplex> kern(nticks/2 + 1);
        fft->DoFFT(fResponse, kern);

        // Update overall convolution kernel.
        if (kern.size() != fConvKernel.size())
            throw cet::exception("SignalShaping") << __func__ << ": inconsistent kernel size, " << kern.size() << " vs. " << fConvKernel.size();

        for(unsigned int i=0; i<kern.size(); ++i) fConvKernel[i] *= kern[i];

        // Recalculate overall response function.
        fft->DoInvFFT(fConvKernel, fResponse);
    }
}


//----------------------------------------------------------------------
// Shift the response function and convolution kernel by the specified
// number of ticks.
void util::SignalShaping::ShiftResponseTime(double ticks)
{
    // Make sure configuration is not locked.
    if(fResponseLocked) throw cet::exception("SignalShaping") << "Configuration locked.\n";

    // Get FFT service.
    art::ServiceHandle<util::LArFFT> fft;

    // Update convolution kernel.
    fft->ShiftData(fConvKernel, ticks);

    // Recalculate overall response functiion.
    fft->DoInvFFT(fConvKernel, fResponse);
}


//----------------------------------------------------------------------
// Set the peak response time to be at the specified tick.
void util::SignalShaping::SetPeakResponseTime(double tick)
{
    // Make sure configuration is not locked.
    if(fResponseLocked) throw cet::exception("SignalShaping") << "Configuration locked.\n";

    // Get FFT service.
    art::ServiceHandle<util::LArFFT> fft;

    // Construct a delta-function response centered at tick zero.
    std::vector<double> delta(fft->FFTSize(), 0.);
    delta[0] = 1.;

    // Figure out peak of current overall response.
    double peak = fft->PeakCorrelation(delta, fResponse);

    // Shift peak response to desired tick.
    ShiftResponseTime(tick - peak);
}


//----------------------------------------------------------------------
// Add a frequency domain filter function to cumulative filter function.
void util::SignalShaping::AddFilterFunction(const std::vector<TComplex>& filt)
{
    // Make sure configuration is not locked.
    if(fFilterLocked) throw cet::exception("SignalShaping") << "Configuration locked.\n";

    // Get FFT service.
    art::ServiceHandle<util::LArFFT> fft;

    // If this is the first filter function, just copy the filter function.
    // Otherwise, update the overall filter function.
    if(fFilter.size() == 0)
    {
        fFilter = filt;
        fFilter.resize(fft->FFTSize() / 2 + 1);
    }
    else
    {
        unsigned int n = std::min(fFilter.size(), filt.size());
        for(unsigned int i=0; i<n; ++i) fFilter[i] *= filt[i];
        for(unsigned int i=n; i<fFilter.size(); ++i) fFilter[i] = 0.;
    }
}

//----------------------------------------------------------------------
// Add a DeconvKernel Polarity Flag to decide how to normalize
void util::SignalShaping::SetDeconvKernelPolarity(int pol)
{
    if ( (pol != 1) and (pol != -1) ) {
        throw cet::exception("SignalShaping") << __func__ << ": DeconvKernelPolarity should be +1 or -1 (got " << pol << "). Setting to +1\n";
        fDeconvKernelPolarity = +1;
    }
    else fDeconvKernelPolarity = pol;
}


//----------------------------------------------------------------------
// Test and lock the response and convolution kernel.
void util::SignalShaping::LockResponse() const
{
    // Do nothing if the response is already locked.
    if(!fResponseLocked)
    {
        // Get FFT service.
        art::ServiceHandle<util::LArFFT> fft;

        // Make sure response has been configured.
        if(fResponse.size() == 0) throw cet::exception("SignalShaping") << "Response has not been configured.\n";

        // Make sure response and convolution kernel have the correct
        // size (should always be the case if we get here).
        unsigned int n = fft->FFTSize();
    
        if (fResponse.size() != n)
            throw cet::exception("SignalShaping") << __func__ << ": inconsistent kernel size, " << fResponse.size() << " vs. " << n << "\n";
    
        if (2 * (fConvKernel.size() - 1) != n)
            throw cet::exception("SignalShaping") << __func__ << ": unexpected FFT size, " << n << " vs. expected " << (2 * (fConvKernel.size() - 1)) << "\n";

        // Set the lock flag.
        fResponseLocked = true;
    }
}


//----------------------------------------------------------------------
// Calculate the deconvolution kernel as the ratio
// of the filter function and convolution kernel.
void util::SignalShaping::CalculateDeconvKernel() const
{
    // Make sure configuration is not locked.
    if(fFilterLocked) throw cet::exception("SignalShaping") << "Configuration locked.\n";

    // Lock response configuration.
    LockResponse();

    // Get FFT service.
    art::ServiceHandle<util::LArFFT> fft;
    
    // Make sure filter function has been configured.
    if(fFilter.size() == 0)
        throw cet::exception("SignalShaping") << "Filter function has not been configured.\n";

    // Make sure filter function has the correct size.
    // (Should always be the case if we get here.)
    if (fFilter.size() != fConvKernel.size())
    {
        throw cet::exception("SignalShaping") << __func__ << ": inconsistent size, "
                << fFilter.size() << " vs. " << fConvKernel.size() << "\n";
    }
  
    // Note that to avoid crazy large values in the range where the convolution kernel tends to zero
    // we need to recover the peak value which we can use to determine when we are near the end
    TComplex convComplex    = fConvKernel.at(0);
    TComplex convMaxComplex = *std::max_element(fConvKernel.begin(),fConvKernel.end(),[](const auto& a, const auto& b){return a.Rho() < b.Rho();});
    double   convMax        = convMaxComplex.Rho();
    
    // Now form the deconvolution kernel which will be the ratio of the filter function and
    // the convolution kernel but where we suppress those regions where the convolution kernel
    // is below our cutoff
    fDeconvKernel = fFilter;
    for(unsigned int idx=0; idx<fDeconvKernel.size(); ++idx)
    {
        // The idea is to use the latest "valid" value of the response function
        // At some point it is assumed this will start to remain constant, at this point
        // it is assumed that the filter will be tending to zero and will squash this
        // constant value but will do so in a "continuous" way.
        if (fConvKernel[idx].Rho() > fMinConvKernelFrac * convMax) convComplex = fConvKernel[idx];
        
        fDeconvKernel[idx] /= convComplex;
    }

    // Normalize the deconvolution kernel.
    if (fNorm)
    {
        // Calculate the unnormalized deconvoluted response
        // (inverse FFT of filter function).
        std::vector<double> deconv(fFilter.size(), 0.);
        fft->DoInvFFT(const_cast<std::vector<TComplex>&>(fFilter), deconv);
        
        // Find the peak value of the response
        // Should normally be at zero, but don't assume that.
        // Use DeconvKernelPolairty to find what to normalize to
        double peak_response = 0;
        
        std::pair<std::vector<double>::const_iterator,std::vector<double>::const_iterator> minMaxPair = std::minmax_element(fResponse.begin(),fResponse.end());
        
        if (fDeconvKernelPolarity > 0) peak_response =  *minMaxPair.second;
        else                           peak_response = -*minMaxPair.first;
    
        if (peak_response <= 0.)
        {
            throw cet::exception("SignalShaping") << __func__ << ": peak should always be positive (got " << peak_response << ")\n";
        }
    
        // Find the peak value of the deconvoluted response
        // Should normally be at zero, but don't assume that.
        double peak_deconv = *std::max_element(deconv.begin(),deconv.end());
        
        if (peak_deconv <= 0.)
        {
            throw cet::exception("SignalShaping") << __func__ << ": deconvolution peak should always be positive (got " << peak_deconv << ")\n";
        }
    
        // Multiply the deconvolution kernel by a factor such that
        // (Peak of response) = (Peak of deconvoluted response).
        double ratio = peak_response / peak_deconv;
        std::transform(fDeconvKernel.begin(),fDeconvKernel.end(),fDeconvKernel.begin(),std::bind(std::multiplies<double>(),std::placeholders::_1,ratio));
    }
    // Set the lock flag.

    fFilterLocked = true;
}
