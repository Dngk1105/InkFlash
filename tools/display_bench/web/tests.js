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
 * params: {key, label, value, min, max, step?} cho số; {type: 'profile', value: tên profile mặc định}; {type: 'bool'}.
 * build(values, orientation) trả {frames, order (index trong frames), intervalMs, repeat, note?, raw?, oldSource?,
 * intent?, flags?, optionsPatch?, special?}. `path` của bài: 'raw', 'pipeline', hoặc không có (đường nào cũng được).
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
