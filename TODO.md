# TODO

## Content

- [ ] make transitions per property instead of "global"
- [ ] add font fallback outside the font stack (registered fonts and FontProvider)
- [ ] a11y: aria struct
- [ ] a11y: [accesskit](https://github.com/AccessKit/accesskit-c)
- [ ] a11y: kbd nav
- [ ] refactor Node::Paint (sorta done? not quite)
- [ ] refactor drawing api (its so hideous. should mirror skia more instead)
- [ ] optimize text (i feel like its slow)
- [ ] docs lol
- [ ] css text properties
- [ ] tab width prop
- [ ] dedupe styles in node
- [ ] skia backend and better platform specific to linux/wayland
- [ ] enhance texture api (`BackgroundImage("cat.png")`, dirty rects and other stuff)
- [ ] freetype module (should i though?)
- [ ] make color space a choice in anim (currently only oklab)
- [ ] flip for properties without anim
- [ ] widgets

## Done

- [x] decouple platform and renderer
- [x] added PlatformInterface to make platform surface creation (and platform in general) not so awkward
- [x] add texture support (freetype kinda depends on this)
