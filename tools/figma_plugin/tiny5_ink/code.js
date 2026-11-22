// Design: docs/design/figma-setup.md
// Run once in the InkFlash Figma desktop app after installing the font "Tiny5 Ink" (tools/figma2ink/make_u8g2_ttf.py).
// Sets the text styles tiny5_8/16/24 to Tiny5 Ink 14/28/42 px (line height = size: one em of Tiny5 Ink is one firmware
// line) and re-links two layers of foundations_portrait that lost their style.
// Tiny5 Ink has no kerning, so a line is 1-2 px wider than with Google's Tiny5: a one-line tiny5_* layer of fixed width
// wraps. On every page such a layer is switched to auto width (Hug). Running again changes nothing more.
const SIZES = {tiny5_8: 14, tiny5_16: 28, tiny5_24: 42};
const FAMILY = "Tiny5 Ink";
// Layers of the Foundations page that must use tiny5_8: "mascot/outline" label and the "sample tiny5_8" line.
const RELINK = [["57:38", "tiny5_8"], ["57:99", "tiny5_8"]];

(async () => {
  const log = [];
  try {
    await figma.loadFontAsync({family: FAMILY, style: "Regular"});
    const styles = await figma.getLocalTextStylesAsync();
    const byName = Object.fromEntries(styles.map((s) => [s.name, s]));
    for (const [name, px] of Object.entries(SIZES)) {
      const style = byName[name];
      if (!style) { log.push(`missing style ${name}`); continue; }
      style.fontName = {family: FAMILY, style: "Regular"};
      style.fontSize = px;
      style.lineHeight = {unit: "PIXELS", value: px};
      style.letterSpacing = {unit: "PIXELS", value: 0};
      log.push(`${name}: ${FAMILY} ${px}/${px}`);
    }
    for (const s of styles) await figma.loadFontAsync(s.fontName);
    for (const [id, name] of RELINK) {
      const node = await figma.getNodeByIdAsync(id);
      if (!node || node.type !== "TEXT" || !byName[name]) { log.push(`skipped ${id}`); continue; }
      if (typeof node.fontName !== "symbol") await figma.loadFontAsync(node.fontName);
      await node.setTextStyleIdAsync(byName[name].id);
      log.push(`${id} ${node.name} -> ${name}`);
    }
    const tinyLine = Object.fromEntries(Object.keys(SIZES).filter((n) => byName[n]).map((n) => [byName[n].id, SIZES[n]]));
    let hugged = 0;
    for (const page of figma.root.children) {
      await page.loadAsync();
      for (const node of page.findAllWithCriteria({types: ["TEXT"]})) {
        const line = tinyLine[node.textStyleId];
        if (!line || node.height <= line || node.characters.includes("\n") || node.textTruncation === "ENDING") continue;
        node.textAutoResize = "WIDTH_AND_HEIGHT";
        hugged++;
      }
    }
    log.push(`${hugged} wrapped tiny5 layers set to auto width`);
    console.log(log.join("\n"));
    figma.closePlugin(`Done: ${log.length} changes. See the console for details.`);
  } catch (e) {
    console.error(e);
    figma.closePlugin(`Failed: ${e.message}`);
  }
})();
