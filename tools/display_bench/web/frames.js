/* Thiết kế: docs/design/display/bench.md */

/*
 * Khung 1 bit theo bố cục của interface màn (ink_hal_display.h): bit 1 = trắng, gói theo hàng, điểm trái nhất ở bit
 * cao nhất, mỗi hàng width / 8 byte, toạ độ theo hướng gốc của panel (240 × 416). Hướng nhìn ngang chép từ put_pixel()
 * trong ink_ui.c, để ảnh trên trang khớp ảnh app sẽ vẽ.
 */

/* ---- Cỡ màn ---- */

export const PANEL_WIDTH = 240;
export const PANEL_HEIGHT = 416;
export const ROW_BYTES = PANEL_WIDTH / 8;
export const FRAME_BYTES = ROW_BYTES * PANEL_HEIGHT;

export const Orientation = Object.freeze({
    PORTRAIT: 'portrait',       /* hướng gốc của panel */
    LANDSCAPE_90: 'landscape90',   /* INK_UI_ROTATION_90 */
    LANDSCAPE_270: 'landscape270', /* INK_UI_ROTATION_270 */
});

export function viewSize(orientation) {
    return orientation === Orientation.PORTRAIT ? {width: PANEL_WIDTH, height: PANEL_HEIGHT} : {width: PANEL_HEIGHT, height: PANEL_WIDTH};
}

/* Điểm (x, y) của hướng nhìn → (column, row) của panel. */
export function viewToPanel(orientation, x, y) {
    if (orientation === Orientation.LANDSCAPE_90) { return {column: y, row: PANEL_HEIGHT - 1 - x}; }
    if (orientation === Orientation.LANDSCAPE_270) { return {column: PANEL_WIDTH - 1 - y, row: x}; }
    return {column: x, row: y};
}

/* ---- Đọc ghi điểm ---- */

export function createFrame(white = true) {
    return new Uint8Array(FRAME_BYTES).fill(white ? 0xff : 0x00);
}

export function isWhite(frame, column, row) {
    return (frame[row * ROW_BYTES + (column >> 3)] & (0x80 >> (column & 7))) !== 0;
}

export function setPanelPixel(frame, column, row, white) {
    const index = row * ROW_BYTES + (column >> 3);
    const mask = 0x80 >> (column & 7);
    frame[index] = white ? frame[index] | mask : frame[index] & ~mask;
}

export function setViewPixel(frame, orientation, x, y, white) {
    const {width, height} = viewSize(orientation);
    if (x < 0 || y < 0 || x >= width || y >= height) { return; }
    const {column, row} = viewToPanel(orientation, x, y);
    setPanelPixel(frame, column, row, white);
}

export function fillViewRect(frame, orientation, x, y, width, height, white) {
    for (let row = y; row < y + height; row++) {
        for (let column = x; column < x + width; column++) { setViewPixel(frame, orientation, column, row, white); }
    }
}

/* Hình chữ nhật trong hướng nhìn → window theo toạ độ panel. */
export function viewRectToPanel(orientation, rect) {
    const corners = [
        viewToPanel(orientation, rect.x, rect.y),
        viewToPanel(orientation, rect.x + rect.width - 1, rect.y + rect.height - 1),
    ];
    const left = Math.min(corners[0].column, corners[1].column);
    const top = Math.min(corners[0].row, corners[1].row);
    return {
        x: left,
        y: top,
        width: Math.abs(corners[0].column - corners[1].column) + 1,
        height: Math.abs(corners[0].row - corners[1].row) + 1,
    };
}

/* Window của panel → hình chữ nhật trong hướng nhìn (để vẽ khung window lên ảnh xem trước). */
export function panelRectToView(orientation, rect) {
    if (orientation === Orientation.PORTRAIT) { return {...rect}; }
    if (orientation === Orientation.LANDSCAPE_90) {
        return {x: PANEL_HEIGHT - rect.y - rect.height, y: rect.x, width: rect.height, height: rect.width};
    }
    return {x: rect.y, y: PANEL_WIDTH - rect.x - rect.width, width: rect.height, height: rect.width};
}

/* ---- Ảnh → mức xám ---- */

/*
 * Vẽ ảnh vào canvas cỡ hướng nhìn, trả mảng độ sáng 0–1 (Float32Array, mỗi điểm một số).
 * fit: 'contain' (giữ tỉ lệ, viền trắng), 'cover' (giữ tỉ lệ, cắt), 'stretch'.
 */
export function imageToGray(image, orientation, options) {
    const {width, height} = viewSize(orientation);
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const context = canvas.getContext('2d', {willReadFrequently: true});
    context.fillStyle = '#ffffff';
    context.fillRect(0, 0, width, height);
    const scaleX = width / image.width;
    const scaleY = height / image.height;
    let scale = Math.min(scaleX, scaleY);
    if (options.fit === 'cover') { scale = Math.max(scaleX, scaleY); }
    let drawWidth = image.width * scale;
    let drawHeight = image.height * scale;
    if (options.fit === 'stretch') {
        drawWidth = width;
        drawHeight = height;
    }
    context.imageSmoothingQuality = 'high';
    context.drawImage(image, (width - drawWidth) / 2, (height - drawHeight) / 2, drawWidth, drawHeight);
    return pixelsToGray(context.getImageData(0, 0, width, height).data, options);
}

/* Điểm RGBA → độ sáng có chỉnh sáng, tương phản, gamma, đảo màu. */
export function pixelsToGray(pixels, options) {
    const gray = new Float32Array(pixels.length / 4);
    const brightness = options.brightness ?? 0;
    const contrast = options.contrast ?? 1;
    const gamma = options.gamma ?? 1;
    for (let i = 0; i < gray.length; i++) {
        const alpha = pixels[i * 4 + 3] / 255;
        const luminance = (0.2126 * pixels[i * 4] + 0.7152 * pixels[i * 4 + 1] + 0.0722 * pixels[i * 4 + 2]) / 255;
        let value = luminance * alpha + (1 - alpha);
        value = (value - 0.5) * contrast + 0.5 + brightness;
        value = Math.pow(Math.min(1, Math.max(0, value)), gamma);
        gray[i] = options.invert ? 1 - value : value;
    }
    return gray;
}

/* ---- Dither: mức xám → 1 bit ---- */

export const DitherMethod = Object.freeze({
    THRESHOLD: 'threshold',
    FLOYD_STEINBERG: 'floyd-steinberg',
    ATKINSON: 'atkinson',
    BAYER_4: 'bayer4',
    BAYER_8: 'bayer8',
});

export const DITHER_NAMES = {
    [DitherMethod.THRESHOLD]: 'Ngưỡng',
    [DitherMethod.FLOYD_STEINBERG]: 'Floyd–Steinberg',
    [DitherMethod.ATKINSON]: 'Atkinson',
    [DitherMethod.BAYER_4]: 'Bayer 4×4',
    [DitherMethod.BAYER_8]: 'Bayer 8×8',
};

/* Phần sai số chia cho điểm bên cạnh: [dx, dy, tỉ lệ]. */
const ERROR_DIFFUSION = {
    [DitherMethod.FLOYD_STEINBERG]: [[1, 0, 7 / 16], [-1, 1, 3 / 16], [0, 1, 5 / 16], [1, 1, 1 / 16]],
    [DitherMethod.ATKINSON]: [[1, 0, 1 / 8], [2, 0, 1 / 8], [-1, 1, 1 / 8], [0, 1, 1 / 8], [1, 1, 1 / 8], [0, 2, 1 / 8]],
};

function bayerMatrix(size) {
    let matrix = [[0]];
    while (matrix.length < size) {
        const length = matrix.length;
        const next = Array.from({length: length * 2}, () => new Array(length * 2));
        for (let y = 0; y < length; y++) {
            for (let x = 0; x < length; x++) {
                const value = matrix[y][x] * 4;
                next[y][x] = value;
                next[y][x + length] = value + 2;
                next[y + length][x] = value + 3;
                next[y + length][x + length] = value + 1;
            }
        }
        matrix = next;
    }
    return matrix.map((row) => row.map((value) => (value + 0.5) / (size * size)));
}

/* Mức xám cỡ hướng nhìn → khung panel. */
export function grayToFrame(gray, orientation, method, threshold = 0.5) {
    const {width, height} = viewSize(orientation);
    const frame = createFrame(true);
    const work = Float32Array.from(gray);
    const diffusion = ERROR_DIFFUSION[method];
    const bayer = method === DitherMethod.BAYER_4 ? bayerMatrix(4) : method === DitherMethod.BAYER_8 ? bayerMatrix(8) : null;

    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const value = work[y * width + x];
            const limit = bayer ? bayer[y % bayer.length][x % bayer.length] : threshold;
            const white = value >= limit;
            setViewPixel(frame, orientation, x, y, white);
            if (!diffusion) { continue; }
            const error = value - (white ? 1 : 0);
            for (const [dx, dy, share] of diffusion) {
                const nx = x + dx;
                const ny = y + dy;
                if (nx >= 0 && nx < width && ny < height) { work[ny * width + nx] += error * share; }
            }
        }
    }
    return frame;
}

/* ---- Vẽ chữ, hình bằng canvas ---- */

/* Gọi draw(context, width, height) trên canvas trắng cỡ hướng nhìn, rồi đổi sang khung bằng ngưỡng. */
export function frameFromCanvas(orientation, draw) {
    const {width, height} = viewSize(orientation);
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const context = canvas.getContext('2d', {willReadFrequently: true});
    context.fillStyle = '#ffffff';
    context.fillRect(0, 0, width, height);
    context.fillStyle = '#000000';
    context.strokeStyle = '#000000';
    draw(context, width, height);
    const gray = pixelsToGray(context.getImageData(0, 0, width, height).data, {});
    return grayToFrame(gray, orientation, DitherMethod.THRESHOLD);
}

/* ---- Xem trước ---- */

/* Màu của biến CSS trên <html> thành [r, g, b]; theme sáng/tối đổi biến nên canvas vẽ lại theo. */
export function themeColor(name, fallback) {
    const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
    const match = /^#([0-9a-f]{6})$/i.exec(value);
    if (!match) { return fallback; }
    const number = parseInt(match[1], 16);
    return [number >> 16, (number >> 8) & 0xff, number & 0xff];
}

/*
 * Vẽ khung lên canvas theo hướng nhìn, phóng `scale` lần, theo devicePixelRatio để không mờ.
 * panelWindow (toạ độ panel) có thì vẽ khung quanh vùng đó.
 */
export function renderFrame(canvas, frame, orientation, scale, panelWindow = null) {
    const {width, height} = viewSize(orientation);
    const ratio = globalThis.devicePixelRatio ?? 1;
    canvas.width = Math.round(width * scale * ratio);
    canvas.height = Math.round(height * scale * ratio);
    canvas.style.width = `${width * scale}px`;
    canvas.style.height = `${height * scale}px`;

    const image = new ImageData(width, height);
    const ink = themeColor('--pixel-black', [31, 29, 26]);
    const paper = themeColor('--pixel-white', [255, 255, 255]);
    const empty = themeColor('--pixel-pad', [223, 217, 204]);
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const {column, row} = viewToPanel(orientation, x, y);
            const color = !frame ? empty : isWhite(frame, column, row) ? paper : ink;
            const index = (y * width + x) * 4;
            image.data.set(color, index);
            image.data[index + 3] = 255;
        }
    }
    const source = document.createElement('canvas');
    source.width = width;
    source.height = height;
    source.getContext('2d').putImageData(image, 0, 0);

    const context = canvas.getContext('2d');
    context.imageSmoothingEnabled = false;
    context.drawImage(source, 0, 0, canvas.width, canvas.height);
    if (panelWindow && panelWindow.width > 0 && panelWindow.height > 0) {
        const rect = panelRectToView(orientation, panelWindow);
        const pixel = scale * ratio;
        context.strokeStyle = `rgb(${themeColor('--brand-teal', [40, 126, 132]).join(',')})`;
        context.lineWidth = Math.max(2, ratio * 2);
        context.strokeRect(rect.x * pixel, rect.y * pixel, rect.width * pixel, rect.height * pixel);
    }
}
