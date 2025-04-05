import os
from pydub import AudioSegment

def convert_wav_to_header(wav_path, header_path, array_name):
    # Convert to 8-bit mono PCM using pydub
    sound = AudioSegment.from_file(wav_path)
    sound = sound.set_channels(1)
    sound = sound.set_sample_width(1)  # 8-bit
    sound = sound.set_frame_rate(44100)
    raw_data = sound.raw_data
    data = list(raw_data)

    # Write header file
    with open(header_path, 'w') as f:
        f.write(f"/* Auto-generated from: {os.path.basename(wav_path)} */\n")
        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const uint8_t {array_name}[] = {{\n")

        for i, byte in enumerate(data):
            f.write(f"0x{byte:02x}, ")
            if (i + 1) % 16 == 0:
                f.write("\n")
        f.write("\n};\n")
        f.write(f"const unsigned int {array_name}_len = {len(data)};\n")

    print(f"✅ Generated: {header_path} ({len(data)} samples)")

def sanitize_filename(name):
    name = os.path.splitext(name)[0]
    return name.replace(" ", "_").replace("-", "_").lower()

def main():
    current_dir = os.path.dirname(os.path.realpath(__file__))
    wav_files = [f for f in os.listdir(current_dir) if f.lower().endswith('.wav')]

    if not wav_files:
        print("No .wav files found.")
        return

    for wav_file in wav_files:
        wav_path = os.path.join(current_dir, wav_file)
        base_name = sanitize_filename(wav_file)
        header_path = os.path.join(current_dir, f"{base_name}.h")
        convert_wav_to_header(wav_path, header_path, base_name)

if __name__ == "__main__":
    main()
