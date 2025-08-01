# Contact Tracing Distance Analysis Report

---

## Dataset Information

- **Data Source:** `data.csv`
- **Analysis Date:** 2025-08-01 16:15:29
- **Total Records:** 138
- **Numeric Columns:** 11
- **Distance Range:** 1.0m to 10.0m
- **Columns Analyzed:** timeStamp, rssi, uploadDuration, contactDuration, closeContactDuration, distanceInMeter, contact_efficiency, theoretical_rssi, rssi_error, path_loss, exposure_risk

## Distance-Based Analysis

Analysis of contact tracing metrics grouped by distance ranges:

| Distance Group | Sample Size | Avg RSSI (dBm) | Avg Contact Duration (s) | Exposure Rate (%) |
|----------------|-------------|----------------|--------------------------|-------------------|
| 1-2m | 31 | -57.5 | 236.6 | 22.6 |
| 2-5m | 55 | -58.6 | 243.3 | 25.5 |
| 5m+ | 52 | -72.6 | 236.3 | 0.0 |

### Distance Group Insights

**1-2m:**
- Signal Strength: -57.5 ± 11.8 dBm
- Contact Duration: 236.6 ± 145.0 seconds
- Exposure Risk: 22.6% of contacts

**2-5m:**
- Signal Strength: -58.6 ± 4.5 dBm
- Contact Duration: 243.3 ± 148.9 seconds
- Exposure Risk: 25.5% of contacts

**5m+:**
- Signal Strength: -72.6 ± 4.3 dBm
- Contact Duration: 236.3 ± 136.9 seconds
- Exposure Risk: 0.0% of contacts

## RSSI vs Distance Relationship

### Correlation Analysis

- **Pearson Correlation:** r = -0.673 (p = 0.0000)
- **Relationship:** Moderate negative correlation
- **Sample Size:** 138 data points

### Model Fitting Results

**Linear Model:** RSSI = -1.99 × distance + -54.55
- R² = 0.452
- Interpretation: RSSI decreases by 1.99 dBm per meter

**Path Loss Model:** RSSI = -17.07 × log₁₀(distance) + -54.71
- R² = 0.416
- Path Loss Exponent: n = 1.71
- **Environment Assessment:** better than free space (possible waveguide effect)

### Model Accuracy

- **RMSE:** 15.16 dBm
- **MAE:** 13.18 dBm
- **Model Accuracy:** Low

## Descriptive Statistics Summary

### Distance Metrics

- **Mean Distance:** 4.58 ± 3.30 meters
- **Median Distance:** 4.00 meters
- **Distance Range:** [1.0, 10.0] meters
- **Interquartile Range:** [2.00, 7.00] meters

### Signal Strength Metrics

- **Mean RSSI:** -63.6 ± 9.7 dBm
- **Median RSSI:** -63.0 dBm
- **RSSI Range:** [-98.0, -45.0] dBm
- **Overall Signal Quality:** Fair

## Key Findings

### Proximity Analysis
- **Close Contacts (≤2m):** 60 contacts (43.5%)
- **Average Contact Distance:** 4.6 meters
- **Environment Type:** Moderate proximity environment

### Signal Propagation
- **Distance-RSSI Correlation:** r = -0.673
- **Path Loss Exponent:** n = 1.71
- **Signal Predictability:** Low predictability (R² = 0.416)

### Exposure Risk by Distance
- **1-2m:** 22.6% exposure rate
- **2-5m:** 25.5% exposure rate
- **5m+:** 0.0% exposure rate

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
*Report generated on 2025-08-01 at 16:15:29*
