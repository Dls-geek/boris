# কীভাবে একটি সলিড পেট অ্যাসেট বানাবেন

এই গাইডটি শূন্য থেকে একটি নতুন পেট (ডেস্কটপ কম্প্যানিয়ন) অ্যাসেট তৈরি করার ব্যবহারিক ধাপ। লক্ষ্য: এমন একটি পেট যে **হাঁটে, বসে, ঘুমায়, কথা বলে** এবং **উত্তেজিত হয়**। শেষে দেখানো হয়েছে Boris অ্যাপে কীভাবে সেই অ্যাসেট লোড/রেজিস্টার করতে হয়।

---

## ১) ধারণা — পেট কী করে?

একটি সলিড পেট মানে শুধু একটা ছবি নয়। এটি কয়েকটি **অ্যাকশন লুপ** নিয়ে গঠিত:

| অ্যাকশন | কখন দেখা যায় | অনুভূতি |
| --- | --- | --- |
| **walk** (হাঁটা) | মেঝেতে ঘোরাফেরা | দুই পা / দুই ফ্রেম বিকল্প |
| **sit** (বসা) | উঁচুতে perch / স্থির | শান্ত, মাথা নিচু বা ডানা গুটানো |
| **sleep** (ঘুম) | অনেকক্ষণ idle থাকলে | চোখ বন্ধ, ধীর শ্বাস |
| **talk** (কথা) | ক্লিক/রিঅ্যাক্ট / বার্তা | মুখ খোলা-বন্ধ |
| **excited** (উত্তেজনা) | হপ / আনন্দ / বিশেষ ইভেন্ট | লাফ, ডানা ঝাপট |

Boris-এ আজকের রানটাইম মুডগুলোর সাথে মিলিয়ে ভাবুন:

- হাঁটা → `walk`
- স্থির / idle → `idle` (sit-এর কাছাকাছি)
- ঘুম → `nap`
- কথা / প্রতিক্রিয়া → `react` + `talk` ফ্রেম
- উত্তেজনা → `react` / hop (excited স্প্রাইট)

আপনি অ্যাসেট **লেখার সময়** সম্পূর্ণ `pet.json` + স্প্রাইট শিট রাখবেন। Boris-এ **আনতে** হয় আলাদা ফ্রেম ফাইল বা নামযুক্ত ছবি/GIF হিসেবে (নিচে ধাপ ৬)।

---

## ২) সুপারিশকৃত ফোল্ডার কাঠামো

প্রতিটি পেটের জন্য আলাদা ফোল্ডার রাখুন। উদাহরণ নাম: `mint-parrot`

```text
pets/
  mint-parrot/
    metadata.json          # নাম, আইডি, লেখক, ভার্সন
    pet.json               # অ্যানিমেশন কনফিগ (অ্যাকশন → ফ্রেম রেঞ্জ)
    sprites/
      sheet.png            # মূল স্প্রাইট শিট (স্বচ্ছ PNG)
      sheet.gif            # (ঐচ্ছিক) পুরো শিট GIF — শুধু রেফারেন্স
    frames/                # শিট থেকে কেটে বের করা আলাদা ফ্রেম (Boris ইমপোর্টের জন্য)
      idle.png
      walk-a.png
      walk-b.png
      sit.png
      sleep-0.png
      sleep-1.png
      talk.png
      excited-0.png
      excited-1.png
      excited-2.png
    sounds/
      walk.ogg
      talk.ogg
      excited.ogg
      sleep.ogg
    preview.gif            # (ঐচ্ছিক) ক্যাটালগ প্রিভিউ — অ্যানিমেটেড GIF OK
    README.txt             # সংক্ষিপ্ত নোট
```

**কেন এভাবে?**

- `sheet.png` = এক জায়গায় সব ফ্রেম এডিট করা সহজ।
- `pet.json` = ফ্রেম সংখ্যা, FPS, লুপ, সাউন্ড, টেক্সট এক জায়গায়।
- `frames/` = Boris আজ যেভাবে `importFrames` / ছবি ইমপোর্ট করে, সেখানে সরাসরি দেওয়া যায়।
- `sounds/` = ভবিষ্যৎ বা আপনার নিজের প্লেয়ারের জন্য; এখনও Boris সাউন্ড প্লে করে না, কিন্তু অ্যাসেট সম্পূর্ণ রাখুন।

---

## ৩) প্রতিটি অ্যাকশন — ধাপে ধাপে (ফ্রেম, FPS, শিট লেআউট)

### স্প্রাইট শিটের সাধারণ নিয়ম

- **সেল সাইজ:** প্রতি ফ্রেম `128×128` px (ছোট পেট) অথবা `256×256` px (বড়/স্পষ্ট)। এক শিটে সব সেল একই সাইজ।
- **গ্রিড:** বাম → ডান, তারপর পরের সারি। ফাঁকা সেল রাখবেন না মাঝখানে।
- **ট্রান্সপারেন্সি:** ব্যাকগ্রাউন্ড সম্পূর্ণ আলফা `0`।
- **অ্যাঙ্কর:** পায়ের তলা সেলের নিচের কেন্দ্রে রাখুন, যাতে হাঁটার সময় “ভাসমান” না লাগে।

**উদাহরণ গ্রিড (সেল 128×128, মোট ৪ সারি × ৮ কলাম = ৩২ সেল):**

```text
সারি 0: walk     ফ্রেম 0–3     (কলাম 0..3)
সারি 0: sit      ফ্রেম 0–1     (কলাম 4..5)
সারি 0: (খালি)                 (কলাম 6..7)
সারি 1: sleep    ফ্রেম 0–3     (কলাম 0..3)
সারি 2: talk     ফ্রেম 0–3     (কলাম 0..3)
সারি 3: excited  ফ্রেম 0–5     (কলাম 0..5)
```

শিটের পিক্সেল সাইজ তখন: প্রস্থ `8×128 = 1024`, উচ্চতা `4×128 = 512`।

---

### ৩.১ Walking (হাঁটা)

| বিষয় | মান |
| --- | --- |
| ফ্রেম সংখ্যা | **৪** (মিনিমাম **২**: A/B) |
| FPS | **8** (ধীর হাঁটা ৬, দ্রুত ১০–১২) |
| লুপ | হ্যাঁ (`loop: true`) |
| শিটে সাজানো | সারি ০, কলাম ০–৩: পা বাম → ডান → বাম → ডান |

**কী আঁকবেন**

1. ফ্রেম ০: বাম পা সামনে।
2. ফ্রেম ১: মাঝামাঝি (দুই পা কাছাকাছি)।
3. ফ্রেম ২: ডান পা সামনে।
4. ফ্রেম ৩: আবার মাঝামাঝি।

**Boris-এ ন্যূনতম ম্যাপ:** শুধু দুই ফ্রেম কাটুন → `walk-a.png`, `walk-b.png`।

---

### ৩.২ Sitting (বসা)

| বিষয় | মান |
| --- | --- |
| ফ্রেম সংখ্যা | **২** (১ হলেও চলে; ২ হলে হালকা শ্বাস) |
| FPS | **2** |
| লুপ | হ্যাঁ |
| শিটে | সারি ০, কলাম ৪–৫ |

**কী আঁকবেন**

1. ফ্রেম ০: বসা, ডানা গুটানো।
2. ফ্রেম ১: বুক একটু ওঠানামা (১–২ px)।

Boris-এ আলাদা `sit` ইমপোর্ট নেই — `idle.png` হিসেবে বসার পোজ দিন (স্থির থাকলে এটিই দেখায়)।

---

### ৩.৩ Sleeping (ঘুম)

| বিষয় | মান |
| --- | --- |
| ফ্রেম সংখ্যা | **৪** |
| FPS | **2** |
| লুপ | হ্যাঁ |
| শিটে | সারি ১, কলাম ০–৩ |
| সাউন্ড | `sleep.ogg` (খুব আস্তে, লুপ) — ঐচ্ছিক |

**কী আঁকবেন**

1. চোখ আধা-বন্ধ।
2. চোখ বন্ধ, মাথা একটু নিচু।
3. “Z” বা ডানা নড়ে।
4. আবার গভীর ঘুম।

পরীক্ষার সময়: সেটিংসে **Sleep when idle** চালু রাখুন।

---

### ৩.৪ Talking (কথা)

| বিষয় | মান |
| --- | --- |
| ফ্রেম সংখ্যা | **৪** (মিনিমাম **২**: মুখ বন্ধ / খোলা) |
| FPS | **10** |
| লুপ | একবার বা ছোট লুপ (`loop: false` বা `repeat: 2`) |
| শিটে | সারি ২, কলাম ০–৩ |
| সাউন্ড | ফ্রেম ০-এ `talk.ogg` ট্রিগার |
| টেক্সট | `lines` অ্যারেতে ২–৪টা ছোট বাক্য |

**কী আঁকবেন**

1. মুখ বন্ধ।
2. মুখ আধা খোলা।
3. মুখ পুরো খোলা।
4. আবার বন্ধ।

**Boris ম্যাপ:** একটি ভালো “মুখ খোলা” ফ্রেম → `talk.png` (রিঅ্যাক্ট/হপে ব্যবহার)।

---

### ৩.৫ Excited (উত্তেজিত)

| বিষয় | মান |
| --- | --- |
| ফ্রেম সংখ্যা | **৬** |
| FPS | **12** |
| লুপ | একবার (`loop: false`) |
| শিটে | সারি ৩, কলাম ০–৫ |
| সাউন্ড | ফ্রেম ১-এ `excited.ogg` |

**কী আঁকবেন**

1. স্থির থেকে উঠতে শুরু।
2. লাফের চূড়া।
3. ডানা ওপরে।
4. ডানা নিচে।
5. আবার লাফ।
6. ল্যান্ড / হাসি পোজ।

---

## ৪) JSON কনফিগ স্কিমা + উদাহরণ

### `metadata.json`

```json
{
  "id": "mint-parrot",
  "name": "মিন্ট প্যারট",
  "name_en": "Mint Parrot",
  "author": "আপনার নাম",
  "version": "1.0.0",
  "cell_width": 128,
  "cell_height": 128,
  "sheet": "sprites/sheet.png",
  "preview": "preview.gif",
  "license": "MIT"
}
```

### `pet.json` — অ্যাকশন স্কিমা

প্রতিটি অ্যাকশনে এই ফিল্ডগুলো রাখুন:

| ফিল্ড | টাইপ | অর্থ |
| --- | --- | --- |
| `frames` | `[start, end]` | শিটের লিনিয়ার ইনডেক্স (০-ভিত্তিক, বাম→ডান তারপর নিচের সারি) |
| `fps` | number | ফ্রেম প্রতি সেকেন্ড |
| `loop` | bool | বারবার চলবে কি না |
| `sound` | object/null | `{ "file": "...", "at_frame": 0 }` |
| `lines` | string[] | কথার লাইন (talk-এর জন্য) |
| `boris_files` | object | Boris `frames/` ফাইলে ম্যাপ |

**সম্পূর্ণ নমুনা `pet.json`:**

```json
{
  "schema": 1,
  "id": "mint-parrot",
  "sheet": {
    "file": "sprites/sheet.png",
    "columns": 8,
    "rows": 4,
    "cell_width": 128,
    "cell_height": 128
  },
  "actions": {
    "walk": {
      "frames": [0, 3],
      "fps": 8,
      "loop": true,
      "sound": { "file": "sounds/walk.ogg", "at_frame": 0 },
      "lines": [],
      "boris_files": {
        "walk-a": "frames/walk-a.png",
        "walk-b": "frames/walk-b.png"
      }
    },
    "sit": {
      "frames": [4, 5],
      "fps": 2,
      "loop": true,
      "sound": null,
      "lines": [],
      "boris_files": {
        "idle": "frames/idle.png"
      }
    },
    "sleep": {
      "frames": [8, 11],
      "fps": 2,
      "loop": true,
      "sound": { "file": "sounds/sleep.ogg", "at_frame": 0 },
      "lines": [],
      "boris_files": {
        "sleep-0": "frames/sleep-0.png",
        "sleep-1": "frames/sleep-1.png"
      }
    },
    "talk": {
      "frames": [16, 19],
      "fps": 10,
      "loop": false,
      "sound": { "file": "sounds/talk.ogg", "at_frame": 0 },
      "lines": [
        "কী খবর, ক্যাপ্টেন?",
        "আজ সমুদ্র শান্ত!",
        "একটা ক্র্যাকার?"
      ],
      "boris_files": {
        "talk": "frames/talk.png"
      }
    },
    "excited": {
      "frames": [24, 29],
      "fps": 12,
      "loop": false,
      "sound": { "file": "sounds/excited.ogg", "at_frame": 1 },
      "lines": ["ইয়্যুহু!"],
      "boris_files": {
        "excited-0": "frames/excited-0.png",
        "excited-1": "frames/excited-1.png",
        "excited-2": "frames/excited-2.png"
      }
    }
  }
}
```

**ফ্রেম ইনডেক্স হিসাব:**  
`index = row * columns + column`  
উদাহরণ: সারি ১, কলাম ০ → `1*8+0 = 8` (sleep শুরু)।

---

## ৫) ছবির ফরম্যাট গাইডেন্স

| ব্যবহার | ফরম্যাট | নোট |
| --- | --- | --- |
| স্প্রাইট শিট (`sheet.png`) | **PNG + আলফা** | সবচেয়ে ভালো; লসলেস, স্বচ্ছতা ঠিক থাকে |
| আলাদা ফ্রেম (`frames/*.png`) | **PNG** | Boris `importFrames` ও কোয়ালিটির জন্য পছন্দনীয় |
| একক লুক / প্রিভিউ | **GIF** | অ্যানিমেশন OK; Boris `AnimatedImage` দিয়ে চালায় |
| JPEG | এড়িয়ে চলুন | স্বচ্ছতা নেই, পেটের কিনারা নোংরা হয় |
| SVG | সিম্পল আইকনের জন্য | জটিল পিক্সেল আর্টের জন্য PNG ভালো |

**সারাংশ:** শিট ও গেম ফ্রেম = PNG। ছোট অ্যানিমেটেড প্রিভিউ বা একক অ্যাভাটার লুক = GIF চলবে। Boris পিকার PNG/JPEG/WebP/SVG/**GIF** নেয়; ফ্রেম ফোল্ডারে একই এক্সটেনশনগুলো চলে, কিন্তু নাম `idle` / `walk-a` / `walk-b` / `talk` হতে হবে।

---

## ৬) Boris অ্যাপে নতুন পেট রেজিস্টার / লোড করা

### কোথায় ক্যাটালগ ও লোড হয়

| জিনিস | ফাইল / জায়গা |
| --- | --- |
| অ্যাভাটার স্টোর + ক্যাটালগ | `src/avatar.cpp`, `src/avatar.h` — `catalog()`, `addNamedPicture()`, `importFrames()`, `selectAvatar()` |
| ডিস্কে সেভ | `~/.local/share/boris/avatar/` — `avatar.json`, `library/index.json`, `library/*`, `frames/*` |
| ব্যাকএন্ড API | `src/backend.cpp` — `beginNamedAvatarImport()`, `saveNamedAvatar()`, `importFrames()`, `selectAvatar()` |
| Apps → Avatars পেজ | `src/AvatarsPage.qml` — ড্রাফট প্রিভিউ, সেভ, ক্যাটালগ, **Import frames folder** |
| Apps → Pet পেজ | `src/PetPage.qml` — বর্তমান লুক, সুইচ বাটন (`backend.avatar.catalog`) |
| ডেস্কটপ স্প্রাইট | `src/Main.qml` + `src/AvatarLook.qml` (ছবি/GIF), `src/PetSprite.qml` (বিল্ট-ইন প্যারট) |
| সেটিংস মেনু | `src/SettingsWindow.qml` → title bar **Apps** → `AppsMenu.qml` |

### পথ A — নামযুক্ত ছবি বা GIF (সহজ)

1. `bin/build` করে `./build/boris --settings` চালান।
2. Title bar-এ **Apps** → **Avatars**।
3. **Choose a picture** → আপনার `preview.gif` বা একটি ফ্রেম PNG।
4. নাম দিন (যেমন `মিন্ট প্যারট`) → **Save look**।
5. ক্যাটালগে ট্যাপ করে পরান।  
   → এটা `library/`-এ কপি হয় এবং `catalog`-এ দেখা যায় (`Pet` পেজেও)।

### পথ B — ফ্রেম ফোল্ডার (হাঁটা/কথার জন্য)

1. `pets/mint-parrot/frames/` তৈরি করুন অন্তত:
   - `idle.png` (**বাধ্যতামূলক**)
   - `walk-a.png`, `walk-b.png` (না থাকলে idle দিয়ে পূরণ)
   - `talk.png` (ঐচ্ছিক কিন্তু উত্তেজনা/রিঅ্যাক্টের জন্য ভালো)
2. Avatars পেজে **Import frames folder** চাপুন এবং সেই `frames/` ফোল্ডার বেছে নিন।
3. কোড পাথ: বাটন → `backend.importFrames()` → `Avatar::importFrames()` (`src/avatar.cpp`)।

### পথ C — শিট থেকে ফ্রেম কেটে নেওয়া (অ্যাসেট ওয়ার্কফ্লো)

1. `sheet.png` এডিট করুন (Aseprite / LibreSprite / GIMP)।
2. প্রতিটি সেল এক্সপোর্ট করে `frames/`-এ রাখুন (উপরের নামগুলো)।
3. `pet.json`-এর `boris_files` ম্যাপ আপডেট রাখুন যাতে পরে ভুল না হয়।
4. পথ B দিয়ে Boris-এ ইমপোর্ট করুন।

> **নোট:** বিল্ট-ইন হলুদ প্যারট `PetSprite.qml` দিয়ে আঁকা; আপনার কাস্টম লুক `kind === "picture"` বা `"frames"` হলে ইমেজ পাইপ দিয়ে যায়। ক্যাটালগে প্যারট এন্ট্রি সবসময় থাকে (`id: "parrot"`)।

---

## ৭) মিনিমাল ওয়ার্কিং উদাহরণ (কপি করে এডিট করুন)

### ফোল্ডার ট্রি

```text
pets/
  mint-parrot/
    metadata.json
    pet.json
    sprites/
      sheet.png
    frames/
      idle.png
      walk-a.png
      walk-b.png
      talk.png
    sounds/
      talk.ogg
    preview.gif
```

### কপি-পেস্ট `metadata.json`

```json
{
  "id": "mint-parrot",
  "name": "মিন্ট প্যারট",
  "name_en": "Mint Parrot",
  "author": "you",
  "version": "1.0.0",
  "cell_width": 128,
  "cell_height": 128,
  "sheet": "sprites/sheet.png",
  "preview": "preview.gif",
  "license": "MIT"
}
```

### কপি-পেস্ট মিনিমাল `pet.json`

```json
{
  "schema": 1,
  "id": "mint-parrot",
  "sheet": {
    "file": "sprites/sheet.png",
    "columns": 4,
    "rows": 2,
    "cell_width": 128,
    "cell_height": 128
  },
  "actions": {
    "walk": {
      "frames": [0, 1],
      "fps": 8,
      "loop": true,
      "sound": null,
      "lines": [],
      "boris_files": {
        "walk-a": "frames/walk-a.png",
        "walk-b": "frames/walk-b.png"
      }
    },
    "sit": {
      "frames": [2, 2],
      "fps": 1,
      "loop": true,
      "sound": null,
      "lines": [],
      "boris_files": { "idle": "frames/idle.png" }
    },
    "sleep": {
      "frames": [3, 3],
      "fps": 1,
      "loop": true,
      "sound": null,
      "lines": [],
      "boris_files": { "idle": "frames/idle.png" }
    },
    "talk": {
      "frames": [4, 5],
      "fps": 10,
      "loop": false,
      "sound": { "file": "sounds/talk.ogg", "at_frame": 0 },
      "lines": ["হ্যালো!", "আমি মিন্ট।"],
      "boris_files": { "talk": "frames/talk.png" }
    },
    "excited": {
      "frames": [6, 7],
      "fps": 12,
      "loop": false,
      "sound": null,
      "lines": ["ইয়্যু!"],
      "boris_files": { "talk": "frames/talk.png" }
    }
  }
}
```

মিনিমাল শিট লেআউট (৪×২):

```text
[ walk-a ] [ walk-b ] [ sit/idle ] [ sleep ]
[ talk-0 ] [ talk-1 ] [ excited-0 ] [ excited-1 ]
```

প্রথমে শুধু চারটে ফাইল দিয়েই Boris টেস্ট করতে পারেন: `idle.png`, `walk-a.png`, `walk-b.png`, `talk.png`।

---

## ৮) টেস্টিং চেকলিস্ট

অ্যাসেট রেডি হলে এই ক্রমে যাচাই করুন:

### Avatars — ড্রাফট প্রিভিউ

- [ ] **Apps → Avatars → Choose a picture** দিয়ে `preview.gif` বা একটি ফ্রেম বাছুন।
- [ ] ড্রাফট বক্সে অ্যানিমেশন/ছবি দেখা যাচ্ছে (`AvatarLook` / `AnimatedImage`)।
- [ ] নাম দিয়ে **Save look** — এরর মেসেজ নেই।

### Avatars — ক্যাটালগ

- [ ] **Your looks** তালিকায় নতুন নাম আছে।
- [ ] GIF হলে লেবেলে GIF দেখা যায়।
- [ ] ট্যাপ করলে **Wearing now** হয়।
- [ ] (ফ্রেম পথে) **Import frames folder** সফল; `idle` ছাড়া ফোল্ডার ব্যর্থ হয় — এটাই প্রত্যাশিত।

### Pet পেজ

- [ ] **Apps → Pet** — প্রিভিউতে বর্তমান লুক।
- [ ] **Current look** নাম ঠিক।
- [ ] **Switch look** দিয়ে প্যারট ↔ আপনার লুক বদল হয়।
- [ ] Follow cursor / Idle antics / Sleep when idle টগল কাজ করে।

### ডেস্কটপ

- [ ] পেট উইন্ডোতে নতুন লুক দেখা যায় (শুধু স্প্রাইট; ব্যাকগ্রাউন্ড ক্লিক থ্রু)।
- [ ] ক্লিক → হপ / রিঅ্যাক্ট (talk ফ্রেম থাকলে বদল)।
- [ ] হাঁটা চলাকালে walk A/B অদলবদল (ফ্রেম ইমপোর্ট থাকলে)।
- [ ] ড্র্যাগ করে perch → sit/idle পোজ; **Walk the floor** এ মেঝেতে ফেরত।
- [ ] অনেকক্ষণ না ছুঁয়ে Sleep when idle চালু থাকলে nap।
- [ ] সাইজ স্লাইডার ৪০–২০০% — টানার সময় সাইজ পিছনে লাফায় না।

### ফাইল সিস্টেম স্মোক টেস্ট

- [ ] `~/.local/share/boris/avatar/library/`-এ সেভ করা ফাইল আছে (ছবি/GIF পথ)।
- [ ] অথবা `.../avatar/frames/`-এ `idle` / `walk-a` / `walk-b` / `talk` আছে।
- [ ] অ্যাপ বন্ধ করে আবার খুললেও একই লুক ফিরে আসে।

---

## দ্রুত রেফারেন্স সংখ্যা

| অ্যাকশন | ফ্রেম | FPS | লুপ |
| --- | --- | --- | --- |
| walk | 4 (মিন 2) | 8 | হ্যাঁ |
| sit | 2 | 2 | হ্যাঁ |
| sleep | 4 | 2 | হ্যাঁ |
| talk | 4 | 10 | না / ছোট |
| excited | 6 | 12 | না |

| সেল | শিট উদাহরণ |
| --- | --- |
| 128×128 | 8×4 গ্রিড → ইমেজ 1024×512 |
| 256×256 | 8×4 গ্রিড → ইমেজ 2048×1024 |

---

## শেষ কথা

1. শিটে আঁকুন (PNG)।  
2. `pet.json`-এ ফ্রেম রেঞ্জ/FPS লিখুন।  
3. `frames/`-এ Boris নামে এক্সপোর্ট করুন।  
4. অ্যাপে **Apps → Avatars** দিয়ে ছবি/GIF বা ফ্রেম ফোল্ডার ইমপোর্ট করুন।  
5. **Pet** পেজ ও ডেস্কটপে চেকলিস্ট শেষ করুন।

এই ফাইলটি রিপোর রুটে রাখা হয়েছে: `HOW_TO_CREATE_A_SOLID_PET.md` — নিজে এডিট করে আপনার পরবর্তী পেটের টেমপ্লেট হিসেবে ব্যবহার করুন।
