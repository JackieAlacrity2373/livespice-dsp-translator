#!/usr/bin/env python3

import sys
import wave
import numpy as np
from pathlib import Path

def analyze_wav(filename):
    """Analyze WAV file and report audio quality metrics"""
    
    filepath = Path(filename)
    if not filepath.exists():
        print(f"Error: File not found: {filename}")
        return False
    
    print(f"\nReading file: {filename}")
    
    try:
        with wave.open(str(filepath), 'rb') as wav_file:
            n_channels = wav_file.getnchannels()
            sample_width = wav_file.getsampwidth()
            sample_rate = wav_file.getframerate()
            n_frames = wav_file.getnframes()
            
            print(f"Audio Properties:")
            print(f"  Channels:        {n_channels}")
            print(f"  Sample Rate:     {sample_rate} Hz")
            print(f"  Bit Depth:       {sample_width * 8} bits")
            print(f"  Frames:          {n_frames}")
            print(f"  Duration:        {n_frames / sample_rate:.2f} seconds")
            
            # Read all frames
            audio_bytes = wav_file.readframes(n_frames)
            
            # Convert to numpy array
            audio_array = np.frombuffer(audio_bytes, dtype=np.int16)
            
            # Reshape to channels
            if n_channels > 1:
                audio_array = audio_array.reshape((-1, n_channels))
                # Mix channels
                audio_mono = np.mean(audio_array, axis=1)
            else:
                audio_mono = audio_array
            
            # Normalize to [-1, 1]
            audio_float = audio_mono.astype(np.float32) / 32768.0
            
            # === ANALYSIS ===
            print("\n" + "="*80)
            print("GUITAR OUTPUT ANALYSIS")
            print("="*80)
            
            # Peak analysis
            peak = np.max(np.abs(audio_float))
            peak_db = 20 * np.log10(peak + 1e-10)
            
            # RMS
            rms = np.sqrt(np.mean(audio_float ** 2))
            rms_db = 20 * np.log10(rms + 1e-10)
            
            # DC offset
            mean_val = np.mean(audio_float)
            
            # Crest factor
            crest = peak / (rms + 1e-10)
            
            print("\nSIGNAL METRICS:")
            print(f"  Peak Level:      {peak:.6f} ({peak_db:+.2f} dB)")
            print(f"  RMS Level:       {rms:.6f} ({rms_db:+.2f} dB)")
            print(f"  DC Offset:       {mean_val:.8f}")
            print(f"  Crest Factor:    {crest:.2f}x")
            
            # Clipping analysis
            hard_clip = np.sum(np.abs(audio_float) >= 0.99)
            soft_clip = np.sum((np.abs(audio_float) >= 0.95) & (np.abs(audio_float) < 0.99))
            
            hard_clip_pct = (hard_clip / len(audio_float)) * 100
            soft_clip_pct = (soft_clip / len(audio_float)) * 100
            
            print("\nCLIPPING ANALYSIS:")
            print(f"  Hard Clipping:   {hard_clip:,} samples ({hard_clip_pct:.4f}%)")
            print(f"  Soft Clipping:   {soft_clip:,} samples ({soft_clip_pct:.4f}%)")
            
            if hard_clip_pct > 0.1:
                print("  STATUS: CLIPPING DETECTED - reduce level!")
            elif soft_clip_pct > 1.0:
                print("  STATUS: Soft clipping (acceptable for overdrive)")
            else:
                print("  STATUS: No clipping")
            
            # Distortion (THD approximation via waveform symmetry)
            half_size = len(audio_float) // 2
            sym_error = np.mean(np.abs(audio_float[:half_size] - audio_float[-half_size:][::-1]))
            thd_est = min(100, (sym_error / (rms + 1e-10)) * 100)
            
            print("\nDISTORTION:")
            print(f"  Estimated THD:   {thd_est:.2f}%")
            if thd_est < 5:
                print("  CHARACTER:       Clean")
            elif thd_est < 15:
                print("  CHARACTER:       Light distortion")
            elif thd_est < 50:
                print("  CHARACTER:       Heavy distortion (typical overdrive)")
            else:
                print("  CHARACTER:       Extreme distortion")
            
            # Noise floor
            sorted_mag = np.sort(np.abs(audio_float))
            noise_floor_idx = len(sorted_mag) // 10
            noise_floor = np.sqrt(np.mean(sorted_mag[:noise_floor_idx] ** 2))
            snr = 20 * np.log10((peak + 1e-10) / (noise_floor + 1e-10))
            
            print("\nNOISE FLOOR:")
            print(f"  SNR:             {snr:.2f} dB")
            if snr > 40:
                print("  QUALITY:         Excellent")
            elif snr > 20:
                print("  QUALITY:         Good")
            else:
                print("  QUALITY:         Poor")
            
            # Overall assessment
            print("\nFINAL ASSESSMENT:")
            score = 100
            issues = []
            
            if peak_db > 0:
                score -= 40
                issues.append("CRITICAL: Digital clipping (peak >= 0dB)")
            elif hard_clip_pct > 0.1:
                score -= 30
                issues.append("Hard clipping detected")
            elif soft_clip_pct > 2:
                score -= 10
                issues.append("Moderate soft clipping")
            
            if peak_db < -24:
                score -= 20
                issues.append("Signal too quiet (<-24dB)")
            
            if abs(mean_val) > 0.01:
                score -= 10
                issues.append("DC offset present")
            
            print(f"  Score:           {score}/100 ", end="")
            if score >= 90:
                print("(A+)")
            elif score >= 80:
                print("(A)")
            elif score >= 70:
                print("(B)")
            elif score >= 60:
                print("(C)")
            else:
                print("(F)")
            
            if not issues:
                print("  Status:          OK - no issues detected!")
            else:
                print("  Issues:")
                for issue in issues:
                    print(f"    - {issue}")
            
            print("\n" + "="*80)
            
            return True
            
    except wave.Error as e:
        print(f"Error reading WAV file: {e}")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python audio_analyzer.py <wav_file>")
        sys.exit(1)
    
    success = analyze_wav(sys.argv[1])
    sys.exit(0 if success else 1)
