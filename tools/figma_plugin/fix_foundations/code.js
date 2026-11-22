// Design: docs/design/figma-setup.md
// Run once in the InkFlash Figma file (Plugins > Development > Import plugin from manifest, then Run).
// Makes the grays neutral (R = G = B), caps radius/full at 255 (ink_ui radius is uint8_t) and switches the Typescale
// specimens (node 4:74) to the firmware fonts: Tiny5 at 1x/2x/3x and Unifont 16.
const GRAYS = {"color/gray-100": "F4F4F4", "color/gray-300": "D4D4D4", "color/gray-500": "717171", "color/gray-700": "3F3F3F", "color/gray-900": "181818"};

// Specimen text node id → [family, size, line height px, label node id, label text, find, replace]
const SPECIMENS = [
  ["4:77", "Tiny5", 24, 42, "4:76", "tiny5_24", null, null],
  ["4:80", "Tiny5", 16, 28, "4:79", "tiny5_16", "(Title 18px)", "(Title 16px)"],
  ["4:83", "Unifont", 16, 16, "4:82", "unifont_vi_16", null, null],
  ["4:89", "Tiny5", 8, 14, "4:88", "tiny5_8", null, null],
];

function hexToRgb(hex) {
  const n = parseInt(hex, 16);
  return {r: ((n >> 16) & 255) / 255, g: ((n >> 8) & 255) / 255, b: (n & 255) / 255};
}

async function fixVariables(log) {
  const variables = await figma.variables.getLocalVariablesAsync();
  const collections = await figma.variables.getLocalVariableCollectionsAsync();
  for (const v of variables) {
    const collection = collections.find((c) => c.id === v.variableCollectionId);
    if (collection.name !== "01_primitives") continue;
    const modeId = collection.defaultModeId;
    if (v.name in GRAYS) {
      v.setValueForMode(modeId, hexToRgb(GRAYS[v.name]));
      log.push(`${v.name} = #${GRAYS[v.name]}`);
    } else if (v.name === "radius/full") {
      v.setValueForMode(modeId, 255);
      log.push("radius/full = 255");
    }
  }
}

async function fixText(log) {
  for (const [id, family, size, lineHeight, labelId, label, find, replace] of SPECIMENS) {
    const node = await figma.getNodeByIdAsync(id);
    const labelNode = await figma.getNodeByIdAsync(labelId);
    if (!node || node.type !== "TEXT") { log.push(`missing text ${id}`); continue; }
    const font = {family, style: "Regular"};
    try {
      await figma.loadFontAsync(font);
      await figma.loadFontAsync(node.fontName);
    } catch (e) {
      log.push(`font ${family} not available (${e.message}); skipped ${id}`);
      continue;
    }
    node.fontName = font;
    node.fontSize = size;
    node.lineHeight = {unit: "PIXELS", value: lineHeight};
    node.letterSpacing = {unit: "PIXELS", value: 0};
    if (find && node.characters.includes(find)) node.characters = node.characters.replace(find, replace);
    if (labelNode && labelNode.type === "TEXT") {
      await figma.loadFontAsync(labelNode.fontName);
      labelNode.characters = label;
    }
    log.push(`${id}: ${family} ${size}/${lineHeight}`);
  }
  log.push("fixed_10 row (4:85, 4:86): no Figma font for X11 6x10 yet; left unchanged");
}

(async () => {
  const log = [];
  await fixVariables(log);
  await fixText(log);
  console.log(log.join("\n"));
  figma.closePlugin(`Done: ${log.length} steps. See the console for details.`);
})();
