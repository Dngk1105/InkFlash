/* Thiết kế: docs/design/display/bench.md */

/*
 * Bài test sinh khung trên PC. Mỗi bài trả một kế hoạch: các khung (gửi vào slot từ TEST_FIRST_SLOT), thứ tự phát,
 * kiểu vẽ, ms giữa hai khung, độ rộng viền. Trang gửi khung rồi gửi gói PLAY.
 */
import {Orientation, createFrame, fillViewRect, frameFromCanvas, grayToFrame, DitherMethod, viewSize} from './frames.js';

/* Slot 0–47 cho ảnh người dùng, 48–63 cho bài test. */
export const TEST_FIRST_SLOT = 48;
export const TEST_SLOT_COUNT = 16;

export const Kind = Object.freeze({AUTO: 0, PARTIAL: 1, FULL: 2});
export const KIND_NAMES = {[Kind.AUTO]: 'policy chọn', [Kind.PARTIAL]: 'partial', [Kind.FULL]: 'full'};

/* ---- Hình dùng chung ---- */

function centeredBox(orientation, boxWidth, boxHeight) {
    const {width, height} = viewSize(orientation);
    return {x: Math.round((width - boxWidth) / 2), y: Math.round((height - boxHeight) / 2), width: boxWidth, height: boxHeight};
}

function frameWithBox(orientation, box, black = true) {
    const frame = createFrame(true);
    fillViewRect(frame, orientation, box.x, box.y, box.width, box.height, !black);
    return frame;
}

function checkerboard(orientation, cell, phase) {
    return frameFromCanvas(orientation, (context, width, height) => {
        for (let y = 0; y < height; y += cell) {
            for (let x = 0; x < width; x += cell) {
                if (((x / cell + y / cell + phase) & 1) === 0) { context.fillRect(x, y, cell, cell); }
            }
        }
    });
}

const SAMPLE_TEXT = [
    'Flashcard: ôn tập ngắt quãng giúp nhớ lâu hơn.',
    'Mỗi lần lật thẻ, màn chỉ vẽ lại vùng đổi.',
    'E-ink giữ hình khi mất điện, gần như không tốn pin.',
    'Bóng mờ là dấu vết hình cũ còn lại sau partial.',
    'Full refresh làm sạch bóng mờ nhưng màn chớp.',
    'Bảng sóng quyết định tốc độ và độ sạch của mỗi lần vẽ.',
    'Nhiệt độ thấp làm mực chậm, cần sóng dài hơn.',
    'Viền xám xuất hiện ở mép vùng partial vẽ nhiều lần.',
];

function textPage(orientation, firstLine, fontSize) {
    return frameFromCanvas(orientation, (context, width, height) => {
        context.font = `${fontSize}px "Segoe UI", system-ui, sans-serif`;
        context.textBaseline = 'top';
        const lineHeight = Math.round(fontSize * 1.4);
        for (let line = 0; line * lineHeight + 8 < height; line++) {
            const text = SAMPLE_TEXT[(firstLine + line) % SAMPLE_TEXT.length];
            context.fillText(text, 8, 8 + line * lineHeight, width - 16);
        }
    });
}

function bigNumberPage(orientation, number) {
    return frameFromCanvas(orientation, (context, width, height) => {
        context.font = `bold ${Math.round(height * 0.5)}px "Segoe UI", system-ui, sans-serif`;
        context.textAlign = 'center';
        context.textBaseline = 'middle';
        context.fillText(String(number), width / 2, height / 2);
        context.font = '16px "Segoe UI", system-ui, sans-serif';
        context.fillText('Bấm A: trang sau, B: trang trước', width / 2, height - 20);
    });
}

/* ---- Danh sách bài ---- */

/*
 * params: {key, label, value, min, max, step?}. build(values, orientation) trả
 * {frames, order (index trong frames), kind, intervalMs, repeat, edgeMargin, note}.
 */
export const TESTS = [
    {
        id: 'flip_box',
        name: 'Lật ô',
        description: 'Ô 80 × 40 giữa màn đổi đen trắng mỗi lần. Đếm bằng mắt: lần nào cũng phải đổi (kiểm hai vùng RAM đổi vai).',
        params: [
            {key: 'count', label: 'Số lần lật', value: 10, min: 1, max: 200},
            {key: 'interval', label: 'ms giữa hai lần', value: 1000, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {
                frames: [createFrame(true), frameWithBox(orientation, box)],
                order: Array.from({length: values.count}, (_, index) => (index + 1) % 2),
                kind: Kind.AUTO,
                intervalMs: values.interval,
                repeat: 1,
                edgeMargin: 0,
            };
        },
    },
    {
        id: 'checker_ghost',
        name: 'Ô cờ bóng mờ',
        description: 'Ô cờ đảo pha N lần bằng partial, rồi về trắng. Chấm bóng mờ của ô cờ còn lại trên nền trắng.',
        params: [
            {key: 'cycles', label: 'Số lần đảo', value: 20, min: 1, max: 60},
            {key: 'cell', label: 'Cỡ ô (điểm)', value: 16, min: 2, max: 64},
            {key: 'interval', label: 'ms giữa hai lần', value: 200, min: 0, max: 10000, step: 50},
        ],
        build(values, orientation) {
            const order = Array.from({length: values.cycles}, (_, index) => index % 2);
            order.push(2);
            return {
                frames: [checkerboard(orientation, values.cell, 0), checkerboard(orientation, values.cell, 1), createFrame(true)],
                order,
                kind: Kind.PARTIAL,
                intervalMs: values.interval,
                repeat: 1,
                edgeMargin: 0,
                note: 'Toàn bộ là partial cả màn; vẽ full bằng tay để so lúc sạch.',
            };
        },
    },
    {
        id: 'window_sweep',
        name: 'Quét cỡ window',
        description: 'Ô đen nhiều cỡ, mỗi ô xen một khung trắng. Xem thời gian spi và panel đổi theo cỡ window.',
        params: [{key: 'interval', label: 'ms giữa hai lần', value: 800, min: 0, max: 10000, step: 100}],
        build(values, orientation) {
            const sizes = [[8, 8], [32, 32], [80, 40], [160, 120], [400, 220]];
            const frames = [createFrame(true)];
            const order = [];
            for (const [boxWidth, boxHeight] of sizes) {
                const {width, height} = viewSize(orientation);
                frames.push(frameWithBox(orientation, centeredBox(orientation, Math.min(boxWidth, width), Math.min(boxHeight, height))));
                order.push(frames.length - 1, 0);
            }
            return {frames, order, kind: Kind.AUTO, intervalMs: values.interval, repeat: 1, edgeMargin: 0};
        },
    },
    {
        id: 'text_scroll',
        name: 'Chữ cuộn',
        description: 'Trang chữ cuộn một dòng mỗi lần, như đọc thẻ. Chấm độ nét chữ và bóng của dòng cũ.',
        params: [
            {key: 'pages', label: 'Số trang', value: 8, min: 2, max: TEST_SLOT_COUNT},
            {key: 'fontSize', label: 'Cỡ chữ (điểm)', value: 18, min: 10, max: 40},
            {key: 'interval', label: 'ms giữa hai lần', value: 1000, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const frames = Array.from({length: values.pages}, (_, index) => textPage(orientation, index, values.fontSize));
            return {frames, order: frames.map((_, index) => index), kind: Kind.AUTO, intervalMs: values.interval, repeat: 1, edgeMargin: 0};
        },
    },
    {
        id: 'gray_ramp',
        name: 'Dải xám dither',
        description: 'Một khung có dải xám từ trắng tới đen, mỗi dải một cách dither. Vẽ full; chấm cách nào trông mịn nhất.',
        params: [],
        build(values, orientation) {
            const {width, height} = viewSize(orientation);
            const methods = [DitherMethod.THRESHOLD, DitherMethod.FLOYD_STEINBERG, DitherMethod.ATKINSON, DitherMethod.BAYER_4, DitherMethod.BAYER_8];
            const frame = createFrame(true);
            const band = Math.floor(height / methods.length);
            methods.forEach((method, index) => {
                const gray = new Float32Array(width * height).fill(1);
                for (let y = index * band; y < (index + 1) * band - 2; y++) {
                    for (let x = 0; x < width; x++) { gray[y * width + x] = 1 - x / (width - 1); }
                }
                const part = grayToFrame(gray, orientation, method);
                for (let i = 0; i < frame.length; i++) { frame[i] &= part[i]; }
            });
            return {frames: [frame], order: [0], kind: Kind.FULL, intervalMs: 0, repeat: 1, edgeMargin: 0,
                note: 'Từ trên xuống: ngưỡng, Floyd–Steinberg, Atkinson, Bayer 4×4, Bayer 8×8.'};
        },
    },
    {
        id: 'invert_full',
        name: 'Đảo cả màn',
        description: 'Cả màn đổi đen trắng bằng partial cả màn. Nặng nhất cho bảng sóng; chấm độ đen, độ trắng sau N lần.',
        params: [
            {key: 'count', label: 'Số lần đảo', value: 6, min: 1, max: 60},
            {key: 'interval', label: 'ms giữa hai lần', value: 500, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            return {
                frames: [createFrame(true), createFrame(false)],
                order: Array.from({length: values.count}, (_, index) => (index + 1) % 2),
                kind: Kind.PARTIAL,
                intervalMs: values.interval,
                repeat: 1,
                edgeMargin: 0,
            };
        },
    },
    {
        id: 'edge_push',
        name: 'Đẩy lại viền',
        description: 'Giống "Lật ô" nhưng có đẩy lại viền. Chạy hai lần (viền 0 và viền 2) rồi so vệt xám quanh ô.',
        params: [
            {key: 'count', label: 'Số lần lật', value: 20, min: 1, max: 200},
            {key: 'edge', label: 'Độ rộng viền (0–7)', value: 2, min: 0, max: 7},
            {key: 'interval', label: 'ms giữa hai lần', value: 500, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {
                frames: [createFrame(true), frameWithBox(orientation, box)],
                order: Array.from({length: values.count}, (_, index) => (index + 1) % 2),
                kind: Kind.AUTO,
                intervalMs: values.interval,
                repeat: 1,
                edgeMargin: values.edge,
            };
        },
    },
    {
        id: 'button_latency',
        name: 'Độ trễ nút',
        description: 'Nạp các trang số lớn rồi chờ. Bấm A sang trang sau, B về trang trước; cột input là từ lúc bấm tới lúc vẽ. Dùng đường pipeline để giống app thật.',
        params: [{key: 'pages', label: 'Số trang', value: 5, min: 2, max: TEST_SLOT_COUNT}],
        build(values, orientation) {
            const frames = Array.from({length: values.pages}, (_, index) => bigNumberPage(orientation, index + 1));
            return {
                frames,
                order: frames.map((_, index) => index),
                kind: Kind.AUTO,
                intervalMs: 65535, /* chuỗi gần như đứng yên; nút điều khiển */
                repeat: 0,
                edgeMargin: 0,
                note: 'Chuỗi vẽ trang 1 rồi chờ nút.',
            };
        },
    },
];

export function defaultOrientation() { return Orientation.LANDSCAPE_90; }
