// Design: docs/design/figma-setup.md
// Run in the InkFlash Figma desktop app (Plugins > Development > Import plugin from manifest, then Run). The fonts
// Unifont and Fixed 6x10 are local fonts, so the plugin must run where they are installed.
// Builds foundations_2_portrait and foundations_2_landscape on page "01 Foundations" from the two foundations frames:
// only what ink_ui and figma2ink draw today.
//   06 DITHER  gray fill → 4 x 4 Bayer pattern of ink_ui_fill(), anchored at the frame origin like the canvas
//   07 SELECT  ways to show a selected item with existing primitives: inverse fill, 2 px outline
//   08 TEXT    align start/center/end, max_lines with "…", line_gap from a smaller line height
//   09 ICON    INK_UI_BITMAP_FORMAT_MASK1: 1 = paint, 0 = left untouched
// NOTE: running again deletes and rebuilds frames named foundations_2_*; edits made inside them are lost.
const PAGE_NAME = "01 Foundations";
const SOURCES = [["foundations_portrait", "foundations_2_portrait", false], ["foundations_landscape", "foundations_2_landscape", true]];
const GRAYS = ["gray-100", "gray-300", "gray-500", "gray-700", "gray-900"];
const BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]; // s_bayer4x4 in ink_ui_canvas.c
const ICON = [
  "................",
  "....##########..",
  "....#........#..",
  "..##########.#..",
  "..#........#.#..",
  "..#........#.#..",
  "..#..####..#.#..",
  "..#........#.#..",
  "..#..###...#.#..",
  "..#........#.#..",
  "..#........###..",
  "..#........#....",
  "..##########....",
  "................",
  "................",
  "................",
];

let V = {};
let S = {};

function paint(name) {
  if (!V[name]) throw new Error(`variable ${name} not found`);
  return figma.variables.setBoundVariableForPaint({type: "SOLID", color: {r: 0, g: 0, b: 0}}, "color", V[name]);
}

function autoLayout(direction, name, props) {
  const f = figma.createFrame();
  f.name = name;
  f.layoutMode = direction;
  f.primaryAxisSizingMode = "AUTO";
  f.counterAxisSizingMode = "AUTO";
  f.fills = [];
  f.clipsContent = false;
  f.itemSpacing = 0;
  if (direction === "HORIZONTAL") f.counterAxisAlignItems = "CENTER";
  Object.assign(f, props || {});
  return f;
}

async function text(chars, style, fill, name) {
  if (!S[style]) throw new Error(`text style ${style} not found`);
  const t = figma.createText();
  await t.setTextStyleIdAsync(S[style].id);
  t.characters = chars;
  t.fills = [paint(fill)];
  t.name = name || chars;
  return t;
}

// Draw lit pixels as one flattened vector inside `frame` (fixed size); isOn(x, y) in frame pixels.
function drawPixels(frame, isOn, fillVar) {
  for (const c of [...frame.children]) c.remove();
  const rects = [];
  for (let y = 0; y < frame.height; y++) {
    let x = 0;
    while (x < frame.width) {
      if (!isOn(x, y)) { x++; continue; }
      let end = x;
      while (end < frame.width && isOn(end, y)) end++;
      const r = figma.createRectangle();
      r.resize(end - x, 1);
      frame.appendChild(r);
      r.x = x;
      r.y = y;
      rects.push(r);
      x = end;
    }
  }
  if (rects.length) {
    const v = figma.flatten(rects, frame);
    v.name = "pixels";
    v.fills = [paint(fillVar)];
  }
}

function pixelFrame(name, size, bgVar) {
  const f = figma.createFrame();
  f.name = name;
  f.resize(size, size);
  f.clipsContent = true;
  f.fills = bgVar ? [paint(bgVar)] : [];
  return f;
}

// Gray level as ink_ui sees it: R of the variable (grays are R = G = B).
function grayLevel(name) {
  const value = Object.values(V["color/" + name].valuesByMode)[0];
  return Math.round(value.r * 255);
}

async function sectionDither(pending) {
  const s = autoLayout("VERTICAL", "06 dither");
  s.appendChild(await text("06 DITHER (FILL ONLY)", "tiny5_8", "text/primary", "06 DITHER"));
  const list = autoLayout("VERTICAL", "dither list", {itemSpacing: 2});
  s.appendChild(list);
  for (const g of GRAYS) {
    const level = grayLevel(g);
    const whites = BAYER.flat().filter((m) => level > m * 16 + 8).length;
    const row = autoLayout("HORIZONTAL", "row " + g, {itemSpacing: 4});
    const swatch = figma.createRectangle();
    swatch.name = "figma " + g;
    swatch.resize(16, 16);
    swatch.fills = [paint("color/" + g)];
    row.appendChild(swatch);
    const panel = pixelFrame("panel " + g, 16, "color/white");
    row.appendChild(panel);
    pending.push([panel, level]);
    row.appendChild(await text(`${g} ${level} ${whites}/16`, "tiny5_8", "text/primary"));
    list.appendChild(row);
  }
  return s;
}

async function sectionSelect() {
  const s = autoLayout("VERTICAL", "07 select");
  s.appendChild(await text("07 SELECT", "tiny5_8", "text/primary"));
  const list = autoLayout("VERTICAL", "select list", {itemSpacing: 2});
  s.appendChild(list);
  const items = [["normal", null, null, "text/primary"], ["inverse", "text/primary", null, "surface/bg"], ["outline 2", null, "border/bold", "text/primary"]];
  for (const [label, bg, stroke, fg] of items) {
    const row = autoLayout("HORIZONTAL", "item " + label, {paddingLeft: 4, paddingRight: 4, paddingTop: 2, paddingBottom: 2});
    row.fills = bg ? [paint(bg)] : [];
    if (stroke) {
      row.strokes = [paint(stroke)];
      row.strokeAlign = "INSIDE";
      row.setBoundVariable("strokeWeight", V["stroke/thick"]);
      for (const corner of ["topLeftRadius", "topRightRadius", "bottomLeftRadius", "bottomRightRadius"]) row.setBoundVariable(corner, V["radius/sm"]);
    }
    list.appendChild(row);
    row.layoutSizingHorizontal = "FILL";
    row.appendChild(await text("Thẻ mới", "unifont_vi_16", fg, "label"));
    row.appendChild(await text(" " + label, "tiny5_8", fg, "kind"));
  }
  return s;
}

async function sectionText() {
  const s = autoLayout("VERTICAL", "08 text");
  s.appendChild(await text("08 TEXT", "tiny5_8", "text/primary"));
  const list = autoLayout("VERTICAL", "text list", {itemSpacing: 2});
  s.appendChild(list);
  for (const [label, align] of [["Trái", "LEFT"], ["Giữa", "CENTER"], ["Phải", "RIGHT"]]) {
    const t = await text(label, "unifont_vi_16", "text/primary", "align " + align.toLowerCase());
    list.appendChild(t);
    t.resize(104, 16);
    t.textAutoResize = "HEIGHT";
    t.textAlignHorizontal = align;
  }
  const cut = await text("Một câu rất dài bị cắt bớt", "unifont_vi_16", "text/primary", "max_lines 1");
  list.appendChild(cut);
  cut.resize(104, 16);
  cut.textAutoResize = "HEIGHT";
  cut.textTruncation = "ENDING";
  cut.maxLines = 1;
  const gap = await text("Dòng một\nDòng hai", "unifont_vi_16", "text/primary", "line_gap -2");
  list.appendChild(gap);
  gap.lineHeight = {unit: "PIXELS", value: 14};
  return s;
}

async function sectionIcon() {
  const s = autoLayout("VERTICAL", "09 icon");
  s.appendChild(await text("09 ICON (MASK1)", "tiny5_8", "text/primary", "09 ICON"));
  const list = autoLayout("HORIZONTAL", "icon list", {itemSpacing: 8});
  s.appendChild(list);
  const lit = (x, y) => ICON[y][x] === "#";
  for (const [label, fg, bg] of [["paint", "text/primary", null], ["inverse", "surface/bg", "text/primary"]]) {
    const pair = autoLayout("HORIZONTAL", "pair " + label, {itemSpacing: 4});
    const icon = pixelFrame("icon/card " + label, 16, bg);
    drawPixels(icon, lit, fg);
    pair.appendChild(icon);
    pair.appendChild(await text(label, "tiny5_8", "text/primary"));
    list.appendChild(pair);
  }
  return s;
}

async function build(page, srcName, name, landscape, log) {
  const src = page.findChild((n) => n.name === srcName);
  if (!src) throw new Error(`frame ${srcName} not found on ${PAGE_NAME}`);
  const old = page.findChild((n) => n.name === name);
  if (old) old.remove();
  const f = src.clone();
  page.appendChild(f);
  f.name = name;
  f.x = src.x;
  f.y = 107 + 416 + 64;
  const header = f.findOne((n) => n.name === "header");
  const title = header.findOne((n) => n.type === "TEXT" && n.characters === "FOUNDATIONS");
  if (title) {
    await figma.loadFontAsync(title.fontName);
    title.characters = "FOUNDATIONS 2";
    title.name = "FOUNDATIONS 2";
    title.textAutoResize = "WIDTH_AND_HEIGHT";
  }
  const body = f.findOne((n) => n.name === "body");
  for (const c of [...body.children]) c.remove();
  const pending = [];
  const secs = [await sectionDither(pending), await sectionSelect(), await sectionText(), await sectionIcon()];
  if (!landscape) {
    for (const s of secs) {
      body.appendChild(s);
      s.layoutSizingHorizontal = "FILL";
    }
  } else {
    [[secs[0]], [secs[1], secs[3]], [secs[2]]].forEach((group, i) => {
      const column = autoLayout("VERTICAL", "col " + (i + 1), {itemSpacing: 4});
      body.appendChild(column);
      column.resize(128, column.height);
      column.layoutSizingHorizontal = "FIXED";
      column.layoutSizingVertical = "HUG";
      for (const s of group) {
        column.appendChild(s);
        s.layoutSizingHorizontal = "FILL";
      }
    });
  }
  // Layout is final now: draw each dither swatch with the pattern phase of the canvas origin (frame top left).
  const ox = f.absoluteTransform[0][2];
  const oy = f.absoluteTransform[1][2];
  for (const [panel, level] of pending) {
    const px = Math.round(panel.absoluteTransform[0][2] - ox);
    const py = Math.round(panel.absoluteTransform[1][2] - oy);
    drawPixels(panel, (x, y) => !(level > BAYER[(py + y) & 3][(px + x) & 3] * 16 + 8), "color/black");
  }
  log.push(`${name}: ${f.id}, ${secs.length} sections`);
}

(async () => {
  const log = [];
  try {
    const page = figma.root.children.find((p) => p.name === PAGE_NAME);
    if (!page) throw new Error(`page ${PAGE_NAME} not found`);
    await figma.setCurrentPageAsync(page);
    const variables = await figma.variables.getLocalVariablesAsync();
    V = Object.fromEntries(variables.map((v) => [v.name, v]));
    const styles = await figma.getLocalTextStylesAsync();
    S = Object.fromEntries(styles.map((s) => [s.name, s]));
    for (const s of styles) await figma.loadFontAsync(s.fontName);
    for (const [srcName, name, landscape] of SOURCES) await build(page, srcName, name, landscape, log);
    console.log(log.join("\n"));
    figma.closePlugin(`Done: ${log.join("; ")}`);
  } catch (e) {
    console.error(e);
    figma.closePlugin(`Failed: ${e.message}`);
  }
})();
