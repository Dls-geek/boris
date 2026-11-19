# Boris

Boris is a parrot who walks on the Omarchy desktop. Click him and he hops. Drag him where you want. Right-click him to open his settings window: size, picture, attention, and the rest of his setup live there, and a size change shows on him immediately.

Architecture for glance-at-cursor and layer-shell companion habits follows [Nala](https://github.com/yappologistic/Nala) (MIT). Boris keeps his own parrot, settings window, and avatar import — not Nala’s SDF body.

He stays out of the way: the window is only as big as he is, and the rest of the screen still takes your clicks. While he is walking he does not take the keyboard. Opening Boris again brings the settings window forward.

![Boris mid-step](screenshot.png)

## Actions

| Action | What it does |
| --- | --- |
| Click | Hop |
| Drag | Move him. Drop him near the floor and he walks again. Drop him higher and he perches. |
| Right-click | Open his settings window |
| Size slider | Change him from 40% to 200%. It is saved for the next launch |
| Walk the floor | Leave a perch and roam |
| Space | Hop, while the settings window is focused |
| Q | Quit, while the settings window is focused |
| Escape | Close the settings window |
| Close | Close the settings window. He keeps walking |
| ? | Show or hide the key list |

## Avatars

A picture can be PNG, JPEG, WebP, SVG, or GIF. A frames folder needs `idle.png`. It may also contain `walk-a.png`, `walk-b.png`, and `talk.png` (WebP, SVG, JPEG, and GIF work with the same names). Boris copies them. He does not change the originals.

## Build

```sh
bin/build
build/boris
```

`bin/test` runs the offscreen checks. `bin/install` packages him for the launcher. `boris --preview screenshot.png` draws him without a display.
