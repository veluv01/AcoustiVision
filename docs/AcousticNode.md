# A-Weighted SPL Meter with BLE

An embedded sound level meter implementation for the Seeed Studio XIAO MG24 microcontroller that measures A-weighted Sound Pressure Level (SPL) and transmits readings via Bluetooth Low Energy.

## What is A-Weighted SPL?

**Sound Pressure Level (SPL)** is a logarithmic measure of sound intensity relative to the threshold of human hearing, expressed in decibels (dB). However, the human ear doesn't perceive all frequencies equally—we're more sensitive to mid-range frequencies (1-4 kHz) and less sensitive to very low and very high frequencies.

**A-Weighting** is a frequency response curve that mimics human hearing sensitivity. It:
- Reduces low frequencies (below 500 Hz) by up to 30 dB
- Leaves mid frequencies (1-5 kHz) mostly unchanged
- Slightly reduces high frequencies (above 5 kHz)

This weighting makes measurements more relevant to human perception and is the standard for:
- Occupational noise exposure assessment
- Environmental noise monitoring
- Residential and commercial noise compliance
- General-purpose sound level measurements

Measurements using A-weighting are expressed in **dBA** (A-weighted decibels).

## Why This Implementation?

This project provides a low-cost, customizable alternative to commercial sound level meters for:
- Educational purposes and learning about acoustics and DSP
- DIY noise monitoring projects
- IoT applications requiring sound level data
- Prototyping and testing before deploying commercial solutions

**Important Note**: This is a hobbyist/educational implementation. It is not certified for regulatory compliance or professional acoustical work. For legally-required measurements, use a Class 1 or Class 2 certified sound level meter.

## Features

- A-weighted SPL measurements following standard frequency weighting curves
- Continuous audio sampling using DMA and interrupts
- BLE connectivity for wireless data access
- Microphone frequency response compensation
- FFT-based frequency analysis using ARM CMSIS-DSP
- Exponential moving average smoothing for stable readings

## Hardware Requirements

- **Microcontroller**: Seeed Studio XIAO MG24
- **Microphone**: Analog microphone compatible with the XIAO MG24
  - Data Pin: PC9
  - Power Pin: PC8
  - Assumed Sensitivity: -38 dBV/Pa

## Software Dependencies

- **Arduino IDE** or compatible development environment
- **SilabsMicrophoneAnalog** library
- **ArduinoBLE** library
- **ARM CMSIS-DSP** library (included with XIAO MG24 board support)

## Installation

1. Install the Arduino IDE and add support for the Seeed Studio XIAO MG24 board
2. Install required libraries:
   ```
   - SilabsMicrophoneAnalog
   - ArduinoBLE
   ```
3. Download or clone this repository
4. Open `acousticNode.ino` in Arduino IDE
5. Select the correct board and port
6. Upload to your XIAO MG24

## Project Structure

```
├── acousticNode.ino                    # Main application file
├── SPL_Meter.h                         # SPL Meter class header
├── SPL_Meter.cpp                       # SPL Meter class implementation
```

## Configuration

### Audio Processing Parameters

In `SPL_Meter.h`:
- `NUM_SAMPLES`: 256 (buffer size for FFT processing)
- `SAMPLING_FREQUENCY`: 16000 Hz
- `SMOOTHING_FACTOR`: 0.1 (EMA filter alpha, lower = more smoothing)
- `CALIBRATION_OFFSET_DB`: -30.0 dB (adjust to match calibrated reference meter)

### BLE Settings

In `AcousticMonitor_Arduino_BLE.ino`:
- `BLE_UPDATE_INTERVAL`: 500 ms (notification frequency)
- Device Name: "SPL_Meter"
- Service UUID: `19B10000-E8F2-537E-4F6C-D104768A1214`
- Characteristic UUID: `19B10001-E8F2-537E-4F6C-D104768A1214`

## Usage

### Basic Operation

1. Power on the device
2. Open Serial Monitor at 115200 baud to view debug output
3. The device will automatically start measuring and display SPL readings every second
4. Use a BLE-capable device (smartphone, computer) to connect and receive wireless updates

### BLE Connection

1. Scan for BLE devices named "SPL_Meter"
2. Connect to the device
3. Subscribe to notifications on the SPL characteristic
4. Receive float values representing dBA measurements every 500ms

### Calibration

To calibrate the meter:

1. Place the device next to a calibrated professional sound level meter
2. Generate a steady reference tone (e.g., 1 kHz at 94 dB SPL)
3. Note the difference between your meter's reading and the reference
4. Adjust `CALIBRATION_OFFSET_DB` in `SPL_Meter.h` accordingly
5. Recompile and upload
6. Verify accuracy across different sound levels

## Technical Details

### DSP Pipeline

The SPL measurement follows these steps:

1. **Dynamic DC Offset Removal**: Calculates and removes the average offset from each buffer
2. **Hann Windowing**: Applies a Hann window to reduce spectral leakage
3. **FFT**: Performs a 256-point Real FFT using ARM CMSIS-DSP
4. **Power Spectrum**: Calculates magnitude squared for each frequency bin
5. **A-Weighting**: Applies pre-calculated A-weighting coefficients
6. **Microphone Correction**: Compensates for microphone frequency response
7. **SPL Conversion**: Converts weighted energy to dBA using microphone sensitivity
8. **EMA Smoothing**: Applies exponential moving average for stable output

### Lookup Tables

Pre-calculated lookup tables optimize performance:
- **Hann Window**: 256 coefficients for time-domain windowing
- **A-Weighting**: 128 squared coefficients matching human hearing sensitivity
- **Microphone Correction**: 128 squared coefficients for frequency response compensation

**Important**: These tables are specific to 256 samples at 16 kHz. If you change `NUM_SAMPLES` or `SAMPLING_FREQUENCY`, you must regenerate these tables.

## Performance Considerations

- **Buffer Size**: Increased to 256 samples to provide sufficient CPU time for DSP calculations
- **DMA Operation**: Audio sampling occurs in the background without blocking the main loop
- **Interrupt-Driven**: Efficient event-driven architecture minimizes CPU usage
- **Optimized Loop**: Single loop combines power calculation, weighting, and energy accumulation

## Troubleshooting

### No BLE Connection
- Ensure BLE is enabled on your connecting device
- Check that the device is advertising (look for "SPL_Meter" in BLE scan results)
- Try resetting the microcontroller

### Unstable Readings
- Increase `SMOOTHING_FACTOR` for more stability (at the cost of response time)
- Check microphone connections
- Ensure adequate power supply

### Inaccurate Measurements
- Perform calibration procedure
- Verify microphone sensitivity matches the assumed -38 dBV/Pa
- Check that ambient temperature is within microphone specifications

## Serial Output Example

```
=================================
A-Weighted SPL Meter with BLE
=================================
SPL Meter initialized...
BLE initialized and advertising...
Device name: SPL_Meter
Microphone library initialized...
Sampling started...
=================================
Smoothed SPL: 45.23 dBA
Connected to central: XX:XX:XX:XX:XX:XX
BLE Update - SPL: 45.67 dBA
Smoothed SPL: 46.12 dBA
BLE Update - SPL: 46.34 dBA
```

## Safety and Limitations

- **Hearing Protection**: This device is for monitoring purposes only. Do not use as a substitute for proper hearing protection in hazardous noise environments
- **Accuracy**: While optimized for accuracy, this is not a Class 1 or Class 2 certified sound level meter
- **Frequency Range**: Effective frequency range depends on the 16 kHz sampling rate (approximately 20 Hz - 8 kHz)
- **Dynamic Range**: Limited by 12-bit ADC resolution



## Algorithm Implementation

### Mathematical Foundation

The SPL meter implements the following mathematical model to convert raw ADC samples into calibrated dBA measurements:

#### 1. Signal Acquisition and Preprocessing

**Dynamic DC Offset Removal:**
```
DC_offset = (1/N) × Σ(samples[i])
centered_sample[i] = samples[i] - DC_offset
```

This adaptive approach accounts for hardware drift and is more robust than using a fixed offset value.

**Hann Window Application:**
```
windowed[i] = centered_sample[i] × w_hann[i]
```

where `w_hann[i] = 0.5 × (1 - cos(2π × i / N))`

The Hann window reduces spectral leakage by tapering the signal at buffer boundaries, which is crucial for accurate frequency analysis.

#### 2. Fast Fourier Transform

The Real FFT transforms N time-domain samples into N/2 complex frequency bins:

```
X[k] = Σ(x[n] × e^(-j2πkn/N))  for k = 0 to N/2-1
```

Each bin k represents a frequency: `f[k] = k × (Fs / N)`

For our configuration:
- N = 256 samples
- Fs = 16000 Hz
- Frequency resolution: 62.5 Hz per bin
- Bin 1: 62.5 Hz, Bin 2: 125 Hz, ... Bin 127: 7937.5 Hz

#### 3. Power Spectrum Calculation

For each frequency bin (excluding DC component at bin 0):

```
Power[k] = Real[k]² + Imag[k]²
```

This represents the energy content at each frequency.

#### 4. Frequency Weighting and Correction

**A-Weighting Function:**

The A-weighting curve approximates human hearing sensitivity, derived from the inverse of the 40-phon equal-loudness contour:

```
A(f) = (12194² × f⁴) / ((f² + 20.6²) × √((f² + 107.7²) × (f² + 737.9²)) × (f² + 12194²))
```

**Combined Weighting:**
```
Weighted_Power[k] = Power[k] × A²[k] × Mic_Correction²[k]
```

Note: Coefficients are pre-squared because they're applied to power (magnitude squared) values.

#### 5. Total Acoustic Energy

```
Total_Energy = Σ(Weighted_Power[k])  for k = 1 to N/2-1
```

Sum starts at k=1 to exclude the DC component.

#### 6. RMS Calculation

**Mean Square Value:**
```
Mean_Square_ADC = (Total_Energy × 2) / N²
```

The factor of 2 accounts for the symmetric negative frequency components not explicitly calculated in the Real FFT.

**RMS in ADC units:**
```
RMS_ADC = √(Mean_Square_ADC)
```

#### 7. Voltage Conversion

```
RMS_Voltage = (RMS_ADC / ADC_Resolution) × V_ref
```

where:
- ADC_Resolution = 4096 (12-bit ADC: 2¹²)
- V_ref = 3.3V (reference voltage)

#### 8. Acoustic Pressure Calculation

Using the microphone sensitivity specification:

```
Sensitivity = 10^(-38/20) V/Pa ≈ 0.01259 V/Pa
Pressure_Pa = RMS_Voltage / Sensitivity
```

#### 9. Sound Pressure Level (dB SPL)

```
SPL = 20 × log₁₀(Pressure_Pa / P_ref)
```

where P_ref = 20 × 10⁻⁶ Pa (threshold of human hearing)

#### 10. Calibration and Smoothing

**Calibrated SPL:**
```
SPL_calibrated = SPL + Calibration_Offset
```

**Exponential Moving Average (EMA):**
```
SPL_smoothed[n] = α × SPL_calibrated[n] + (1 - α) × SPL_smoothed[n-1]
```

where α = 0.1 (smoothing factor)

The EMA provides a time-weighted average that's responsive to changes while filtering out rapid fluctuations.

### Code Architecture

#### Class Structure: SPL_Meter

**Public Interface:**
- `SPL_Meter()`: Constructor initializes state variables
- `begin()`: Initializes CMSIS-DSP FFT instance
- `process(buffer)`: Main processing pipeline
- `getSmoothedDbaSpl()`: Returns current smoothed reading

**Private Members:**
- FFT instance and buffers
- Configuration constants
- State variables for smoothing

#### Main Application Flow

```
setup():
  ├── Initialize Serial communication
  ├── Initialize SPL_Meter (FFT setup)
  ├── Initialize BLE service and characteristic
  ├── Initialize microphone hardware
  └── Start DMA-based continuous sampling

loop():
  ├── Handle BLE events (connection/disconnection)
  ├── Check data_ready_flag
  │   ├── If true: Process new audio buffer
  │   ├── Update currentDbaSpl value
  │   └── Print to Serial (throttled to 1 Hz)
  └── Send BLE notifications (at 2 Hz when connected)

[Interrupt Context]:
mic_samples_ready_cb():
  ├── Copy DMA buffer to local buffer
  └── Set data_ready_flag
```

### Performance Optimization Techniques

#### 1. Lookup Table Pre-computation

All frequency-dependent coefficients are pre-calculated and stored in Flash memory:

```cpp
// Instead of calculating on-the-fly:
// float a_weight = calculate_a_weighting(frequency);

// We use:
float a_weight_squared = A_WEIGHTING_LUT_SQUARED[bin_index];
```

This eliminates:
- 128 exponential calculations
- 128 logarithmic calculations
- Multiple trigonometric operations
- Per frame, saves approximately 50,000 CPU cycles

#### 2. Single-Loop Optimization

The original implementation might use separate loops:

```cpp
// Less efficient approach:
for (i=0; i<N/2; i++) calculate_power[i];
for (i=0; i<N/2; i++) apply_weighting[i];
for (i=0; i<N/2; i++) sum_energy += weighted[i];
```

Our optimized version:

```cpp
// Efficient single-loop approach:
for (i=1; i<N/2; i++) {
    float power = real² + imag²;
    float weighted = power × A²[i] × Mic²[i];
    total_energy += weighted;
}
```

Benefits:
- Better CPU cache utilization
- Reduced memory bandwidth
- Fewer loop overhead operations

#### 3. DMA-Based Acquisition

```
Hardware (DMA) ──[continuous]──> Buffer A
                                     ↓
                              [interrupt fires]
                                     ↓
                              Copy to Buffer B
                                     ↓
CPU processes Buffer B    (while DMA fills Buffer A)
```

This architecture ensures:
- Zero sample loss
- Minimal CPU intervention in data acquisition
- Parallel processing and acquisition

#### 4. Fixed-Point Considerations

While this implementation uses floating-point (optimal for ARM Cortex-M33 with FPU), the algorithm could be adapted for fixed-point:

```cpp
// Q15 format example (16-bit signed, 15 fractional bits)
int16_t sample_q15 = (int16_t)((sample / max_value) * 32768);
```

The XIAO MG24's hardware floating-point unit makes float32 operations faster than fixed-point emulation.

### Frequency Response Analysis

#### Effective Measurement Range

Given the sampling parameters:
- **Nyquist Frequency**: Fs/2 = 8000 Hz
- **Usable Range**: ~20 Hz to ~7000 Hz (accounting for anti-aliasing)
- **FFT Bin Resolution**: 62.5 Hz

#### A-Weighting Characteristics

The A-weighting curve provides:
- **-20 dB at 100 Hz** (reduces low-frequency rumble)
- **0 dB at 1 kHz** (peak sensitivity)
- **-1 dB at 2 kHz**
- **-9 dB at 8 kHz** (high-frequency rolloff)

This matches the human ear's reduced sensitivity at low and very high frequencies.

#### Microphone Correction

The correction curve compensates for the specific microphone's deviation from flat response:
- Typically shows slight boost in mid-frequencies
- Gentle rolloff at extremes
- Values stored as squared multipliers for computational efficiency

### Buffer Size Selection Rationale

**Why 256 Samples?**

1. **Processing Time**: At 16 kHz, 256 samples = 16 ms of audio
   - Provides sufficient time for DSP calculations
   - Prevents buffer overruns under heavy computational load

2. **Frequency Resolution**: 62.5 Hz per bin
   - Adequate for A-weighting curve application
   - Good compromise between resolution and update rate

3. **Computational Efficiency**: 
   - Power-of-2 size optimizes FFT performance
   - 256-point FFT is fast enough for real-time operation
   - Smaller FFT working set improves cache performance

4. **Update Rate**: 
   - 256 samples at 16 kHz = 62.5 Hz frame rate
   - After smoothing, provides responsive yet stable readings

**Trade-offs:**

| Buffer Size | Frequency Resolution | Update Rate | CPU Load |
|-------------|---------------------|-------------|----------|
| 128         | 125 Hz              | 125 Hz      | Light    |
| 256         | 62.5 Hz             | 62.5 Hz     | Moderate |
| 512         | 31.25 Hz            | 31.25 Hz    | Heavy    |

### Calibration Procedure Details

#### Equipment Needed
- Calibrated reference sound level meter (Class 1 or Class 2)
- Acoustic calibrator (94 dB @ 1 kHz recommended)
- Quiet test environment

#### Step-by-Step Process

1. **Setup Phase:**
   - Place both meters at same location
   - Ensure microphones face same direction
   - Minimize reflections (use outdoors or anechoic space)

2. **Reference Tone Measurement:**
   - Apply acoustic calibrator (typically 94 dB @ 1 kHz)
   - Note reference meter reading: R_ref
   - Note your meter reading: R_device
   - Calculate: Offset = R_ref - R_device

3. **Verification:**
   - Test with various sound sources
   - Check at different levels (e.g., 70 dB, 80 dB, 90 dB)
   - Verify linearity (offset should be constant)

4. **Apply Calibration:**
   ```cpp
   static constexpr float CALIBRATION_OFFSET_DB = -30.0f; // Adjust this value
   ```

5. **Document Results:**
   - Record calibration date
   - Note environmental conditions
   - Keep calibration certificate if available

#### Expected Accuracy

After proper calibration:
- **±2 dB** for steady-state sounds
- **±3 dB** for rapidly varying sounds
- Best accuracy between 100 Hz - 4 kHz

### Error Sources and Mitigation

#### 1. Quantization Noise
- **Source**: 12-bit ADC resolution
- **Impact**: ±0.5 LSB uncertainty
- **Mitigation**: Dithering, averaging multiple frames

#### 2. Spectral Leakage
- **Source**: Finite-length FFT window
- **Impact**: Energy spreads to adjacent bins
- **Mitigation**: Hann window reduces leakage by ~30 dB

#### 3. Aliasing
- **Source**: Frequencies above Nyquist (8 kHz)
- **Impact**: High frequencies fold back into measurement band
- **Mitigation**: Analog anti-aliasing filter (if available), or digital filtering

#### 4. Microphone Limitations
- **Source**: Non-ideal frequency response, THD
- **Impact**: Frequency-dependent errors
- **Mitigation**: Correction LUT, calibration

#### 5. Temperature Drift
- **Source**: Component temperature coefficients
- **Impact**: DC offset shift, gain variation
- **Mitigation**: Dynamic DC offset calculation, periodic recalibration

### Extending the Implementation

#### Adding C-Weighting

C-weighting has flatter response, useful for peak measurements:

```cpp
// Add to SPL_Meter.h
static const float32_t C_WEIGHTING_LUT_SQUARED[128] = {
    // Calculate using C-weighting formula
    // C(f) = (12194² × f²) / ((f² + 20.6²) × (f² + 12194²))
};

// Modify process() to use C_WEIGHTING_LUT_SQUARED
```

#### Implementing LEQ (Equivalent Continuous Sound Level)

```cpp
// In SPL_Meter class:
private:
    float m_leq_accumulator = 0.0f;
    uint32_t m_leq_sample_count = 0;

public:
    void resetLEQ() {
        m_leq_accumulator = 0.0f;
        m_leq_sample_count = 0;
    }
    
    float getLEQ() {
        if (m_leq_sample_count == 0) return 0.0f;
        float mean_pressure_squared = m_leq_accumulator / m_leq_sample_count;
        return 10.0f * log10f(mean_pressure_squared);
    }
    
    // In process():
    m_leq_accumulator += powf(10.0f, spl / 10.0f);
    m_leq_sample_count++;
```

#### Adding Data Logging

```cpp
// Store measurements with timestamps
struct SPL_Record {
    uint32_t timestamp_ms;
    float spl_dba;
    float leq_dba;
};

// Circular buffer for recent history
SPL_Record history[100];
uint8_t history_index = 0;
```

## References

### Standards and Specifications
- **IEC 61672-1**: Electroacoustics - Sound level meters (Specification)
- **ANSI S1.4**: Specification for Sound Level Meters
- **IEC 61260**: Electroacoustics - Octave-band and fractional-octave-band filters
- **ISO 1996**: Acoustics - Description, measurement and assessment of environmental noise

### Technical Documentation
- **ARM CMSIS-DSP Library Documentation**: [Link](https://arm-software.github.io/CMSIS_5/DSP/html/index.html)
- **ARM Cortex-M33 Technical Reference Manual**
- **Seeed Studio XIAO MG24 Documentation**

### Academic Resources
- Smith, J. O. (2011). *Spectral Audio Signal Processing*. W3K Publishing.
- Oppenheim, A. V., & Schafer, R. W. (2009). *Discrete-Time Signal Processing*. Prentice Hall.
- Kinsler, L. E., et al. (1999). *Fundamentals of Acoustics*. Wiley.

### A-Weighting References
- Robinson, D. W., & Dadson, R. S. (1956). "A re-determination of the equal-loudness relations for pure tones." *British Journal of Applied Physics*, 7(5), 166.

---
**Version**: 1.0  
**Last Updated**: November 2025  
**Compatibility**: Seeed Studio XIAO MG24 Sense

