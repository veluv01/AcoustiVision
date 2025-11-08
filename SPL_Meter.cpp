#include "SPL_Meter.h"
#include <Arduino.h> // Required for standard Arduino functions like Serial and math.

// =============================================================================
// --- DSP LOOKUP TABLES (LUTs) ---
// Pre-calculating these values saves significant processing time in the main loop.
// These tables are specifically generated for:
// - NUM_SAMPLES = 256
// - SAMPLING_FREQUENCY = 16000 Hz
// If you change those constants, these tables MUST be recalculated.
// =============================================================================

// A Hann window is applied to the time-domain samples before the FFT. This
// tapers the signal at the beginning and end of the buffer, which significantly
// reduces an artifact called "spectral leakage" and results in a more accurate
// frequency spectrum. This table contains the 256 windowing coefficients.
static const float32_t HANN_WINDOW_LUT[256] = {
    0.00000, 0.00015, 0.00061, 0.00137, 0.00244, 0.00381, 0.00548, 0.00745, 0.00972, 0.01228,
    0.01513, 0.01827, 0.02169, 0.02539, 0.02937, 0.03362, 0.03813, 0.04290, 0.04793, 0.05320,
    0.05872, 0.06448, 0.07048, 0.07671, 0.08317, 0.08986, 0.09677, 0.10390, 0.11124, 0.11879,
    0.12655, 0.13451, 0.14266, 0.15099, 0.15951, 0.16821, 0.17709, 0.18613, 0.19534, 0.20471,
    0.21423, 0.22390, 0.23371, 0.24366, 0.25374, 0.26395, 0.27428, 0.28472, 0.29528, 0.30594,
    0.31670, 0.32756, 0.33850, 0.34954, 0.36066, 0.37186, 0.38313, 0.39447, 0.40588, 0.41734,
    0.42886, 0.44042, 0.45203, 0.46368, 0.47536, 0.48708, 0.49882, 0.51058, 0.52236, 0.53415,
    0.54595, 0.55775, 0.56956, 0.58136, 0.59315, 0.60493, 0.61670, 0.62844, 0.64017, 0.65187,
    0.66355, 0.67520, 0.68681, 0.69838, 0.70991, 0.72139, 0.73281, 0.74418, 0.75548, 0.76672,
    0.77789, 0.78898, 0.80000, 0.81093, 0.82177, 0.83251, 0.84315, 0.85368, 0.86410, 0.87440,
    0.88458, 0.89464, 0.90457, 0.91436, 0.92402, 0.93354, 0.94291, 0.95212, 0.96116, 0.97004,
    0.97874, 0.98726, 0.99560, 1.00000, 0.99560, 0.98726, 0.97874, 0.97004, 0.96116, 0.95212,
    0.94291, 0.93354, 0.92402, 0.91436, 0.90457, 0.89464, 0.88458, 0.87440, 0.86410, 0.85368,
    0.84315, 0.83251, 0.82177, 0.81093, 0.80000, 0.78898, 0.77789, 0.76672, 0.75548, 0.74418,
    0.73281, 0.72139, 0.70991, 0.69838, 0.68681, 0.67520, 0.66355, 0.65187, 0.64017, 0.62844,
    0.61670, 0.60493, 0.59315, 0.58136, 0.56956, 0.55775, 0.54595, 0.53415, 0.52236, 0.51058,
    0.49882, 0.48708, 0.47536, 0.46368, 0.45203, 0.44042, 0.42886, 0.41734, 0.40588, 0.39447,
    0.38313, 0.37186, 0.36066, 0.34954, 0.33850, 0.32756, 0.31670, 0.30594, 0.29528, 0.28472,
    0.27428, 0.26395, 0.25374, 0.24366, 0.23371, 0.22390, 0.21423, 0.20471, 0.19534, 0.18613,
    0.17709, 0.16821, 0.15951, 0.15099, 0.14266, 0.13451, 0.12655, 0.11879, 0.11124, 0.10390,
    0.09677, 0.08986, 0.08317, 0.07671, 0.07048, 0.06448, 0.05872, 0.05320, 0.04793, 0.04290,
    0.03813, 0.03362, 0.02937, 0.02539, 0.02169, 0.01827, 0.01513, 0.01228, 0.00972, 0.00745,
    0.00548, 0.00381, 0.00244, 0.00137, 0.00061, 0.00015
};
// These tables contain the correction factors that will be applied to the
// power spectrum (the output of the FFT). They have been PRE-SQUARED because
// we apply them to the magnitude-squared values, saving a multiplication
// in the main processing loop.
// The A-Weighting curve mimics the frequency response of the human ear.
static const float32_t A_WEIGHTING_LUT_SQUARED[128] = {
    0.1287, 0.2120, 0.3162, 0.4365, 0.5623, 0.6813, 0.7852, 0.8692, 0.9333, 0.9772,
    1.0000, 1.0000, 0.9853, 0.9661, 0.9333, 0.8913, 0.8414, 0.7852, 0.7244, 0.6607,
    0.5957, 0.5309, 0.4677, 0.4074, 0.3548, 0.3020, 0.2512, 0.2000, 0.1585, 0.1259,
    0.1000, 0.0794, 0.0631, 0.0501, 0.0400, 0.0316, 0.0251, 0.0200, 0.0158, 0.0126,
    0.0100, 0.0079, 0.0063, 0.0050, 0.0040, 0.0032, 0.0025, 0.0020, 0.0016, 0.0013,
    0.0010, 0.0008, 0.0006, 0.0005, 0.0004, 0.0003, 0.0003, 0.0002, 0.0002, 0.0001,
    0.0001, 0.0001, 0.0001, 0.0001, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000
};
// The Mic Correction curve compensates for the microphone's own non-flat frequency response.
static const float32_t MIC_CORRECTION_LUT_SQUARED[128] = {
    1.1834, 1.1220, 1.0965, 1.0715, 1.0471, 1.0233, 1.0000, 0.9886, 0.9772, 0.9772,
    0.9772, 0.9886, 0.9886, 0.9886, 1.0000, 1.0000, 1.0000, 1.0000, 0.9886, 0.9886,
    0.9886, 0.9772, 0.9772, 0.9772, 0.9661, 0.9661, 0.9550, 0.9550, 0.9441, 0.9441,
    0.9333, 0.9333, 0.9226, 0.9226, 0.9120, 0.9120, 0.9016, 0.9016, 0.8913, 0.8913,
    0.8810, 0.8810, 0.8710, 0.8710, 0.8610, 0.8610, 0.8511, 0.8511, 0.8414, 0.8414,
    0.8318, 0.8318, 0.8222, 0.8222, 0.8128, 0.8128, 0.8035, 0.8035, 0.7943, 0.7943,
    0.7852, 0.7852, 0.7762, 0.7762, 0.7674, 0.7674, 0.7586, 0.7586, 0.7499, 0.7499,
    0.7413, 0.7413, 0.7328, 0.7328, 0.7244, 0.7244, 0.7161, 0.7161, 0.7079, 0.7079,
    0.6998, 0.6998, 0.6918, 0.6918, 0.6839, 0.6839, 0.6761, 0.6761, 0.6683, 0.6683,
    0.6607, 0.6607, 0.6531, 0.6531, 0.6457, 0.6457, 0.6383, 0.6383, 0.6310, 0.6310,
    0.6237, 0.6237, 0.6166, 0.6166, 0.6095, 0.6095, 0.6026, 0.6026, 0.5957, 0.5957,
    0.5888, 0.5888, 0.5821, 0.5821, 0.5754, 0.5754, 0.5688, 0.5688, 0.5623, 0.5623,
    0.5559, 0.5559, 0.5495, 0.5495, 0.5433, 0.5433, 0.5370, 0.5370
};
/**
 * @brief Constructor. Initializes member variables to a known state.
 */
SPL_Meter::SPL_Meter() :
  m_latest_dba_spl(0.0f),
  m_smoothed_dba_spl(0.0f) // Start the smoothed value at 0.
{
}

/**
 * @brief Initializes the CMSIS-DSP Fast Fourier Transform instance.
 */
void SPL_Meter::begin()
{
  // This function prepares the CMSIS-DSP library by providing it with the FFT size.
  // It pre-calculates twiddle factors and other data needed by the FFT algorithm.
  arm_rfft_fast_init_f32(&m_fft_instance, NUM_SAMPLES);
  Serial.println("SPL_Meter DSP (Optimized, 256 samples) initialized.");
}

/**
 * @brief Returns the latest smoothed dBA SPL value.
 */
float SPL_Meter::getSmoothedDbaSpl() const
{
  return m_smoothed_dba_spl;
}

/**
 * @brief Runs the complete DSP pipeline on a buffer of audio samples.
 */
void SPL_Meter::process(const uint32_t* raw_buffer)
{
    // --- Step 1: Calculate Dynamic DC Offset ---
    // We calculate the average of the buffer to find its center-point or "DC offset".
    // This is more robust than a fixed constant, as it adapts to minor hardware fluctuations.
    uint32_t sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      sum += raw_buffer[i];
    }
    float dynamic_dc_offset = (float)sum / NUM_SAMPLES;

    // --- Step 2: Prepare Samples & Apply Window ---
    // This loop prepares the data for the FFT. For each sample, it:
    // 1. Removes the DC offset to get a pure AC waveform centered around zero.
    // 2. Multiplies by the Hann window coefficient to shape the signal.
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        float32_t sample = (float32_t)raw_buffer[i] - dynamic_dc_offset;
        m_fft_input_buffer[i] = sample * HANN_WINDOW_LUT[i];
    }

    // --- Step 3: Perform the Fast Fourier Transform (FFT) ---
    // This is the core of the frequency analysis. It transforms our 256 time-domain
    // samples into 128 "bins" of frequency-domain data.
    arm_rfft_fast_f32(&m_fft_instance, m_fft_input_buffer, m_fft_output_buffer, 0);

    // --- Step 4, 5, 6 (OPTIMIZED): Calculate Final Weighted Energy ---
    // This single, efficient loop performs three tasks at once to maximize performance:
    // 1. Calculates the power (magnitude squared) of each frequency bin.
    // 2. Applies the A-Weighting and Microphone Correction factors.
    // 3. Sums the final, weighted energy of all bins.
    float32_t total_energy = 0.0f;
    for (uint32_t i = 1; i < (NUM_SAMPLES / 2); i++) { // Start at bin 1 to ignore the DC component.
        float32_t real = m_fft_output_buffer[2 * i];
        float32_t imag = m_fft_output_buffer[2 * i + 1];
        float32_t mag_sq = (real * real) + (imag * imag);
        float32_t weighted_mag_sq = mag_sq * A_WEIGHTING_LUT_SQUARED[i] * MIC_CORRECTION_LUT_SQUARED[i];
        total_energy += weighted_mag_sq;
    }

    // --- Step 7: Convert Final Energy to dBA SPL ---
    // This sequence of mathematical conversions transforms the abstract 'total_energy'
    // value into a physical, meaningful decibel reading based on the microphone's known sensitivity.
    if (total_energy <= 0.0f) {
        m_latest_dba_spl = 0.0f; // Avoid math errors with log(0).
    } else {
        float32_t mean_sq_adc = (total_energy * 2.0f) / (NUM_SAMPLES * NUM_SAMPLES);
        float32_t rms_adc = sqrtf(mean_sq_adc);
        float32_t rms_voltage = (rms_adc / ADC_RESOLUTION) * ADC_REF_VOLTAGE;
        float32_t sensitivity_V_Pa = powf(10.0f, -38.0f / 20.0f); // Convert -38 dBV/Pa to linear V/Pa
        float32_t pressure_Pa = rms_voltage / sensitivity_V_Pa;
        float32_t spl = 20.0f * log10f(pressure_Pa / 20e-6f); // Convert Pascals to dB SPL (re: 20 uPa)
        m_latest_dba_spl = spl + CALIBRATION_OFFSET_DB;
    }

    // --- Step 8: Apply Smoothing Filter ---
    // The Exponential Moving Average (EMA) filter smooths the output for a stable,
    // readable display. It blends the new reading with the previous smoothed reading.
    m_smoothed_dba_spl = (SMOOTHING_FACTOR * m_latest_dba_spl) + ((1.0f - SMOOTHING_FACTOR) * m_smoothed_dba_spl);
}