# digital_pomo

![splash](rsc/splash.gif)

Simple distraction free countdown timer to use with the [POMO Method](https://en.wikipedia.org/wiki/Pomodoro_Technique).

## Materials

- [LiliyGo Display S3 Touch](https://www.lilygo.cc/products/t-display-s3)
- [LiPo Battery with connection cable](https://www.bastelgarage.ch/lipo-akku-1000mah-jst-1-25-lithium-ion-polymer-fur-lora-ttgo)
- wires, switch
- 3d-printed enclosure

## How to make it

- 3d print enclosure from folder `3d_print/`
- connect switch with red wire of the battery and the esp32
- Install LiliyGo Dependencies, see [here](https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main?tab=readme-ov-file#4%EF%B8%8F%E2%83%A3--arduino-ide-manual-installation)
- Upload code/code.ino

## How to use

To charge the battery the switch must be in the ON position.
To Upload code the switch must be in the OFF position.

## Possible Improvements

- Add a small buzzer
- add visualisation methods
  - Color gradient over time (e.g. going from blue to green in the given time)
- add progress indication in countdown mode

## Acknowledgement

- The enclosure in `3d_print/` was adapted from [gzumwalt](https://www.instructables.com/member/gzumwalt/) and the original can be found [here](https://www.instructables.com/A-Case-and-Calculator-for-the-Lilygo-T-Display-S3-/)
- Parts of the code were taken from the [LiliyGo Examples](https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/examples)
- This project uses the [LVGL](https://lvgl.io/)
