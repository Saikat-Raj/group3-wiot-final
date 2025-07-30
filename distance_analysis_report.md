# Contact Tracing Distance Analysis Report

---

## Dataset Information

- **Data Source:** `1m_device_10_12_4_64_data_u.csv`
- **Analysis Date:** 2025-07-29 22:00:21
- **Total Records:** 131
- **Numeric Columns:** 11
- **Distance Range:** 1.0m to 10.0m
- **Columns Analyzed:** timeStamp, rssi, uploadDuration, contactDuration, closeContactDuration, distanceInMeter, contact_efficiency, theoretical_rssi, rssi_error, path_loss, exposure_risk

## Distance-Based Analysis

Analysis of contact tracing metrics grouped by distance ranges:

| Distance Group | Sample Size | Avg RSSI (dBm) | Avg Contact Duration (s) | Exposure Rate (%) |
|----------------|-------------|----------------|--------------------------|-------------------|
| 1-2m | 34 | -57.2 | 171.4 | 11.8 |
| 2-5m | 51 | -63.1 | 1301.5 | 78.4 |
| 5m+ | 46 | -73.2 | 2774.9 | 87.0 |

### Distance Group Insights

**1-2m:**
- Signal Strength: -57.2 ± 9.0 dBm
- Contact Duration: 171.4 ± 154.0 seconds
- Exposure Risk: 11.8% of contacts

**2-5m:**
- Signal Strength: -63.1 ± 12.0 dBm
- Contact Duration: 1301.5 ± 643.3 seconds
- Exposure Risk: 78.4% of contacts

**5m+:**
- Signal Strength: -73.2 ± 6.7 dBm
- Contact Duration: 2774.9 ± 1229.6 seconds
- Exposure Risk: 87.0% of contacts

## RSSI vs Distance Relationship

### Correlation Analysis

- **Pearson Correlation:** r = -0.553 (p = 0.0000)
- **Relationship:** Moderate negative correlation
- **Sample Size:** 131 data points

### Model Fitting Results

**Linear Model:** RSSI = -1.90 × distance + -56.78
- R² = 0.305
- Interpretation: RSSI decreases by 1.90 dBm per meter

**Path Loss Model:** RSSI = -17.07 × log₁₀(distance) + -56.68
- R² = 0.312
- Path Loss Exponent: n = 1.71
- **Environment Assessment:** better than free space (possible waveguide effect)

### Model Accuracy

- **RMSE:** 18.00 dBm
- **MAE:** 15.25 dBm
- **Model Accuracy:** Low

## Descriptive Statistics Summary

### Distance Metrics

- **Mean Distance:** 4.38 ± 3.36 meters
- **Median Distance:** 4.00 meters
- **Distance Range:** [1.0, 10.0] meters
- **Interquartile Range:** [1.00, 7.00] meters

### Signal Strength Metrics

- **Mean RSSI:** -65.1 ± 11.5 dBm
- **Median RSSI:** -64.0 dBm
- **RSSI Range:** [-100.0, -45.0] dBm
- **Overall Signal Quality:** Fair

## Key Findings

### Proximity Analysis
- **Close Contacts (≤2m):** 63 contacts (48.1%)
- **Average Contact Distance:** 4.4 meters
- **Environment Type:** Moderate proximity environment

### Signal Propagation
- **Distance-RSSI Correlation:** r = -0.553
- **Path Loss Exponent:** n = 1.71
- **Signal Predictability:** Low predictability (R² = 0.312)

### Exposure Risk by Distance
- **1-2m:** 11.8% exposure rate
- **2-5m:** 78.4% exposure rate
- **5m+:** 87.0% exposure rate

## Data Quality Assessment

- **Missing Data:** 0 values (0.0% of total)
- **Distance Data Completeness:** 100.0%
- **Overall Data Quality:** High

## Methodology

### Analysis Methods
- **Distance Grouping:** Data segmented into 0-1m, 1-2m, 2-5m, and >5m ranges
- **Signal Modeling:** Linear and logarithmic models fitted to RSSI vs distance
- **Path Loss Analysis:** Free Space Path Loss model comparison
- **Correlation Analysis:** Pearson correlation for RSSI-distance relationship
- **Statistical Testing:** Significance testing for model parameters

### Theoretical Framework
- **Free Space Path Loss (FSPL):** Based on electromagnetic wave propagation theory
  - Formula: `FSPL(dB) = 32.45 + 20×log₁₀(f_MHz) + 20×log₁₀(d_km)`
  - For 2.4GHz BLE: `FSPL ≈ 40 + 20×log₁₀(d_m)` at 1m reference
  - Theoretical RSSI: `RSSI = Tx_Power - FSPL` (assuming 0 dBm Tx power)
- **Path Loss Model:** `RSSI = -10×n×log₁₀(d) + C`, where n is path loss exponent
- **Distance Categories:** Based on social distancing and contact tracing guidelines
- **ESP32 BLE Parameters:** 2.4GHz frequency, ~0 dBm typical transmit power

### Files Generated
- `distance_analysis_report.md` - This comprehensive report
- `distance_analysis_plots.png` - Multi-panel distance analysis visualizations
- `enhanced_correlation_heatmap.png` - Correlation matrix with distance metrics

---
*Report generated on 2025-07-29 at 22:00:21*
