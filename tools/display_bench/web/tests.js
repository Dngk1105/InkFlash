/* Thiết kế: docs/design/display/bench.md */

/*
 * Bài test sinh khung trên PC. Mỗi bài trả một kế hoạch: các khung (gửi vào slot từ TEST_FIRST_SLOT), thứ tự phát,
 * ms giữa hai khung, cách vẽ raw (profile, window, mặt phẳng cũ, cấu hình vẽ) hoặc ý định và cài đặt của đường pipeline.
 * Đường raw: trang gửi gói DRAW cho khung đầu rồi PLAY phần còn lại. Đường pipeline: trang gửi PLAY.
 * Mỗi bài trả lời một câu trong bảng "Bài đo" của docs/design/display/bench.md.
 */
import {Orientation, createFrame, fillViewRect, frameFromCanvas, grayToFrame, DitherMethod, viewSize} from './frames.js';
import {Intent, SubmitFlag} from './link.js';

/* Slot 0–47 cho ảnh người dùng, 48–63 cho bài test. */
export const TEST_FIRST_SLOT = 48;
export const TEST_SLOT_COUNT = 16;

/* Giá trị của gói DRAW (bench.h). */
export const OldSource = Object.freeze({SHOWN: 0, SLOT: 1, INVERTED: 2, SAME: 3});
export const WindowKind = Object.freeze({GIVEN: 0, WHOLE: 1, CHANGED: 2});

/* VCOM_DC: mã 0x00–0x13 là −0,1 tới −2,0 V (UC8253 tr.37); 255 = theo profile. */
export const VCOM_OPTIONS = Object.freeze([
    {value: 255, label: 'Theo profile (tr8, vẽ tay: 09; tr0–tr7: 13)'},
    ...Array.from({length: 0x14}, (_, code) => ({
        value: code,
        label: `${code.toString(16).toUpperCase().padStart(2, '0')}: −${((code + 1) / 10).toFixed(1).replace('.', ',')} V`
            + (code === 0x09 ? ' (TR8)' : code === 0x13 ? ' (TR0–TR7)' : ''),
    })),
]);

/* PSR byte 2 (commands.md): D3 TS_AUTO, D2 TIEG (VGL nối đất sau POF), D1 NORG, D0 VC_LUTZ. */
export const PSR_SECOND_OPTIONS = Object.freeze([
    {value: 0x0a, label: '0A: TIEG 0, VCOM nối đất ngoài lúc vẽ (mặc định driver)'},
    {value: 0x09, label: '09: TIEG 0, VCOM nổi sau khi vẽ'},
    {value: 0x0e, label: '0E: TIEG 1, VCOM nối đất ngoài lúc vẽ (mã mẫu hãng)'},
    {value: 0x0d, label: '0D: TIEG 1, VCOM nổi sau khi vẽ (mặc định UC8253, GxEPD2)'},
]);

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

/* Ô ngẫu nhiên có seed: cùng seed thì cùng vị trí, để so hai lần chạy. Các ô cách nhau ít nhất `gap` điểm. */
function scatteredBoxes(orientation, count, seed, gap = 6) {
    const SIZES = [[8, 8], [16, 48], [24, 24], [40, 16], [60, 30], [80, 40], [100, 60], [30, 90], [120, 24], [48, 48], [12, 100], [150, 80],
        [20, 12], [64, 64], [36, 20]];
    const {width, height} = viewSize(orientation);
    let state = seed >>> 0;
    const random = () => { /* mulberry32 */
        state = (state + 0x6d2b79f5) >>> 0;
        let t = state;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
    const overlaps = (a, b) => a.x < b.x + b.width + gap && b.x < a.x + a.width + gap && a.y < b.y + b.height + gap && b.y < a.y + a.height + gap;
    const boxes = [];
    /* Ô lớn đặt trước cho dễ còn chỗ */
    const sizes = SIZES.slice(0, count).sort((a, b) => b[0] * b[1] - a[0] * a[1]);
    for (const [boxWidth, boxHeight] of sizes) {
        const w = Math.min(boxWidth, width - 2 * gap);
        const h = Math.min(boxHeight, height - 2 * gap);
        for (let attempt = 0; attempt < 200; attempt++) {
            const box = {x: gap + Math.floor(random() * (width - w - 2 * gap)), y: gap + Math.floor(random() * (height - h - 2 * gap)), width: w, height: h};
            if (!boxes.some((other) => overlaps(box, other))) {
                boxes.push(box);
                break;
            }
        }
    }
    return boxes;
}

function frameWithBoxes(orientation, boxes) {
    const frame = createFrame(true);
    for (const box of boxes) { fillViewRect(frame, orientation, box.x, box.y, box.width, box.height, false); }
    return frame;
}

/* ---- Danh sách bài ---- */

/*
 * params: {key, label, value, min, max, step?} cho số; {type: 'profile', value: tên profile mặc định}; {type: 'bool'};
 * {type: 'select', options: [{value, label}], value}.
 * build(values, orientation) trả {frames, order (index trong frames), intervalMs, repeat, note?, raw?, oldSource?,
 * intent?, flags?, optionsPatch?, psrSecond?, vcomOverride?, special?}. psrSecond, vcomOverride: gói SET_PANEL trang gửi trước
 * khi chạy (255 = VCOM_DC theo profile). `path` của bài: 'raw', 'pipeline', hoặc không có (đường nào cũng được).
 */
const flips = (count) => Array.from({length: count}, (_, index) => (index + 1) % 2);

export const TESTS = [
    {
        id: 'ghost_kind',
        name: 'Loại bóng mờ',
        path: 'raw',
        description: 'Ô 80 × 40 giữa màn lật 10 lần cùng chỗ bằng một profile, window vùng đổi, mặt phẳng cũ là ảnh đang hiện. Chụp ảnh sau lần cuối: bóng là viền quanh ô, xám trong ô, hay cả màn xám dần.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'lut10f50'},
            {key: 'count', label: 'Số lần lật', value: 10, min: 1, max: 200},
            {key: 'interval', label: 'ms giữa hai lần', value: 1000, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {
                frames: [createFrame(true), frameWithBox(orientation, box)],
                order: flips(values.count),
                intervalMs: values.interval,
                repeat: 1,
                raw: {profile: values.profile, windowKind: WindowKind.CHANGED, partialMode: true},
                note: 'Chụp ảnh ô và vùng quanh ô sau lần lật cuối; chấm điểm bóng mờ và viền.',
            };
        },
    },
    {
        id: 'hold_zero',
        name: 'Giữ màu 0 V',
        path: 'raw',
        description: 'Ô cờ đảo pha N lần bằng một profile, cả màn trong chế độ partial, rồi về trắng. Chạy hai lần: tr8 rồi tr8_kk0 (bảng đen giữ đen 0 V). So nền trắng và ô đen đứng yên: có xám dần không.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'cycles', label: 'Số lần đảo', value: 100, min: 1, max: 200},
            {key: 'cell', label: 'Cỡ ô (điểm)', value: 16, min: 2, max: 64},
            {key: 'interval', label: 'ms giữa hai lần', value: 100, min: 0, max: 10000, step: 50},
        ],
        build(values, orientation) {
            const order = Array.from({length: values.cycles}, (_, index) => index % 2);
            order.push(2);
            return {
                frames: [checkerboard(orientation, values.cell, 0), checkerboard(orientation, values.cell, 1), createFrame(true)],
                order,
                intervalMs: values.interval,
                repeat: 1,
                raw: {profile: values.profile, windowKind: WindowKind.WHOLE, partialMode: true},
            };
        },
    },
    {
        id: 'edge_cleanup',
        name: 'Dọn viền theo cờ',
        path: 'pipeline',
        description: 'Ô lật N lần với ý định UI rồi máy nghỉ. Sau edge_ms, lượt dọn tâm viền và dọn quanh viền hiện trong log kèm số điểm có cờ và thời gian panel (phải dưới 100 ms). Chụp trước và sau lượt dọn; đếm viền thấy trên ảnh so với số điểm có cờ.',
        params: [
            {key: 'count', label: 'Số lần lật', value: 20, min: 1, max: 200},
            {key: 'interval', label: 'ms giữa hai lần', value: 300, min: 0, max: 10000, step: 50},
            {key: 'edge', label: 'edge_ms', value: 3000, min: 0, max: 60000, step: 500},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {
                frames: [createFrame(true), frameWithBox(orientation, box)],
                order: flips(values.count),
                intervalMs: values.interval,
                repeat: 1,
                intent: Intent.UI,
                flags: 0,
                optionsPatch: {edgeMs: values.edge},
                note: 'Chụp ảnh ngay sau lần lật cuối (trước lượt dọn) và sau gói "dọn quanh viền".',
            };
        },
    },
    {
        id: 'edge_cleanup_multi',
        name: 'Dọn viền nhiều ô',
        path: 'pipeline',
        description: 'Như "Dọn viền theo cờ" nhưng nhiều ô khác cỡ, khác chỗ (vị trí theo seed). Lật lần lượt: mỗi khung thêm hoặc bớt một ô, nên window và tuổi viền mỗi ô khác nhau. Lật cùng lúc: mọi ô đổi trong một khung. Chụp trước và sau lượt dọn; so viền ô nhỏ với ô lớn, ô gần nhau với ô đứng riêng.',
        params: [
            {key: 'boxes', label: 'Số ô', value: 8, min: 2, max: 15},
            {key: 'seed', label: 'Seed vị trí', value: 1, min: 1, max: 9999},
            {key: 'together', label: 'Lật cùng lúc', type: 'bool', value: false},
            {key: 'cycles', label: 'Số vòng lật', value: 3, min: 1, max: 30},
            {key: 'interval', label: 'ms giữa hai lần', value: 300, min: 0, max: 10000, step: 50},
            {key: 'edge', label: 'edge_ms', value: 3000, min: 0, max: 60000, step: 500},
        ],
        build(values, orientation) {
            const boxes = scatteredBoxes(orientation, values.boxes, values.seed);
            const note = `${boxes.length} ô: ${boxes.map((box) => `${box.width}×${box.height}@${box.x},${box.y}`).join(' ')}. `
                + 'Chụp ảnh ngay sau lần lật cuối (trước lượt dọn) và sau gói "dọn quanh viền".';
            if (values.together) {
                return {frames: [createFrame(true), frameWithBoxes(orientation, boxes)], order: flips(values.cycles * 2), intervalMs: values.interval,
                    repeat: 1, intent: Intent.UI, flags: 0, optionsPatch: {edgeMs: values.edge}, note};
            }
            /* Khung i có i ô đầu đen: 0, 1, ..., n, n-1, ..., 1 lặp lại, mỗi bước đổi đúng một ô. */
            const frames = Array.from({length: boxes.length + 1}, (_, index) => frameWithBoxes(orientation, boxes.slice(0, index)));
            const cycle = [...frames.keys(), ...[...frames.keys()].slice(1, -1).reverse()];
            const order = [...Array.from({length: values.cycles}, () => cycle).flat(), 0];
            return {frames, order, intervalMs: values.interval, repeat: 1, intent: Intent.UI, flags: 0, optionsPatch: {edgeMs: values.edge}, note};
        },
    },
    {
        id: 'psr_power_off',
        name: 'PSR byte 2 và tắt nguồn',
        path: 'pipeline',
        description: 'Trang chữ, một ô lật N lần với ý định UI cho có nợ, rồi máy nghỉ qua dọn viền, dọn nhẹ và tắt nguồn (dọn mạnh dời xa để không xoá dấu vết). Chạy với từng giá trị PSR byte 2, cùng thông số. Xem có hạt nào tự xám sau gói "tắt nguồn" không, và hạt xám là đen nhạt đi hay trắng tối đi, trong vùng ô hay cả ở vùng chữ không bị đụng. Đặt power_off_ms = 0 để xem hạt có xám khi nguồn vẫn bật.',
        params: [
            {key: 'psr', label: 'PSR byte 2', type: 'select', value: 0x0a, options: PSR_SECOND_OPTIONS},
            {key: 'count', label: 'Số lần lật', value: 20, min: 1, max: 60},
            {key: 'interval', label: 'ms giữa hai lần', value: 300, min: 0, max: 10000, step: 50},
            {key: 'edge', label: 'edge_ms', value: 2000, min: 0, max: 60000, step: 500},
            {key: 'light', label: 'light_ms', value: 6000, min: 0, max: 120000, step: 500},
            {key: 'power', label: 'power_off_ms (0 = không tắt)', value: 2000, min: 0, max: 600000, step: 500},
        ],
        build(values, orientation) {
            const page = textPage(orientation, 0, 18);
            const box = centeredBox(orientation, 120, 60);
            const withBox = page.slice();
            fillViewRect(withBox, orientation, box.x, box.y, box.width, box.height, false);
            return {
                frames: [page, withBox],
                order: [...flips(values.count), 0],
                intervalMs: values.interval,
                repeat: 1,
                intent: Intent.UI,
                flags: 0,
                psrSecond: values.psr,
                optionsPatch: {edgeMs: values.edge, lightMs: values.light, strongMs: 3_600_000, powerOffMs: values.power},
                note: 'Chụp ảnh: sau gói "dọn nhẹ", ngay sau gói "tắt nguồn", và 30 s sau đó. Dọn mạnh dời tới 1 giờ; bấm "Về mặc định" ở tab Chế độ khi xong.',
            };
        },
    },
    {
        id: 'vcom_still',
        name: 'VCOM và vùng đứng yên',
        path: 'raw',
        description: 'Nửa trái trắng, nửa phải đen, đứng yên; ô giữa màn lật N lần, window cả màn trong chế độ partial nên mọi điểm đứng yên đều bị quét. Chạy cùng profile với nhiều VCOM_DC (09 = −1,0 V như TR8; mã nhỏ hơn là ít âm hơn). So vùng đứng yên trong lúc chạy, ngay sau gói tắt nguồn và vài phút sau: nửa trắng có tối dần, nửa đen có nhạt dần không. PSR byte 2 có TIEG 0 để xem VGL nối đất lúc POF có phải nguyên nhân.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'vcom', label: 'VCOM_DC', type: 'select', value: 255,
                options: VCOM_OPTIONS},
            {key: 'psr', label: 'PSR byte 2', type: 'select', value: 0x0a, options: PSR_SECOND_OPTIONS},
            {key: 'count', label: 'Số lần lật', value: 60, min: 1, max: 200},
            {key: 'interval', label: 'ms giữa hai lần', value: 200, min: 0, max: 10000, step: 50},
        ],
        build(values, orientation) {
            const {width, height} = viewSize(orientation);
            const still = createFrame(true);
            fillViewRect(still, orientation, Math.floor(width / 2), 0, width - Math.floor(width / 2), height, false);
            const box = centeredBox(orientation, 60, 60);
            const flipped = still.slice();
            for (let row = box.y; row < box.y + box.height; row++) {
                fillViewRect(flipped, orientation, box.x, row, box.width / 2, 1, false);
                fillViewRect(flipped, orientation, box.x + box.width / 2, row, box.width / 2, 1, true);
            }
            return {
                frames: [still, flipped],
                order: [0, ...flips(values.count), 0],
                intervalMs: values.interval,
                repeat: 1,
                vcomOverride: values.vcom,
                psrSecond: values.psr,
                raw: {profile: values.profile, windowKind: WindowKind.WHOLE, partialMode: true},
                note: 'Chụp ảnh vùng đứng yên trước khi chạy, sau lần cuối, và 2–3 phút sau, cùng góc, cùng đèn. Ô giữa: nửa trái đổi trắng → đen, nửa phải đen → trắng.',
            };
        },
    },
    {
        id: 'text_profile',
        name: 'Chữ cho TEXT',
        path: 'raw',
        description: 'Trang chữ cuộn một dòng mỗi lần, như đọc thẻ, bằng profile chọn, window vùng đổi. Chạy với từng profile ứng viên (tr8, lut15f65, lut12f50...) rồi chấm độ nét chữ và bóng của dòng cũ.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'pages', label: 'Số trang', value: 8, min: 2, max: TEST_SLOT_COUNT},
            {key: 'fontSize', label: 'Cỡ chữ (điểm)', value: 18, min: 10, max: 40},
            {key: 'interval', label: 'ms giữa hai lần', value: 1500, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const frames = Array.from({length: values.pages}, (_, index) => textPage(orientation, index, values.fontSize));
            return {
                frames,
                order: frames.map((_, index) => index),
                intervalMs: values.interval,
                repeat: 1,
                raw: {profile: values.profile, windowKind: WindowKind.CHANGED, partialMode: true},
            };
        },
    },
    {
        id: 'ladder',
        name: 'Thang dọn',
        path: 'pipeline',
        description: 'Lật ô nhiều lần với ý định UI (nợ cao), rồi nghỉ qua cả ba bậc với thời gian rút ngắn. Gói DRAWN phải lần lượt là dọn tâm viền, dọn quanh viền, dọn nhẹ, dọn mạnh, tắt nguồn; nợ lớn nhất về 0 sau dọn mạnh.',
        params: [
            {key: 'count', label: 'Số lần lật', value: 6, min: 1, max: 60},
            {key: 'interval', label: 'ms giữa hai lần', value: 200, min: 0, max: 5000, step: 50},
            {key: 'edge', label: 'edge_ms', value: 2000, min: 0, max: 60000, step: 500},
            {key: 'light', label: 'light_ms', value: 6000, min: 0, max: 120000, step: 500},
            {key: 'strong', label: 'strong_ms', value: 12000, min: 0, max: 600000, step: 1000},
            {key: 'power', label: 'power_off_ms', value: 2000, min: 0, max: 60000, step: 500},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {
                frames: [createFrame(true), frameWithBox(orientation, box)],
                order: flips(values.count),
                intervalMs: values.interval,
                repeat: 1,
                intent: Intent.UI,
                flags: 0,
                optionsPatch: {edgeMs: values.edge, lightMs: values.light, strongMs: values.strong, powerOffMs: values.power},
                note: 'Để máy yên tới khi có gói "dọn mạnh" và "tắt nguồn"; xem thứ tự trong log.',
            };
        },
    },
    {
        id: 'policy_time',
        name: 'Thời gian policy',
        path: 'pipeline',
        description: 'Cả màn đổi giữa hai trang chữ với ý định TEXT. Cột diff là thời gian policy so khung, cập nhật trạng thái từng điểm trong PSRAM và dựng mặt phẳng cũ.',
        params: [
            {key: 'count', label: 'Số lần đổi', value: 10, min: 1, max: 60},
            {key: 'interval', label: 'ms giữa hai lần', value: 500, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            return {
                frames: [textPage(orientation, 0, 18), textPage(orientation, 3, 18)],
                order: Array.from({length: values.count}, (_, index) => index % 2),
                intervalMs: values.interval,
                repeat: 1,
                intent: Intent.TEXT,
                flags: SubmitFlag.DEFER_CLEANUP,
                note: 'Cờ hoãn dọn bật để lượt dọn không xen vào số đo.',
            };
        },
    },
    {
        id: 'pt_scan',
        name: 'PT_SCAN',
        path: 'raw',
        description: 'Vẽ ô A bên trái, rồi vẽ thêm ô B bên phải (window chỉ quanh B). Chạy hai lần, PT_SCAN bật và tắt: ô A có bị nhạt hay kéo theo khi vẽ B không.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'ptScan', label: 'PT_SCAN = 1', type: 'bool', value: true},
            {key: 'repeats', label: 'Số lần vẽ lại B', value: 5, min: 1, max: 30},
        ],
        build(values, orientation) {
            const {width, height} = viewSize(orientation);
            const boxA = {x: Math.round(width * 0.15), y: Math.round(height / 2 - 30), width: 60, height: 60};
            const boxB = {x: Math.round(width * 0.65), y: Math.round(height / 2 - 30), width: 60, height: 60};
            const withA = frameWithBox(orientation, boxA);
            const withBoth = frameWithBox(orientation, boxA);
            fillViewRect(withBoth, orientation, boxB.x, boxB.y, boxB.width, boxB.height, false);
            const order = [0, 1];
            for (let i = 0; i < values.repeats; i++) { order.push(2, 1); }
            return {
                frames: [createFrame(true), withA, withBoth],
                order,
                intervalMs: 800,
                repeat: 1,
                raw: {profile: values.profile, windowKind: WindowKind.CHANGED, partialMode: true, ptScan: values.ptScan},
            };
        },
    },
    {
        id: 'border',
        name: 'Viền màn',
        path: 'raw',
        description: 'Cả màn đổi trắng ↔ đen, mặt phẳng cũ đảo bit. Chạy các tổ hợp: trong/ngoài chế độ partial, có/không kéo viền. Xem dải viền ngoài vùng ảnh của panel có đổi màu không.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'partialMode', label: 'Vào chế độ partial', type: 'bool', value: false},
            {key: 'driveBorder', label: 'Kéo viền màn', type: 'bool', value: true},
            {key: 'count', label: 'Số lần đổi', value: 4, min: 1, max: 20},
        ],
        build(values) {
            return {
                frames: [createFrame(true), createFrame(false)],
                order: flips(values.count),
                intervalMs: 1500,
                repeat: 1,
                oldSource: OldSource.INVERTED,
                raw: {profile: values.profile, windowKind: WindowKind.WHOLE, partialMode: values.partialMode, driveBorder: values.driveBorder},
                note: 'Chuỗi sau khung đầu dùng mặt phẳng cũ là ảnh đang hiện; mọi điểm đều đổi nên tương đương đảo bit.',
            };
        },
    },
    {
        id: 'temperature',
        name: 'Nhiệt độ',
        path: 'raw',
        description: 'Vẽ một khung (REG = 1, bảng nạp qua thanh ghi), rồi đọc cảm biến 0x40 nhiều lần. So với nhiệt kế trong phòng; ghi số đo vào ghi chú khi chấm điểm.',
        params: [
            {key: 'readings', label: 'Số lần đọc', value: 10, min: 1, max: 100},
            {key: 'interval', label: 'ms giữa hai lần đọc', value: 1000, min: 100, max: 60000, step: 100},
        ],
        build(values, orientation) {
            return {
                frames: [bigNumberPage(orientation, 25)],
                order: [0],
                intervalMs: values.interval,
                repeat: 1,
                readings: values.readings,
                special: 'temperature',
            };
        },
    },
    {
        id: 'hold_power',
        name: 'Giữ nguồn',
        path: 'raw',
        description: 'Vẽ một khung, giữ nguồn analog bật N giây, rồi tắt và chờ N giây nữa. Đo dòng của board ở hai giai đoạn bằng đồng hồ; ghi hai số vào ghi chú khi chấm điểm.',
        params: [{key: 'hold', label: 'Giây mỗi giai đoạn', value: 20, min: 5, max: 600}],
        build(values, orientation) {
            return {frames: [bigNumberPage(orientation, 1)], order: [0], intervalMs: 0, repeat: 1, holdMs: values.hold * 1000, special: 'hold_power'};
        },
    },
    {
        id: 'flip_box',
        name: 'Lật ô',
        description: 'Ô 80 × 40 giữa màn đổi đen trắng mỗi lần. Đếm bằng mắt: lần nào cũng phải đổi (kiểm hai vùng RAM đổi vai). Raw dùng cách vẽ ở tab Chế độ; pipeline dùng ý định ở tab Chế độ.',
        params: [
            {key: 'count', label: 'Số lần lật', value: 10, min: 1, max: 200},
            {key: 'interval', label: 'ms giữa hai lần', value: 1000, min: 0, max: 10000, step: 100},
        ],
        build(values, orientation) {
            const box = centeredBox(orientation, 80, 40);
            return {frames: [createFrame(true), frameWithBox(orientation, box)], order: flips(values.count), intervalMs: values.interval, repeat: 1};
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
            return {frames, order, intervalMs: values.interval, repeat: 1, raw: {windowKind: WindowKind.CHANGED}};
        },
    },
    {
        id: 'gray_ramp',
        name: 'Dải xám dither',
        path: 'raw',
        description: 'Một khung có dải xám từ trắng tới đen, mỗi dải một cách dither, vẽ cả màn bằng mặt phẳng cũ đảo bit. Chấm cách nào trông mịn nhất.',
        params: [{key: 'profile', label: 'Profile', type: 'profile', value: 'tr4'}],
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
            return {frames: [frame], order: [0], intervalMs: 0, repeat: 1, oldSource: OldSource.INVERTED,
                raw: {profile: values.profile, windowKind: WindowKind.WHOLE, partialMode: false, driveBorder: true},
                note: 'Từ trên xuống: ngưỡng, Floyd–Steinberg, Atkinson, Bayer 4×4, Bayer 8×8.'};
        },
    },
    {
        id: 'invert_full',
        name: 'Đảo cả màn',
        path: 'raw',
        description: 'Cả màn đổi đen trắng bằng profile chọn, cả màn trong chế độ partial. Nặng nhất cho bảng sóng; chấm độ đen, độ trắng sau N lần.',
        params: [
            {key: 'profile', label: 'Profile', type: 'profile', value: 'tr8'},
            {key: 'count', label: 'Số lần đảo', value: 6, min: 1, max: 60},
            {key: 'interval', label: 'ms giữa hai lần', value: 500, min: 0, max: 10000, step: 100},
        ],
        build(values) {
            return {frames: [createFrame(true), createFrame(false)], order: flips(values.count), intervalMs: values.interval, repeat: 1,
                raw: {profile: values.profile, windowKind: WindowKind.WHOLE, partialMode: true}};
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
                intervalMs: 65535, /* chuỗi gần như đứng yên; nút điều khiển */
                repeat: 0,
                note: 'Chuỗi vẽ trang 1 rồi chờ nút.',
            };
        },
    },
];

export function defaultOrientation() { return Orientation.LANDSCAPE_90; }
