# Gesture Controlled Media Player

## Overview

Gesture Controlled Media Player is a project that lets you control media playback using hand gestures or physical motion instead of traditional buttons. Using Arduino (or compatible microcontroller) with sensors (e.g. accelerometer/gyroscope or other input), the system sends commands to a media player (e.g. VLC) for actions such as play, pause, next track, previous track, and volume control.

This makes media control more intuitive and hands-free — useful for smart home setups, car dashboards, accessibility, or creative installations.

---

## Features

- Play / Pause  
- Skip to Next / Previous track  
- Volume Up / Down  
- Gesture recognition via Arduino (or equivalent) sensors  
- Integration with media player software  

---

## Hardware Requirements

- Arduino board (e.g. Arduino Uno / other)  
- Sensors for motion detection (e.g. accelerometer, gyroscope, IMU)  
- USB connection to a computer (or wireless if modified)  
- Computer with VLC (or other media player) that can be controlled via commands/keyboard shortcuts  

---

## Software Requirements

- Arduino IDE for uploading the sensor/gesture sketch  
- Software/libraries for the sensors being used (e.g. MPU-6050 libraries)  
- A way to map Arduino input to media control events on the host computer (e.g. via serial communication)  
- Media player software like VLC that can respond to keyboard shortcuts or external control  

---

## Setup & Usage

1. Upload the Arduino code (`vlc.ino` or `yt.ino`) to your microcontroller.  
2. Connect the sensor(s) as specified in code.  
3. Get the host computer to listen to Arduino/serial input.  
4. Map recognized gestures to media control commands.  
5. Test gesture actions: e.g. wave hand → play/pause; tilt → volume change; etc.  

---

## Folder / File Structure

| File | Description |
|---|---|
| `vlc.ino` | Sketch for Arduino to send gestures/commands targeted to VLC media player |
| `yt.ino` | Sketch for Arduino targeting YouTube control or alternate media control |
| `woahthasgreat.txt` | Notes / observations *(*could be removed or refined*)* |

---

## Potential Improvements

- Add more gestures (swipe left/right, spread fingers, etc.)  
- Use robust filtering / smoothing of sensor data to avoid false triggers  
- Cross-platform support for media players beyond VLC  
- Wireless communication (Bluetooth / WiFi) for untethered control  
- Visual feedback (LEDs or on-screen) to show detected gesture  

---

## License

This project is open source. Feel free to adapt and expand it.  
(Include specific license if you want, e.g. MIT / Apache / GPL.)

---

## Contact

Developed by **Sameer** – [GitHub profile](https://github.com/sameer-at-git)  
For feedback, suggestions, or contributions, open an issue or submit a pull request.  
