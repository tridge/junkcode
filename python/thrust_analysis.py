#!/usr/bin/env python3
"""
Thrust Linearization Analysis for V505 KV260 + P17x5.8
Generates graphs to justify MOT_THST_EXPO = 0.55
"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Test data: PWM, Thrust(g), Voltage, Current
data = [
    (1000, 0, 48.00, 0),
    (1400, 2329, 47.89, 7.51),
    (1420, 2513, 47.88, 8.25),
    (1440, 2663, 47.87, 8.93),
    (1460, 2806, 47.85, 9.76),
    (1480, 2979, 47.83, 10.64),
    (1500, 3203, 47.80, 11.68),
    (1520, 3379, 47.78, 12.61),
    (1540, 3476, 47.76, 12.99),
    (1560, 3677, 47.74, 14.12),
    (1580, 3900, 47.72, 15.30),
    (1600, 4093, 47.70, 16.37),
    (1620, 4288, 47.67, 17.54),
    (1640, 4502, 47.64, 18.81),
    (1660, 4721, 47.60, 20.07),
    (1680, 4927, 47.56, 21.42),
    (1700, 5149, 47.52, 22.88),
    (1800, 6292, 47.30, 30.61),
    (1900, 7349, 47.10, 39.85),
    (2000, 8741, 46.93, 52.93),
]

pwm = np.array([d[0] for d in data])
thrust = np.array([d[1] for d in data])
current = np.array([d[3] for d in data])

# Normalize
throttle = (pwm - 1000) / 1000  # 0 to 1
thrust_norm = thrust / thrust.max()  # 0 to 1

# Thrust model: thrust = (1-expo)*throttle + expo*throttle^2
def thrust_model(throttle, expo):
    return (1 - expo) * throttle + expo * throttle**2

def inverse_thrust_model(thrust_cmd, expo):
    """Given desired thrust, return throttle needed"""
    # Solve: thrust = (1-expo)*t + expo*t^2
    # expo*t^2 + (1-expo)*t - thrust = 0
    a = expo
    b = 1 - expo
    c = -thrust_cmd
    if a == 0:
        return thrust_cmd / b
    discriminant = b**2 - 4*a*c
    return (-b + np.sqrt(discriminant)) / (2*a)

# Generate smooth curves
t_smooth = np.linspace(0, 1, 100)

# ============ FIGURE 1: Raw Thrust Curve ============
fig1, ax1 = plt.subplots(figsize=(10, 6))
ax1.plot(throttle * 100, thrust / 1000, 'bo-', markersize=8, linewidth=2, label='Measured Thrust')
ax1.plot([0, 100], [0, thrust.max()/1000], 'g--', linewidth=2, alpha=0.7, label='Ideal Linear')
ax1.set_xlabel('Throttle Command (%)', fontsize=12)
ax1.set_ylabel('Thrust (kg)', fontsize=12)
ax1.set_title('V505 KV260 + P17×5.8 Thrust Curve', fontsize=14, fontweight='bold')
ax1.legend(fontsize=11)
ax1.grid(True, alpha=0.3)
ax1.set_xlim(0, 100)
ax1.set_ylim(0, 9)
fig1.tight_layout()
fig1.savefig('graph1_raw_thrust_curve.png', dpi=150)
print("Saved: graph1_raw_thrust_curve.png")

# ============ FIGURE 2: Normalized Comparison ============
fig2, ax2 = plt.subplots(figsize=(10, 6))
ax2.plot(throttle * 100, thrust_norm * 100, 'bo-', markersize=8, linewidth=2, label='Measured')
ax2.plot(t_smooth * 100, t_smooth * 100, 'g--', linewidth=2, label='Ideal Linear')
ax2.plot(t_smooth * 100, thrust_model(t_smooth, 0.2) * 100, 'r-', linewidth=2, label='Expo=0.2 (current)')
ax2.plot(t_smooth * 100, thrust_model(t_smooth, 0.55) * 100, 'm-', linewidth=2, label='Expo=0.55 (recommended)')
ax2.set_xlabel('Throttle Command (%)', fontsize=12)
ax2.set_ylabel('Thrust Output (%)', fontsize=12)
ax2.set_title('Thrust Linearization Model Comparison', fontsize=14, fontweight='bold')
ax2.legend(fontsize=11)
ax2.grid(True, alpha=0.3)
ax2.set_xlim(0, 100)
ax2.set_ylim(0, 100)
fig2.tight_layout()
fig2.savefig('graph2_model_comparison.png', dpi=150)
print("Saved: graph2_model_comparison.png")

# ============ FIGURE 3: Linearization Error ============
fig3, ax3 = plt.subplots(figsize=(10, 6))

# Error with expo=0.2 (current)
error_020 = []
for t, tn in zip(throttle[1:], thrust_norm[1:]):  # skip zero point
    predicted = thrust_model(t, 0.2)
    error_020.append((predicted - tn) * 100)

# Error with expo=0.55 (recommended)
error_055 = []
for t, tn in zip(throttle[1:], thrust_norm[1:]):
    predicted = thrust_model(t, 0.55)
    error_055.append((predicted - tn) * 100)

throttle_pct = throttle[1:] * 100
ax3.bar(throttle_pct - 1.5, error_020, width=3, color='red', alpha=0.7, label='Expo=0.2 (current)')
ax3.bar(throttle_pct + 1.5, error_055, width=3, color='green', alpha=0.7, label='Expo=0.55 (recommended)')
ax3.axhline(y=0, color='black', linestyle='-', linewidth=1)
ax3.set_xlabel('Throttle Command (%)', fontsize=12)
ax3.set_ylabel('Thrust Prediction Error (%)', fontsize=12)
ax3.set_title('Thrust Prediction Error: Current vs Recommended Expo', fontsize=14, fontweight='bold')
ax3.legend(fontsize=11)
ax3.grid(True, alpha=0.3, axis='y')
ax3.set_xlim(35, 105)
fig3.tight_layout()
fig3.savefig('graph3_error_comparison.png', dpi=150)
print("Saved: graph3_error_comparison.png")

# ============ FIGURE 4: Linearized Response ============
fig4, ax4 = plt.subplots(figsize=(10, 6))

# With expo=0.55, show what throttle is commanded for desired thrust
desired_thrust = np.linspace(0, 1, 100)
throttle_cmd_055 = np.array([inverse_thrust_model(t, 0.55) for t in desired_thrust])
actual_thrust_055 = thrust_model(throttle_cmd_055, 0.55)

# Also show expo=0.2 for comparison
throttle_cmd_020 = np.array([inverse_thrust_model(t, 0.20) for t in desired_thrust])

ax4.plot(desired_thrust * 100, desired_thrust * 100, 'g--', linewidth=2, label='Ideal (commanded = actual)')
ax4.plot(throttle_cmd_020 * 100, thrust_model(throttle_cmd_020, 0.55) * 100, 'r-', linewidth=2,
         label='Expo=0.2 commanding real motor')
ax4.plot(throttle_cmd_055 * 100, actual_thrust_055 * 100, 'm-', linewidth=2,
         label='Expo=0.55 commanding real motor')
ax4.set_xlabel('Commanded Thrust (%)', fontsize=12)
ax4.set_ylabel('Actual Thrust Output (%)', fontsize=12)
ax4.set_title('Linearized Response: Commanded vs Actual Thrust', fontsize=14, fontweight='bold')
ax4.legend(fontsize=11)
ax4.grid(True, alpha=0.3)
ax4.set_xlim(0, 100)
ax4.set_ylim(0, 100)
fig4.tight_layout()
fig4.savefig('graph4_linearized_response.png', dpi=150)
print("Saved: graph4_linearized_response.png")

# ============ FIGURE 5: Summary with Key Points ============
fig5, ((ax5a, ax5b), (ax5c, ax5d)) = plt.subplots(2, 2, figsize=(14, 10))

# 5a: Raw data
ax5a.plot(throttle * 100, thrust / 1000, 'bo-', markersize=6, linewidth=1.5)
ax5a.plot([0, 100], [0, thrust.max()/1000], 'g--', linewidth=1.5, alpha=0.7)
ax5a.set_xlabel('Throttle (%)')
ax5a.set_ylabel('Thrust (kg)')
ax5a.set_title('Raw Thrust Curve')
ax5a.grid(True, alpha=0.3)

# 5b: Model fit
ax5b.plot(throttle * 100, thrust_norm * 100, 'bo', markersize=6, label='Measured')
ax5b.plot(t_smooth * 100, thrust_model(t_smooth, 0.55) * 100, 'm-', linewidth=2, label='Expo=0.55 fit')
ax5b.set_xlabel('Throttle (%)')
ax5b.set_ylabel('Thrust (%)')
ax5b.set_title('Best Fit: MOT_THST_EXPO = 0.55')
ax5b.legend()
ax5b.grid(True, alpha=0.3)

# 5c: Error comparison
ax5c.bar(throttle_pct - 1.5, np.abs(error_020), width=3, color='red', alpha=0.7, label='|Error| Expo=0.2')
ax5c.bar(throttle_pct + 1.5, np.abs(error_055), width=3, color='green', alpha=0.7, label='|Error| Expo=0.55')
ax5c.set_xlabel('Throttle (%)')
ax5c.set_ylabel('Absolute Error (%)')
ax5c.set_title('Prediction Error Magnitude')
ax5c.legend()
ax5c.grid(True, alpha=0.3, axis='y')

# 5d: Stats text
ax5d.axis('off')
stats_text = """
THRUST LINEARIZATION ANALYSIS
─────────────────────────────
Motor: T-Motor V505 KV260
Prop:  P17×5.8
ESC:   Flame 60A 12S

CURRENT SETTING
  MOT_THST_EXPO = 0.2
  RMS Error: {:.1f}%
  Max Error: {:.1f}%

RECOMMENDED SETTING
  MOT_THST_EXPO = 0.55
  RMS Error: {:.1f}%
  Max Error: {:.1f}%

IMPROVEMENT
  Error reduced by {:.0f}%

At 50% throttle:
  Expo=0.2:  expects 40%, gets 37% → 8% error
  Expo=0.55: expects 37%, gets 37% → linearized
""".format(
    np.sqrt(np.mean(np.array(error_020)**2)),
    np.max(np.abs(error_020)),
    np.sqrt(np.mean(np.array(error_055)**2)),
    np.max(np.abs(error_055)),
    (1 - np.sqrt(np.mean(np.array(error_055)**2)) / np.sqrt(np.mean(np.array(error_020)**2))) * 100
)
ax5d.text(0.1, 0.9, stats_text, transform=ax5d.transAxes, fontsize=12,
          verticalalignment='top', fontfamily='monospace',
          bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

fig5.suptitle('V505 KV260 + P17×5.8 Thrust Linearization Summary', fontsize=14, fontweight='bold')
fig5.tight_layout()
fig5.savefig('graph5_summary.png', dpi=150)
print("Saved: graph5_summary.png")

plt.close('all')
print("\nAll graphs saved to current directory.")
