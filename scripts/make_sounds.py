import wave
import math
import struct
import os
import sys

def generate_beep(output_dir, filename, frequency=440.0, duration=0.1, volume=0.5):
    filepath = os.path.join(output_dir, filename)
    sample_rate = 22050
    n_samples = int(sample_rate * duration)
    
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1) # Mono
        wav_file.setsampwidth(1) # 8-bit
        wav_file.setframerate(sample_rate)
        
        for i in range(n_samples):
            t = float(i) / sample_rate
            # Simple Sine Wave
            val = math.sin(2.0 * math.pi * frequency * t)
            
            # Apply fade out
            if i > n_samples - 500:
                val *= (n_samples - i) / 500.0
            if i < 500:
                val *= i / 500.0
            
            # Convert to 8-bit unsigned (0-255, center 128)
            data = int((val * 0.5 + 0.5) * volume * 255.0)
            wav_file.writeframes(struct.pack('B', data))
            
def generate_chime(output_dir, filename):
    filepath = os.path.join(output_dir, filename)
    sample_rate = 22050
    duration = 0.4
    n_samples = int(sample_rate * duration)
    
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(1)
        wav_file.setframerate(sample_rate)
        
        for i in range(n_samples):
            t = float(i) / sample_rate
            # Mix two frequencies (Major 3rd)
            val = 0.5 * math.sin(2.0 * math.pi * 880.0 * t) + \
                  0.5 * math.sin(2.0 * math.pi * 1108.7 * t)
            
            # Decay
            val *= math.exp(-3.0 * t)
            
            data = int((val * 0.5 + 0.5) * 255.0)
            wav_file.writeframes(struct.pack('B', data))

def generate_finish(output_dir, filename):
    filepath = os.path.join(output_dir, filename)
    sample_rate = 22050
    duration = 1.5
    n_samples = int(sample_rate * duration)
    
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(1)
        wav_file.setframerate(sample_rate)
        
        for i in range(n_samples):
            t = float(i) / sample_rate
            # Arpeggio
            freq = 440.0
            if t > 0.3: freq = 554.37
            if t > 0.6: freq = 659.25
            if t > 0.9: freq = 880.0
            
            val = math.sin(2.0 * math.pi * freq * t) * math.exp(-2.0 * (t % 0.3))
            
            data = int((val * 0.5 + 0.5) * 255.0)
            wav_file.writeframes(struct.pack('B', data))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python make_sounds.py <output_dir>")
        sys.exit(1)
    
    out_dir = sys.argv[1]
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
        
    generate_beep(out_dir, "hit.wav", frequency=1200.0, duration=0.05, volume=0.3)
    generate_chime(out_dir, "ok.wav")
    generate_finish(out_dir, "finish.wav")
    print(f"Generated all sounds in {out_dir}")
