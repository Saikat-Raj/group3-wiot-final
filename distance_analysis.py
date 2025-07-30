#!/usr/bin/env python3
"""
Contact Tracing Distance Analysis Tool
Analyzes CSV data from ESP32 contact tracing devices with distance measurements.
Provides comprehensive statistical analysis including distance-based metrics.

Usage: python distance_analysis.py <csv_file_path>
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from scipy import stats
from scipy.stats import pearsonr
import argparse
import sys
from pathlib import Path
import warnings
warnings.filterwarnings('ignore')

class DistanceContactAnalyzer:
    def __init__(self, csv_path):
        """Initialize analyzer with CSV data path."""
        self.csv_path = Path(csv_path)
        self.data = None
        self.numeric_columns = []
        self.results = {}
        self.distance_groups = {}
        
    def load_data(self):
        """Load and validate CSV data with distance information."""
        try:
            print(f"Loading data from: {self.csv_path}")
            self.data = pd.read_csv(self.csv_path)
            print(f"Data loaded successfully: {len(self.data)} rows, {len(self.data.columns)} columns")
            print(f"Columns: {list(self.data.columns)}")
            
            # Filter out header rows that might have been included
            if 'timeStamp' in self.data.columns:
                self.data = self.data[pd.to_numeric(self.data['timeStamp'], errors='coerce').notna()]
            
            # Convert numeric columns
            numeric_cols = ['timeStamp', 'rssi', 'uploadDuration', 'contactDuration', 
                          'closeContactDuration', 'distanceInMeter']
            for col in numeric_cols:
                if col in self.data.columns:
                    self.data[col] = pd.to_numeric(self.data[col], errors='coerce')
                    self.numeric_columns.append(col)
            
            # Add derived metrics
            self.add_derived_metrics()
            
            # Create distance groups
            self.create_distance_groups()
            
            print(f"Numeric columns identified: {self.numeric_columns}")
            print(f"Distance range: {self.data['distanceInMeter'].min():.1f}m to {self.data['distanceInMeter'].max():.1f}m")
            return True
            
        except Exception as e:
            print(f"Error loading data: {e}")
            return False
    
    def add_derived_metrics(self):
        """Add derived metrics for analysis."""
        try:
            # Signal strength categories
            if 'rssi' in self.data.columns:
                self.data['signal_strength_category'] = pd.cut(
                    self.data['rssi'], 
                    bins=[-np.inf, -80, -60, -40, np.inf],
                    labels=['Very Weak', 'Weak', 'Good', 'Strong']
                )
            
            # Distance categories
            if 'distanceInMeter' in self.data.columns:
                self.data['distance_category'] = pd.cut(
                    self.data['distanceInMeter'],
                    bins=[0, 1, 2, 5, np.inf],
                    labels=['Very Close (<1m)', 'Close (1-2m)', 'Moderate (2-5m)', 'Far (>5m)'],
                    include_lowest=True
                )
                
            # Contact efficiency (close contact / total contact ratio)
            if 'closeContactDuration' in self.data.columns and 'contactDuration' in self.data.columns:
                self.data['contact_efficiency'] = (
                    self.data['closeContactDuration'] / 
                    (self.data['contactDuration'] + 1)
                ).fillna(0)
                self.numeric_columns.append('contact_efficiency')
            
            # Theoretical RSSI based on distance (Free Space Path Loss model)
            if 'distanceInMeter' in self.data.columns:
                # FSPL for 2.4GHz BLE: FSPL(dB) = 32.45 + 20*log10(f_MHz) + 20*log10(d_km)
                # For 2.4GHz: FSPL(dB) = 32.45 + 20*log10(2400) + 20*log10(d_m/1000)
                # Simplified: FSPL ≈ 40 + 20*log10(d_m) for 2.4GHz at 1m reference
                # RSSI = Tx_Power - FSPL
                # ESP32 BLE typical Tx power: 0 to +4 dBm, using 0 dBm as reference
                TX_POWER_DBM = 0  # Conservative estimate for ESP32 BLE
                FSPL_CONSTANT = 40  # Free space path loss constant for 2.4GHz BLE
                
                distance_safe = np.maximum(self.data['distanceInMeter'], 0.1)  # Avoid log(0)
                fspl = FSPL_CONSTANT + 20 * np.log10(distance_safe)
                self.data['theoretical_rssi'] = TX_POWER_DBM - fspl
                self.numeric_columns.append('theoretical_rssi')
                
                # RSSI error (difference from theoretical)
                if 'rssi' in self.data.columns:
                    self.data['rssi_error'] = self.data['rssi'] - self.data['theoretical_rssi']
                    self.numeric_columns.append('rssi_error')
                
            # Path loss calculation
            if 'rssi' in self.data.columns and 'distanceInMeter' in self.data.columns:
                # Path Loss = Tx Power - RSSI (assuming 0dBm Tx power)
                self.data['path_loss'] = 0 - self.data['rssi']
                self.numeric_columns.append('path_loss')
                
            # Time-based analysis
            if 'timeStamp' in self.data.columns:
                self.data['datetime'] = pd.to_datetime(self.data['timeStamp'], unit='s')
                self.data['hour'] = self.data['datetime'].dt.hour
                self.data['day_of_week'] = self.data['datetime'].dt.dayofweek
                
            # Exposure risk level
            if 'exposureStatus' in self.data.columns:
                self.data['exposure_risk'] = (
                    self.data['exposureStatus'] == 'EXPOSURE'
                ).astype(int)
                self.numeric_columns.append('exposure_risk')
                
        except Exception as e:
            print(f"Warning: Could not add all derived metrics: {e}")
    
    def create_distance_groups(self):
        """Create distance-based groups for analysis."""
        if 'distanceInMeter' in self.data.columns:
            # Define distance groups
            distance_ranges = [(0, 1), (1, 2), (2, 5), (5, float('inf'))]
            
            for min_dist, max_dist in distance_ranges:
                if max_dist == float('inf'):
                    mask = self.data['distanceInMeter'] >= min_dist
                    group_name = f"{min_dist}m+"
                else:
                    mask = (self.data['distanceInMeter'] >= min_dist) & (self.data['distanceInMeter'] < max_dist)
                    group_name = f"{min_dist}-{max_dist}m"
                
                self.distance_groups[group_name] = self.data[mask]
                
            print(f"Created {len(self.distance_groups)} distance groups:")
            for group_name, group_data in self.distance_groups.items():
                print(f"  {group_name}: {len(group_data)} samples")
    
    def distance_based_analysis(self):
        """Perform analysis for each distance group."""
        print("\n" + "="*80)
        print("DISTANCE-BASED ANALYSIS")
        print("="*80)
        
        distance_results = {}
        
        for group_name, group_data in self.distance_groups.items():
            if len(group_data) == 0:
                continue
                
            print(f"\n{group_name.upper()} ANALYSIS:")
            print("-" * 40)
            
            group_stats = {}
            
            # Basic statistics for this distance group
            for col in ['rssi', 'contactDuration', 'closeContactDuration', 'uploadDuration']:
                if col in group_data.columns and not group_data[col].isna().all():
                    data_col = group_data[col].dropna()
                    if len(data_col) > 0:
                        group_stats[col] = {
                            'count': len(data_col),
                            'mean': np.mean(data_col),
                            'median': np.median(data_col),
                            'std': np.std(data_col, ddof=1) if len(data_col) > 1 else 0,
                            'min': np.min(data_col),
                            'max': np.max(data_col)
                        }
                        
                        print(f"  {col}: μ={group_stats[col]['mean']:.2f}, "
                              f"σ={group_stats[col]['std']:.2f}, "
                              f"n={group_stats[col]['count']}")
            
            # Exposure rate for this distance
            if 'exposureStatus' in group_data.columns:
                exposure_rate = (group_data['exposureStatus'] == 'EXPOSURE').mean() * 100
                group_stats['exposure_rate'] = exposure_rate
                print(f"  Exposure Rate: {exposure_rate:.1f}%")
                
            distance_results[group_name] = group_stats
        
        self.results['distance_analysis'] = distance_results
        return distance_results
    
    def rssi_distance_relationship(self):
        """Analyze RSSI vs Distance relationship."""
        print("\n" + "="*80)
        print("RSSI vs DISTANCE RELATIONSHIP ANALYSIS")
        print("="*80)
        
        if 'rssi' not in self.data.columns or 'distanceInMeter' not in self.data.columns:
            print("RSSI or distance data not available.")
            return None
            
        # Remove invalid data
        valid_data = self.data[(self.data['rssi'].notna()) & 
                              (self.data['distanceInMeter'].notna()) &
                              (self.data['distanceInMeter'] > 0)].copy()
        
        if len(valid_data) < 2:
            print("Insufficient valid data for RSSI-distance analysis.")
            return None
            
        rssi_vals = valid_data['rssi'].values
        distance_vals = valid_data['distanceInMeter'].values
        
        # Calculate correlation
        correlation, p_value = pearsonr(rssi_vals, distance_vals)
        
        # Fit models
        # Linear model: RSSI = a * distance + b
        linear_coeffs = np.polyfit(distance_vals, rssi_vals, 1)
        linear_r2 = np.corrcoef(rssi_vals, np.polyval(linear_coeffs, distance_vals))[0,1]**2
        
        # Log model: RSSI = a * log(distance) + b (path loss model)
        log_distance = np.log10(np.maximum(distance_vals, 0.1))
        log_coeffs = np.polyfit(log_distance, rssi_vals, 1)
        log_r2 = np.corrcoef(rssi_vals, np.polyval(log_coeffs, log_distance))[0,1]**2
        
        # Calculate path loss exponent (n in RSSI = -10*n*log10(d) + C)
        path_loss_exponent = -log_coeffs[0] / 10
        
        print(f"CORRELATION ANALYSIS:")
        print(f"  Pearson correlation: r = {correlation:.3f} (p = {p_value:.4f})")
        print(f"  Relationship strength: {'Strong' if abs(correlation) > 0.7 else 'Moderate' if abs(correlation) > 0.3 else 'Weak'}")
        
        print(f"\nMODEL FITTING:")
        print(f"  Linear Model (RSSI = {linear_coeffs[0]:.2f} * distance + {linear_coeffs[1]:.2f}):")
        print(f"    R² = {linear_r2:.3f}")
        
        print(f"  Log Model (RSSI = {log_coeffs[0]:.2f} * log10(distance) + {log_coeffs[1]:.2f}):")
        print(f"    R² = {log_r2:.3f}")
        print(f"    Path Loss Exponent: n = {path_loss_exponent:.2f}")
        
        # Theoretical vs actual comparison
        if 'theoretical_rssi' in self.data.columns:
            theoretical_vals = valid_data['theoretical_rssi'].values
            rmse = np.sqrt(np.mean((rssi_vals - theoretical_vals)**2))
            mae = np.mean(np.abs(rssi_vals - theoretical_vals))
            
            print(f"\nTHEORETICAL vs ACTUAL:")
            print(f"  RMSE: {rmse:.2f} dBm")
            print(f"  MAE: {mae:.2f} dBm")
        
        rssi_distance_results = {
            'correlation': correlation,
            'p_value': p_value,
            'linear_model': {'coefficients': linear_coeffs, 'r2': linear_r2},
            'log_model': {'coefficients': log_coeffs, 'r2': log_r2},
            'path_loss_exponent': path_loss_exponent,
            'sample_size': len(valid_data)
        }
        
        if 'theoretical_rssi' in self.data.columns:
            rssi_distance_results['rmse'] = rmse
            rssi_distance_results['mae'] = mae
        
        self.results['rssi_distance'] = rssi_distance_results
        return rssi_distance_results
    
    def create_distance_visualizations(self):
        """Create comprehensive distance-based visualizations."""
        try:
            # Create a large figure with multiple subplots
            fig = plt.figure(figsize=(20, 16))
            
            # 1. RSSI vs Distance Scatter Plot
            ax1 = plt.subplot(3, 3, 1)
            if 'rssi' in self.data.columns and 'distanceInMeter' in self.data.columns:
                valid_data = self.data[(self.data['rssi'].notna()) & (self.data['distanceInMeter'].notna())]
                
                # Color by exposure status
                exposure_colors = valid_data['exposureStatus'].map({'NORMAL': 'blue', 'EXPOSURE': 'red'})
                scatter = ax1.scatter(valid_data['distanceInMeter'], valid_data['rssi'], 
                                    c=exposure_colors, alpha=0.6, s=30)
                
                # Add theoretical line
                if 'theoretical_rssi' in self.data.columns:
                    distances = np.linspace(valid_data['distanceInMeter'].min(), 
                                          valid_data['distanceInMeter'].max(), 100)
                    # Use same formula as in derived metrics
                    TX_POWER_DBM = 0
                    FSPL_CONSTANT = 40
                    fspl = FSPL_CONSTANT + 20 * np.log10(np.maximum(distances, 0.1))
                    theoretical = TX_POWER_DBM - fspl
                    ax1.plot(distances, theoretical, 'k--', label='Theoretical (FSPL)', linewidth=2)
                
                # Add fitted line
                if len(valid_data) > 1:
                    z = np.polyfit(valid_data['distanceInMeter'], valid_data['rssi'], 1)
                    p = np.poly1d(z)
                    ax1.plot(valid_data['distanceInMeter'].sort_values(), 
                           p(valid_data['distanceInMeter'].sort_values()), "r--", alpha=0.8, label='Linear Fit')
                
                ax1.set_xlabel('Distance (meters)')
                ax1.set_ylabel('RSSI (dBm)')
                ax1.set_title('RSSI vs Distance')
                ax1.grid(True, alpha=0.3)
                ax1.legend()
            
            # 2. Distance Distribution
            ax2 = plt.subplot(3, 3, 2)
            if 'distanceInMeter' in self.data.columns:
                distances = self.data['distanceInMeter'].dropna()
                ax2.hist(distances, bins=30, alpha=0.7, color='skyblue', edgecolor='black')
                ax2.axvline(distances.mean(), color='red', linestyle='--', label=f'Mean: {distances.mean():.1f}m')
                ax2.axvline(distances.median(), color='orange', linestyle='--', label=f'Median: {distances.median():.1f}m')
                ax2.set_xlabel('Distance (meters)')
                ax2.set_ylabel('Frequency')
                ax2.set_title('Distance Distribution')
                ax2.legend()
                ax2.grid(True, alpha=0.3)
            
            # 3. RSSI Distribution by Distance Categories
            ax3 = plt.subplot(3, 3, 3)
            if 'distance_category' in self.data.columns and 'rssi' in self.data.columns:
                distance_cats = ['Very Close (<1m)', 'Close (1-2m)', 'Moderate (2-5m)', 'Far (>5m)']
                rssi_by_distance = [self.data[self.data['distance_category'] == cat]['rssi'].dropna() 
                                  for cat in distance_cats if cat in self.data['distance_category'].values]
                labels = [cat for cat in distance_cats if cat in self.data['distance_category'].values]
                
                if rssi_by_distance:
                    ax3.boxplot(rssi_by_distance, labels=[label.split(' ')[0] for label in labels])
                    ax3.set_xlabel('Distance Category')
                    ax3.set_ylabel('RSSI (dBm)')
                    ax3.set_title('RSSI Distribution by Distance')
                    ax3.grid(True, alpha=0.3)
                    plt.setp(ax3.get_xticklabels(), rotation=45)
            
            # 4. Contact Duration vs Distance
            ax4 = plt.subplot(3, 3, 4)
            if 'contactDuration' in self.data.columns and 'distanceInMeter' in self.data.columns:
                valid_data = self.data[(self.data['contactDuration'].notna()) & (self.data['distanceInMeter'].notna())]
                ax4.scatter(valid_data['distanceInMeter'], valid_data['contactDuration'], alpha=0.6, color='green')
                ax4.set_xlabel('Distance (meters)')
                ax4.set_ylabel('Contact Duration (seconds)')
                ax4.set_title('Contact Duration vs Distance')
                ax4.grid(True, alpha=0.3)
            
            # 5. Exposure Rate by Distance
            ax5 = plt.subplot(3, 3, 5)
            if 'distance_groups' in dir(self) and self.distance_groups:
                group_names = []
                exposure_rates = []
                sample_sizes = []
                
                for group_name, group_data in self.distance_groups.items():
                    if len(group_data) > 0 and 'exposureStatus' in group_data.columns:
                        exposure_rate = (group_data['exposureStatus'] == 'EXPOSURE').mean() * 100
                        group_names.append(group_name)
                        exposure_rates.append(exposure_rate)
                        sample_sizes.append(len(group_data))
                
                if group_names:
                    bars = ax5.bar(group_names, exposure_rates, color='coral', alpha=0.7)
                    ax5.set_xlabel('Distance Group')
                    ax5.set_ylabel('Exposure Rate (%)')
                    ax5.set_title('Exposure Rate by Distance')
                    ax5.grid(True, alpha=0.3)
                    plt.setp(ax5.get_xticklabels(), rotation=45)
                    
                    # Add sample size annotations
                    for bar, size in zip(bars, sample_sizes):
                        height = bar.get_height()
                        ax5.text(bar.get_x() + bar.get_width()/2., height + 0.5,
                               f'n={size}', ha='center', va='bottom', fontsize=8)
            
            # 6. Path Loss vs Log Distance
            ax6 = plt.subplot(3, 3, 6)
            if 'path_loss' in self.data.columns and 'distanceInMeter' in self.data.columns:
                valid_data = self.data[(self.data['path_loss'].notna()) & (self.data['distanceInMeter'].notna())]
                log_distance = np.log10(np.maximum(valid_data['distanceInMeter'], 0.1))
                ax6.scatter(log_distance, valid_data['path_loss'], alpha=0.6, color='purple')
                
                # Add fitted line
                if len(valid_data) > 1:
                    z = np.polyfit(log_distance, valid_data['path_loss'], 1)
                    p = np.poly1d(z)
                    sorted_log_dist = np.sort(log_distance)
                    ax6.plot(sorted_log_dist, p(sorted_log_dist), "r--", alpha=0.8, 
                           label=f'Slope: {z[0]:.1f} dB/decade')
                
                ax6.set_xlabel('Log10(Distance)')
                ax6.set_ylabel('Path Loss (dB)')
                ax6.set_title('Path Loss vs Log Distance')
                ax6.grid(True, alpha=0.3)
                ax6.legend()
            
            # 7. RSSI Error vs Distance
            ax7 = plt.subplot(3, 3, 7)
            if 'rssi_error' in self.data.columns and 'distanceInMeter' in self.data.columns:
                valid_data = self.data[(self.data['rssi_error'].notna()) & (self.data['distanceInMeter'].notna())]
                ax7.scatter(valid_data['distanceInMeter'], valid_data['rssi_error'], alpha=0.6, color='orange')
                ax7.axhline(y=0, color='black', linestyle='-', alpha=0.5)
                ax7.set_xlabel('Distance (meters)')
                ax7.set_ylabel('RSSI Error (dBm)')
                ax7.set_title('RSSI Error vs Distance')
                ax7.grid(True, alpha=0.3)
            
            # 8. Upload Duration by Distance
            ax8 = plt.subplot(3, 3, 8)
            if 'uploadDuration' in self.data.columns and 'distance_category' in self.data.columns:
                distance_cats = ['Very Close (<1m)', 'Close (1-2m)', 'Moderate (2-5m)', 'Far (>5m)']
                upload_by_distance = [self.data[self.data['distance_category'] == cat]['uploadDuration'].dropna() 
                                    for cat in distance_cats if cat in self.data['distance_category'].values]
                labels = [cat for cat in distance_cats if cat in self.data['distance_category'].values]
                
                if upload_by_distance:
                    ax8.boxplot(upload_by_distance, labels=[label.split(' ')[0] for label in labels])
                    ax8.set_xlabel('Distance Category')
                    ax8.set_ylabel('Upload Duration (ms)')
                    ax8.set_title('Upload Duration by Distance')
                    ax8.grid(True, alpha=0.3)
                    plt.setp(ax8.get_xticklabels(), rotation=45)
            
            # 9. Contact Efficiency vs Distance
            ax9 = plt.subplot(3, 3, 9)
            if 'contact_efficiency' in self.data.columns and 'distanceInMeter' in self.data.columns:
                valid_data = self.data[(self.data['contact_efficiency'].notna()) & (self.data['distanceInMeter'].notna())]
                ax9.scatter(valid_data['distanceInMeter'], valid_data['contact_efficiency'], alpha=0.6, color='brown')
                ax9.set_xlabel('Distance (meters)')
                ax9.set_ylabel('Contact Efficiency')
                ax9.set_title('Contact Efficiency vs Distance')
                ax9.grid(True, alpha=0.3)
            
            plt.tight_layout()
            
            output_path = self.csv_path.parent / 'distance_analysis_plots.png'
            plt.savefig(output_path, dpi=300, bbox_inches='tight')
            print(f"\nDistance analysis plots saved to: {output_path}")
            plt.close()
            
        except Exception as e:
            print(f"Could not create distance visualizations: {e}")
    
    def create_correlation_heatmap(self):
        """Create enhanced correlation heatmap including distance metrics."""
        try:
            # Select numeric columns for correlation
            numeric_data = self.data[self.numeric_columns].select_dtypes(include=[np.number])
            
            if numeric_data.empty or len(numeric_data.columns) < 2:
                print("Insufficient numeric data for correlation analysis.")
                return
            
            # Calculate correlation matrix
            corr_matrix = numeric_data.corr(method='pearson')
            
            # Create the heatmap
            plt.figure(figsize=(14, 12))
            mask = np.zeros_like(corr_matrix, dtype=bool)
            mask[np.triu_indices_from(mask)] = True
            
            # Create custom colormap
            cmap = sns.diverging_palette(250, 10, as_cmap=True)
            
            sns.heatmap(corr_matrix, mask=mask, annot=True, cmap=cmap, center=0,
                       square=True, fmt='.3f', cbar_kws={"shrink": .8},
                       annot_kws={"size": 8})
            
            plt.title('Enhanced Correlation Matrix with Distance Metrics', 
                     fontsize=16, fontweight='bold', pad=20)
            plt.tight_layout()
            
            output_path = self.csv_path.parent / 'enhanced_correlation_heatmap.png'
            plt.savefig(output_path, dpi=300, bbox_inches='tight')
            print(f"Enhanced correlation heatmap saved to: {output_path}")
            plt.close()
            
        except Exception as e:
            print(f"Could not create enhanced correlation heatmap: {e}")
    
    def generate_distance_report(self):
        """Generate comprehensive distance analysis report in Markdown format."""
        report_path = self.csv_path.parent / 'distance_analysis_report.md'
        
        with open(report_path, 'w') as f:
            # Header
            f.write("# Contact Tracing Distance Analysis Report\n\n")
            f.write("---\n\n")
            
            # Metadata
            f.write("## Dataset Information\n\n")
            f.write(f"- **Data Source:** `{self.csv_path.name}`\n")
            f.write(f"- **Analysis Date:** {pd.Timestamp.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"- **Total Records:** {len(self.data):,}\n")
            f.write(f"- **Numeric Columns:** {len(self.numeric_columns)}\n")
            f.write(f"- **Distance Range:** {self.data['distanceInMeter'].min():.1f}m to {self.data['distanceInMeter'].max():.1f}m\n")
            f.write(f"- **Columns Analyzed:** {', '.join(self.numeric_columns)}\n\n")
            
            # Distance-based Analysis
            if 'distance_analysis' in self.results:
                f.write("## Distance-Based Analysis\n\n")
                f.write("Analysis of contact tracing metrics grouped by distance ranges:\n\n")
                
                # Create summary table
                f.write("| Distance Group | Sample Size | Avg RSSI (dBm) | Avg Contact Duration (s) | Exposure Rate (%) |\n")
                f.write("|----------------|-------------|----------------|--------------------------|-------------------|\n")
                
                for group_name, group_stats in self.results['distance_analysis'].items():
                    sample_size = group_stats.get('rssi', {}).get('count', 'N/A')
                    avg_rssi = group_stats.get('rssi', {}).get('mean', 'N/A')
                    avg_contact = group_stats.get('contactDuration', {}).get('mean', 'N/A')
                    exposure_rate = group_stats.get('exposure_rate', 'N/A')
                    
                    f.write(f"| {group_name} | {sample_size} | ")
                    f.write(f"{avg_rssi:.1f} | " if avg_rssi != 'N/A' else "N/A | ")
                    f.write(f"{avg_contact:.1f} | " if avg_contact != 'N/A' else "N/A | ")
                    f.write(f"{exposure_rate:.1f} |\n" if exposure_rate != 'N/A' else "N/A |\n")
                
                f.write("\n### Distance Group Insights\n\n")
                for group_name, group_stats in self.results['distance_analysis'].items():
                    f.write(f"**{group_name}:**\n")
                    if 'rssi' in group_stats:
                        rssi_stats = group_stats['rssi']
                        f.write(f"- Signal Strength: {rssi_stats['mean']:.1f} ± {rssi_stats['std']:.1f} dBm\n")
                    if 'contactDuration' in group_stats:
                        contact_stats = group_stats['contactDuration']
                        f.write(f"- Contact Duration: {contact_stats['mean']:.1f} ± {contact_stats['std']:.1f} seconds\n")
                    if 'exposure_rate' in group_stats:
                        f.write(f"- Exposure Risk: {group_stats['exposure_rate']:.1f}% of contacts\n")
                    f.write("\n")
            
            # RSSI vs Distance Analysis
            if 'rssi_distance' in self.results:
                f.write("## RSSI vs Distance Relationship\n\n")
                rssi_data = self.results['rssi_distance']
                
                f.write("### Correlation Analysis\n\n")
                correlation = rssi_data['correlation']
                p_value = rssi_data['p_value']
                
                strength = "Strong" if abs(correlation) > 0.7 else "Moderate" if abs(correlation) > 0.3 else "Weak"
                direction = "negative" if correlation < 0 else "positive"
                
                f.write(f"- **Pearson Correlation:** r = {correlation:.3f} (p = {p_value:.4f})\n")
                f.write(f"- **Relationship:** {strength} {direction} correlation\n")
                f.write(f"- **Sample Size:** {rssi_data['sample_size']:,} data points\n\n")
                
                f.write("### Model Fitting Results\n\n")
                
                # Linear model
                linear_coeffs = rssi_data['linear_model']['coefficients']
                linear_r2 = rssi_data['linear_model']['r2']
                f.write(f"**Linear Model:** RSSI = {linear_coeffs[0]:.2f} × distance + {linear_coeffs[1]:.2f}\n")
                f.write(f"- R² = {linear_r2:.3f}\n")
                f.write(f"- Interpretation: RSSI decreases by {abs(linear_coeffs[0]):.2f} dBm per meter\n\n")
                
                # Log model (Path Loss)
                log_coeffs = rssi_data['log_model']['coefficients']
                log_r2 = rssi_data['log_model']['r2']
                path_loss_exp = rssi_data['path_loss_exponent']
                f.write(f"**Path Loss Model:** RSSI = {log_coeffs[0]:.2f} × log₁₀(distance) + {log_coeffs[1]:.2f}\n")
                f.write(f"- R² = {log_r2:.3f}\n")
                f.write(f"- Path Loss Exponent: n = {path_loss_exp:.2f}\n")
                
                # Interpret path loss exponent
                if path_loss_exp < 2:
                    environment = "better than free space (possible waveguide effect)"
                elif path_loss_exp < 2.5:
                    environment = "free space or open area"
                elif path_loss_exp < 3:
                    environment = "light obstruction environment"
                elif path_loss_exp < 4:
                    environment = "indoor or moderate obstruction"
                else:
                    environment = "heavy obstruction or indoor with walls"
                    
                f.write(f"- **Environment Assessment:** {environment}\n\n")
                
                # Model accuracy
                if 'rmse' in rssi_data:
                    f.write("### Model Accuracy\n\n")
                    f.write(f"- **RMSE:** {rssi_data['rmse']:.2f} dBm\n")
                    f.write(f"- **MAE:** {rssi_data['mae']:.2f} dBm\n")
                    
                    accuracy = "High" if rssi_data['rmse'] < 5 else "Moderate" if rssi_data['rmse'] < 10 else "Low"
                    f.write(f"- **Model Accuracy:** {accuracy}\n\n")
            
            # Traditional descriptive statistics (simplified for distance focus)
            f.write("## Descriptive Statistics Summary\n\n")
            
            if 'distanceInMeter' in self.data.columns:
                dist_stats = self.data['distanceInMeter'].describe()
                f.write("### Distance Metrics\n\n")
                f.write(f"- **Mean Distance:** {dist_stats['mean']:.2f} ± {dist_stats['std']:.2f} meters\n")
                f.write(f"- **Median Distance:** {dist_stats['50%']:.2f} meters\n")
                f.write(f"- **Distance Range:** [{dist_stats['min']:.1f}, {dist_stats['max']:.1f}] meters\n")
                f.write(f"- **Interquartile Range:** [{dist_stats['25%']:.2f}, {dist_stats['75%']:.2f}] meters\n\n")
            
            if 'rssi' in self.data.columns:
                rssi_stats = self.data['rssi'].describe()
                f.write("### Signal Strength Metrics\n\n")
                f.write(f"- **Mean RSSI:** {rssi_stats['mean']:.1f} ± {rssi_stats['std']:.1f} dBm\n")
                f.write(f"- **Median RSSI:** {rssi_stats['50%']:.1f} dBm\n")
                f.write(f"- **RSSI Range:** [{rssi_stats['min']:.1f}, {rssi_stats['max']:.1f}] dBm\n")
                
                # Signal quality assessment
                mean_rssi = rssi_stats['mean']
                if mean_rssi > -50:
                    quality = "Excellent"
                elif mean_rssi > -60:
                    quality = "Good"
                elif mean_rssi > -70:
                    quality = "Fair"
                else:
                    quality = "Poor"
                f.write(f"- **Overall Signal Quality:** {quality}\n\n")
            
            # Key Findings
            f.write("## Key Findings\n\n")
            
            # Distance distribution
            if 'distanceInMeter' in self.data.columns:
                close_contacts = (self.data['distanceInMeter'] <= 2).sum()
                close_percentage = (close_contacts / len(self.data)) * 100
                f.write(f"### Proximity Analysis\n")
                f.write(f"- **Close Contacts (≤2m):** {close_contacts:,} contacts ({close_percentage:.1f}%)\n")
                
                avg_distance = self.data['distanceInMeter'].mean()
                f.write(f"- **Average Contact Distance:** {avg_distance:.1f} meters\n")
                
                if avg_distance <= 2:
                    proximity_assessment = "High proximity environment"
                elif avg_distance <= 5:
                    proximity_assessment = "Moderate proximity environment"
                else:
                    proximity_assessment = "Low proximity environment"
                f.write(f"- **Environment Type:** {proximity_assessment}\n\n")
            
            # RSSI-Distance relationship summary
            if 'rssi_distance' in self.results:
                rssi_data = self.results['rssi_distance']
                f.write(f"### Signal Propagation\n")
                f.write(f"- **Distance-RSSI Correlation:** r = {rssi_data['correlation']:.3f}\n")
                f.write(f"- **Path Loss Exponent:** n = {rssi_data['path_loss_exponent']:.2f}\n")
                
                if rssi_data['log_model']['r2'] > 0.7:
                    predictability = "Highly predictable"
                elif rssi_data['log_model']['r2'] > 0.5:
                    predictability = "Moderately predictable"
                else:
                    predictability = "Low predictability"
                f.write(f"- **Signal Predictability:** {predictability} (R² = {rssi_data['log_model']['r2']:.3f})\n\n")
            
            # Exposure analysis by distance
            if 'distance_analysis' in self.results:
                f.write(f"### Exposure Risk by Distance\n")
                for group_name, group_stats in self.results['distance_analysis'].items():
                    if 'exposure_rate' in group_stats:
                        f.write(f"- **{group_name}:** {group_stats['exposure_rate']:.1f}% exposure rate\n")
                f.write("\n")
            
            # Data Quality Assessment
            f.write("## Data Quality Assessment\n\n")
            total_missing = self.data.isnull().sum().sum()
            missing_percentage = (total_missing / (len(self.data) * len(self.data.columns))) * 100
            f.write(f"- **Missing Data:** {total_missing:,} values ({missing_percentage:.1f}% of total)\n")
            
            # Distance data quality
            if 'distanceInMeter' in self.data.columns:
                valid_distances = self.data['distanceInMeter'].notna().sum()
                distance_completeness = (valid_distances / len(self.data)) * 100
                f.write(f"- **Distance Data Completeness:** {distance_completeness:.1f}%\n")
                
                zero_distances = (self.data['distanceInMeter'] == 0).sum()
                if zero_distances > 0:
                    f.write(f"- **Zero Distance Measurements:** {zero_distances} ({(zero_distances/len(self.data))*100:.1f}%)\n")
            
            quality_rating = "High" if missing_percentage < 1 else "Moderate" if missing_percentage < 5 else "Low"
            f.write(f"- **Overall Data Quality:** {quality_rating}\n\n")
            
            # Methodology
            f.write("## Methodology\n\n")
            f.write("### Analysis Methods\n")
            f.write("- **Distance Grouping:** Data segmented into 0-1m, 1-2m, 2-5m, and >5m ranges\n")
            f.write("- **Signal Modeling:** Linear and logarithmic models fitted to RSSI vs distance\n")
            f.write("- **Path Loss Analysis:** Free Space Path Loss model comparison\n")
            f.write("- **Correlation Analysis:** Pearson correlation for RSSI-distance relationship\n")
            f.write("- **Statistical Testing:** Significance testing for model parameters\n\n")
            
            f.write("### Theoretical Framework\n")
            f.write("- **Free Space Path Loss (FSPL):** Based on electromagnetic wave propagation theory\n")
            f.write("  - Formula: `FSPL(dB) = 32.45 + 20×log₁₀(f_MHz) + 20×log₁₀(d_km)`\n")
            f.write("  - For 2.4GHz BLE: `FSPL ≈ 40 + 20×log₁₀(d_m)` at 1m reference\n")
            f.write("  - Theoretical RSSI: `RSSI = Tx_Power - FSPL` (assuming 0 dBm Tx power)\n")
            f.write("- **Path Loss Model:** `RSSI = -10×n×log₁₀(d) + C`, where n is path loss exponent\n")
            f.write("- **Distance Categories:** Based on social distancing and contact tracing guidelines\n")
            f.write("- **ESP32 BLE Parameters:** 2.4GHz frequency, ~0 dBm typical transmit power\n\n")
            
            f.write("### Files Generated\n")
            f.write("- `distance_analysis_report.md` - This comprehensive report\n")
            f.write("- `distance_analysis_plots.png` - Multi-panel distance analysis visualizations\n")
            f.write("- `enhanced_correlation_heatmap.png` - Correlation matrix with distance metrics\n\n")
            
            f.write("---\n")
            f.write(f"*Report generated on {pd.Timestamp.now().strftime('%Y-%m-%d at %H:%M:%S')}*\n")
        
        print(f"\nComprehensive distance analysis report saved to: {report_path}")
    
    def run_full_analysis(self):
        """Run complete distance-based analysis."""
        if not self.load_data():
            return False
        
        print(f"\nRunning comprehensive distance analysis on {len(self.data)} records...")
        
        # Run all analyses
        self.distance_based_analysis()
        self.rssi_distance_relationship()
        
        # Generate visualizations
        self.create_distance_visualizations()
        self.create_correlation_heatmap()
        
        # Generate report
        self.generate_distance_report()
        
        print("\n" + "="*80)
        print("DISTANCE ANALYSIS COMPLETE")
        print("="*80)
        print(f"Results saved to: {self.csv_path.parent}")
        
        return True

def main():
    """Main function to run the distance analysis."""
    parser = argparse.ArgumentParser(description='Analyze contact tracing CSV data with distance measurements')
    parser.add_argument('csv_path', help='Path to the CSV file to analyze')
    
    args = parser.parse_args()
    
    # Validate input file
    csv_path = Path(args.csv_path)
    if not csv_path.exists():
        print(f"Error: File not found: {csv_path}")
        sys.exit(1)
    
    if not csv_path.suffix.lower() == '.csv':
        print(f"Warning: File does not have .csv extension: {csv_path}")
    
    # Run analysis
    analyzer = DistanceContactAnalyzer(csv_path)
    success = analyzer.run_full_analysis()
    
    if not success:
        print("Analysis failed!")
        sys.exit(1)
    
    print("Distance analysis completed successfully!")

if __name__ == "__main__":
    main()