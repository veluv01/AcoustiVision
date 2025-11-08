#ifndef SPL_METER_H
#define SPL_METER_H

#include <cstdint>
#include "arm_math.h" // The master header for the ARM CMSIS-DSP library.

/**
 * @class SPL_Meter
 * @brief A class to encapsulate all DSP calculations for A-weighted SPL measurement.
 *
 * This class takes a buffer of raw ADC samples and runs a complete DSP pipeline
 * to compute a smoothed, A-weighted, and microphone-corrected SPL value in dBA.
 */
class SPL_Meter {
public:
  /**
   * @brief Constructor. Initializes member variables.
   */
  SPL_Meter();

  /**
   * @brief Initializes the DSP components (specifically, the FFT instance).
   * Must be called once from the main setup() function.
   */
  void begin();

  /**
   * @brief The main processing function. Runs the entire DSP pipeline on a buffer of samples.
   * @param buffer A pointer to the buffer of raw ADC samples.
   */
  void process(const uint32_t* buffer);

  /**
   * @brief Gets the latest smoothed, A-weighted SPL value.
   * @return The smoothed SPL value in decibels (dBA).
   */
  float getSmoothedDbaSpl() const;

private:
  // --- Constants and Configuration ---
  static constexpr uint32_t NUM_SAMPLES = 256;          // Buffer size for processing.
  static constexpr uint32_t SAMPLING_FREQUENCY = 16000; // Assumed audio sampling rate.
  static constexpr float ADC_REF_VOLTAGE = 3.3f;        // ADC reference voltage.
  static constexpr uint32_t ADC_RESOLUTION = 4096;      // 12-bit ADC resolution (2^12).
  // The 'alpha' for the EMA filter. A smaller value means more smoothing and a
  // slower response. 0.1 is a good starting point for a responsive but stable display.
  static constexpr float SMOOTHING_FACTOR = 0.1f;
  // This is the final tuning value. It should be adjusted after comparing the
  // output with a calibrated, professional sound level meter.
  static constexpr float CALIBRATION_OFFSET_DB = -30.0f;

  // --- Buffers and State Variables ---
  float m_latest_dba_spl;   // Stores the "raw" instantaneous dBA value.
  float m_smoothed_dba_spl; // Stores the final, smoothed dBA value for display.

  // --- CMSIS-DSP Members ---
  arm_rfft_fast_instance_f32 m_fft_instance; // Instance structure required by the CMSIS-DSP FFT functions.
  float32_t m_fft_input_buffer[NUM_SAMPLES];   // Buffer for windowed, time-domain data before the FFT.
  float32_t m_fft_output_buffer[NUM_SAMPLES];  // Buffer for the packed, complex, frequency-domain data after the FFT.
  float32_t m_mag_sq_buffer[NUM_SAMPLES / 2];  // Buffer to store the power spectrum (magnitude squared of each frequency bin).
};

#endif // SPL_METER_H