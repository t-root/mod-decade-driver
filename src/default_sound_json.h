#pragma once

#include <Arduino.h>

// sound.json mặc định (dùng khi lần đầu nạp code, chưa có dữ liệu trong Preferences)
const char defaultSoundJson[] PROGMEM = R"rawliteral(
{
  "basic": {
    "in_card": "001.mp3",
    "out_card": "002.mp3",
    "open": "003.mp3",
    "close": "004.mp3",
    "error": "005.mp3",
    "kamen_ride": "006.mp3",
    "attack_ride": "007.mp3",
    "final_attack_ride": "008.mp3",
    "final_form_ride": "009.mp3",
    "touch": "010.mp3",
    "boot_sound": "011.mp3",
    "final_mode": "012.mp3"
  },
  "bmg": {"decade": "001.mp3"},
  "voice": {"decade": ["001.mp3", "002.mp3", "003.mp3", "004.mp3", "005.mp3", "006.mp3", "007.mp3"]},
  "card": {
    "01101110101": {
      "name": "Decade",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0010.mp3"
    },
    "11100111111": {
      "name": "W",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0011.mp3"
    },
    "11101000011": {
      "name": "OOO",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0012.mp3"
    },
    "11101001101": {
      "name": "Fourze",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0013.mp3"
    },
    "11101011111": {
      "name": "Wizard",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0014.mp3"
    },
    "11110010101": {
      "name": "Gaim",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0015.mp3"
    },
    "11110110111": {
      "name": "Drive",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0016.mp3"
    },
    "00000100101": {
      "name": "Ghost",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0017.mp3"
    },
    "00011000111": {
      "name": "Ex-Aid",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0018.mp3"
    },
    "00111011101": {
      "name": "Build",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0019.mp3"
    },
    "00111101101": {
      "name": "Zi-O",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0020.mp3"
    },

    "10111110111": {
      "name": "Blade",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0031.mp3"
    },
    "00000110111": {
      "name": "Den-O",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0034.mp3"
    },
    "11101010101": {
      "name": "Decade",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0036.mp3"
    },
    "10011101101": {
      "name": "W",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0037.mp3"
    },
    "10100000111": {
      "name": "OOO",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0038.mp3"
    },
    "10100110011": {
      "name": "Fourze",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0039.mp3"},
      "10110010011": {
        "name": "Wizard",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0040.mp3"
      },
      "11000011011": {
        "name": "Gaim",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0041.mp3"
      },
      "11010000001": {
        "name": "Drive",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0042.mp3"
      },
      "11011011111": {
        "name": "Ghost",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0043.mp3"
      },
      "11011110111": {
        "name": "Ex-Aid",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0044.mp3"
      },
      "11100010011": {
        "name": "Build",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0045.mp3"
      },
      "00001111011": {
        "name": "Zi-O",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0046.mp3"
      },
  
      "10000010111": {
        "name": "BOKUNI TSURRRETE MIRU",
        "type": "attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0053.mp3"
      },
      "10001000111": {
        "name": "TSUPPARI",
        "type": "final_attack_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0054.mp3"
      },
  
      "00000001101": {
        "name": "Decade Complete Form",
        "type": "final_form_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0061.mp3"
      },
      "01001010001": {
        "name": "W Cyclone Joker Xtreme",
        "type": "final_form_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0062.mp3"
      },
      "00010001110": {
        "name": "GRANDZI-O",
        "type": "final_form_ride",
        "voice": "decade",
        "bmg": "001.mp3",
        "file": "0063.mp3"
      }
    }
  }
)rawliteral";
