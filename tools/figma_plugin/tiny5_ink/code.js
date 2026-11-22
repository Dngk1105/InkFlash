// Design: docs/design/figma-setup.md
// Run in the InkFlash Figma desktop app after installing the fonts "Tiny5 Ink" and "Fixed 6x10"
// (tools/figma2ink/make_u8g2_ttf.py). Sets the text styles of these fonts to the size and line height of
// docs/design/figma-setup.md (one em = 16 font pixels) and re-links two layers of foundations_portrait that lost
// their style.
// A one-line layer of fixed width that wraps after a font change (Tiny5 Ink has no kerning, so a line is 1-2 px wider
// than with Google's Tiny5) is switched to auto width (Hug) on every page. Running again changes nothing more.
// Text style → [Figma family, font size px, line height px].
const STYLES = {
  tiny5_8: ["Tiny5 Ink", 16, 14],
  tiny5_16: ["Tiny5 Ink", 32, 28],
  tiny5_24: ["Tiny5 Ink", 48, 42],
  fixed_10: ["Fixed 6x10", 16, 10],
};
// Layers of the Foundations page that must use tiny5_8: "mascot/outline" label and the "sample tiny5_8" line.
const RELINK = [["57:38", "tiny5_8"], ["57:99", "tiny5_8"]];

(async () => {
  const log = [];
  try {
    for (const family of new Set(Object.values(STYLES).map((s) => s[0]))) await figma.loadFontAsync({family, style: "Regular"});
    const styles = await figma.getLocalTextStylesAsync();
    const byName = Object.fromEntries(styles.map((s) => [s.name, s]));
    for (const [name, [family, size, line]] of Object.entries(STYLES)) {
      const style = byName[name];
      if (!style) { log.push(`missing style ${name}`); continue; }
      style.fontName = {family, style: "Regular"};
      style.fontSize = size;
      style.lineHeight = {unit: "PIXELS", value: line};
      style.letterSpacing = {unit: "PIXELS", value: 0};
      log.push(`${name}: ${family} ${size}/${line}`);
    }
    for (const s of styles) await figma.loadFontAsync(s.fontName);
    for (const [id, name] of RELINK) {
      const node = await figma.getNodeByIdAsync(id);
      if (!node || node.type !== "TEXT" || !byName[name] || node.textStyleId === byName[name].id) continue;
      if (typeof node.fontName !== "symbol") await figma.loadFontAsync(node.fontName);
      await node.setTextStyleIdAsync(byName[name].id);
      log.push(`${id} ${node.name} -> ${name}`);
    }
    const lineOf = Object.fromEntries(Object.entries(STYLES).filter(([n]) => byName[n]).map(([n, s]) => [byName[n].id, s[2]]));
    let hugged = 0;
    let relaid = 0;
    for (const page of figma.root.children) {
      await page.loadAsync();
      for (const node of page.findAllWithCriteria({types: ["TEXT"]})) {
        const line = lineOf[node.textStyleId];
        if (!line) continue;
        // Figma keeps the glyph outlines laid out with the font file it had; after a font file changes (same family
        // name) the export and other viewers still show the old glyphs until the text is laid out again.
        const text = node.characters;
        node.characters = text + " ";
        node.characters = text;
        relaid++;
        if (node.height <= line || text.includes("\n") || node.textTruncation === "ENDING") continue;
        node.textAutoResize = "WIDTH_AND_HEIGHT";
        hugged++;
      }
    }
    log.push(`${relaid} layers laid out again, ${hugged} wrapped layers set to auto width`);
    console.log(log.join("\n"));
    figma.closePlugin(`Done: ${log.join("; ")}`);
  } catch (e) {
    console.error(e);
    figma.closePlugin(`Failed: ${e.message}`);
  }
})();
