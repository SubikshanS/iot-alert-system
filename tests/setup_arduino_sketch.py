import os
import shutil
import re

src_dir = os.path.join(os.path.dirname(__file__), '..', 'firmware')
dst_dir = os.path.join(os.path.dirname(__file__), '..', 'arduino', 'smart_fire_detection')

os.makedirs(dst_dir, exist_ok=True)

# Files to copy
file_map = {
    # Headers
    'include/config.h': 'config.h',
    'include/types.h': 'types.h',
    'include/secrets.example.h': 'secrets.example.h',
    'include/system_manager.h': 'system_manager.h',
    'include/sensors/mq2.h': 'mq2.h',
    'include/sensors/dht11.h': 'dht11.h',
    'include/sensors/flame.h': 'flame.h',
    'include/sensors/gps.h': 'gps.h',
    'include/controllers/detection.h': 'detection.h',
    'include/controllers/alarm.h': 'alarm.h',
    'include/controllers/emergency.h': 'emergency.h',
    'include/display/oled.h': 'oled.h',
    'include/communication/wifi_manager.h': 'wifi_manager.h',
    'include/communication/telegram_manager.h': 'telegram_manager.h',
    'include/communication/web_server.h': 'web_server.h',
    # Source files
    'src/controllers/system_manager.cpp': 'system_manager.cpp',
    'src/sensors/mq2.cpp': 'mq2.cpp',
    'src/sensors/dht11.cpp': 'dht11.cpp',
    'src/sensors/flame.cpp': 'flame.cpp',
    'src/sensors/gps.cpp': 'gps.cpp',
    'src/controllers/detection.cpp': 'detection.cpp',
    'src/controllers/alarm.cpp': 'alarm.cpp',
    'src/controllers/emergency.cpp': 'emergency.cpp',
    'src/display/oled.cpp': 'oled.cpp',
    'src/communication/wifi_manager.cpp': 'wifi_manager.cpp',
    'src/communication/telegram_manager.cpp': 'telegram_manager.cpp',
    'src/communication/web_server.cpp': 'web_server.cpp',
    # Main entrypoint
    'src/main.cpp': 'smart_fire_detection.ino'
}

# Regex to flatten #include "subfolder/header.h" -> #include "header.h"
pattern = re.compile(r'#include\s+"(?:sensors|controllers|display|communication)/([^"]+)"')

for rel_src, dst_name in file_map.items():
    src_path = os.path.join(src_dir, rel_src)
    dst_path = os.path.join(dst_dir, dst_name)
    if os.path.exists(src_path):
        with open(src_path, 'r', encoding='utf-8') as f:
            content = f.read()
        # Flatten include paths
        content = pattern.sub(r'#include "\1"', content)
        with open(dst_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Copied & prepared: {dst_name}")

# Also create secrets.h from secrets.example.h if not present
secrets_h = os.path.join(dst_dir, 'secrets.h')
if not os.path.exists(secrets_h):
    shutil.copy(os.path.join(dst_dir, 'secrets.example.h'), secrets_h)
    print("Created default secrets.h in arduino sketch")

print("\nArduino sketch setup complete!")
