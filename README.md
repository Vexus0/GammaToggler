# Gamma Toggler
<p align="center"><img width="256" height="256" alt="GammaToggler" src="https://github.com/user-attachments/assets/05e57a84-8c74-441a-bd9b-e0802c69ce7a" /></p>

**Gamma Toggler** is a lightweight, easy to use program that will toggle your display's gamma between the default value (1.0) and another value that you can custom set.

The inspiration for this project comes from games such as Rust and Foxhole which have extremely dark night-time visuals. Players who adjust their gamma settings achieve a significant advantage over players who do not, resulting in unfair gameplay. 

To compound the problem, changing gamma tends to be cumbersome for most users when using existing options like the NVIDIA Control Panel, AutoIt, or RivaTuner to achieve the same results.

Gamma Toggler runs in your system tray and binds a single, user-defined hotkey (default: F10), as well as setting a user-defined gamma value (default: 2.8), to enable you to switch between gamma values at the press of a key.

## Features
1. Toggle screen gamma between default (1.0) and a custom value.
2. Set a custom global hotkey to trigger the gamma toggle.
3. User-friendly system tray icon with a context menu for configuration.
4. Settings are saved to a config.ini file in the same directory as the application.
5. Single, lightweight executable with no dependencies.

## Getting Started
### Installation
- Download the latest release (currently in .zip format)
- Extract the .exe into a preferred folder, ex. `C:\Users\<username>\Desktop\` or `C:\Users\<username>\Downloads\`
- Navigate to the `GammaToggler` folder
- Run `GamaToggler.exe` to launch the application
- Check your system tray (typically at the bottom right near your clock) for the application icon

### Usage
- After running GammaToggler.exe, a new system tray icon will appear
- Out of the box, you can press F10 by default to toggle gamma to 2.8 and back to 1.0
- Exiting the app resets gamma to 1.0

You may also configure the hotkey and gamma values from the system tray icon.
- Right-click on the system tray icon
- Select `Set Gamma...` to open a dialog to set gamma
- Select `Set Hotkey...` to open a dialog to set hotkey
- Select `Exit` to quit the program and reset default gamma
- *If you set custom values, a `config.ini` file will be created in the folder with the executable, saving your settings for the next time you run the application

### What is gamma?
Gamma is a way to increase the color intensity of a pixel programmatically to achieve brightness ranges that appeal to the human eye. In contrast to simple brightness, gamma scales each pixel on an exponential curve, spreading out darker ranges along the curve to reveal detail in the resulting image. Where brightness settings increase intensity linearly, the exponential curve of gamma allows bright pixels to "clump up" at the top range of intensity, while the darker pixels "spread out" along the intensity curve, revealing the otherwise subtle differences in dark-pixel intensity.

To learn more, check out this excellent article: [https://www.cambridgeincolour.com/tutorials/gamma-correction.htm](https://www.cambridgeincolour.com/tutorials/gamma-correction.htm)

### Prerequisites
- Gamma Toggler is currently a Windows-only application
- A `config.ini` file will be generated in the application directory if custom settings are used

### Limitations
- Gamma ranges of `0.23 to 4.45` are generally valid; there is a Windows limitation beyond these ranges to prevent washout
- You can further "double down" on gamma by adjusting hardware gamma, such as with NVIDIA Control Panel settings, should you feel you need more detail
- This allows you to set your "default" gamma through hardware, and use Gamma Toggler to still achieve the desired effect when needed
