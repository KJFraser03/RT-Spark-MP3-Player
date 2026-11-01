with open("src/exported.wav", "rb") as f:
    f.seek(44)  # Skip 44-byte WAV header
    data = f.read()

print(f"const unsigned char pcmAudioSample[{len(data)}] PROGMEM = {{")
for i, byte in enumerate(data):
    if i % 12 == 0:
        print("\n  ", end="")
    print(f"0x{byte:02X}, ", end="")
print("\n};")