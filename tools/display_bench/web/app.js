/* Thiết kế: docs/design/display/bench.md */

/* Điều khiển trang: kết nối, chế độ, ảnh, chuỗi, bài test, chấm điểm, lịch sử. Trạng thái gom trong `app`. */
import {BenchLink, Command, Writer, delay, espErrorName} from './link.js';
import {
    DITHER_NAMES, DitherMethod, FRAME_BYTES, createFrame, grayToFrame, imageToGray, renderFrame, viewRectToPanel, viewSize,
} from './frames.js';
import {KIND_NAMES, Kind, TESTS, TEST_FIRST_SLOT, TEST_SLOT_COUNT, defaultOrientation} from './tests.js';

/* ---- Hằng số ---- */

const SLOT_COUNT = 64;
const USER_SLOT_COUNT = TEST_FIRST_SLOT;
const PATH_CODES = {raw: 0, pipeline: 1};
const DISPLAY_SLOT = {FULL: 0, PARTIAL: 1};
const LOG_LINE_LIMIT = 600;
const GESTURE_LIMIT = 30;
const PREVIEW_SCALE = 1;
const THUMBNAIL_SCALE = 0.25;

const SEGMENTS = [
    {key: 'inputMs', label: 'input'},
    {key: 'queueMs', label: 'queue'},
    {key: 'diffMs', label: 'diff'},
    {key: 'spiMs', label: 'spi'},
    {key: 'panelMs', label: 'panel'},
    {key: 'afterMs', label: 'after'},
    {key: 'totalMs', label: 'tổng'},
];

const SCORES = [
    {key: 'black', label: 'Độ đen', short: 'Đen'},
    {key: 'white', label: 'Độ trắng', short: 'Trắng'},
    {key: 'ghosting', label: 'Bóng mờ (5 = không thấy)', short: 'Bóng mờ'},
    {key: 'edge', label: 'Viền (5 = không thấy)', short: 'Viền'},
    {key: 'flicker', label: 'Chớp (5 = không chớp)', short: 'Chớp'},
    {key: 'overall', label: 'Tổng thể', short: 'Tổng'},
];

/* Tần số khung có trong bảng PLL của UC8253 (pll_hz trong ink_drv_uc8253.c); 0 = tần số của panel. */
function pllFrequencies() {
    const frequencies = [];
    for (let code = 0; code < 32; code++) {
        if (code < 16) { frequencies.push(5 + 5 * code); } else if (code < 24) { frequencies.push(85 + 5 * (code - 16)); } else { frequencies.push(130 + 10 * (code - 24)); }
    }
    return frequencies;
}

/* ---- Trạng thái ---- */

const app = {
    link: new BenchLink(),
    info: null,
    orientation: defaultOrientation(),
    slots: Array.from({length: SLOT_COUNT}, () => ({frame: null, name: '', uploaded: false})),
    shown: {frame: null, window: null},
    selectedSlot: 0,
    image: {source: null, name: '', frame: null},
    selectedWindow: null, /* toạ độ panel; null = cả màn */
    sequence: [],
    selectedTestId: TESTS[0].id,
    settings: {
        fullProfile: 0,
        partialProfile: 0,
        tune: {[DISPLAY_SLOT.FULL]: null, [DISPLAY_SLOT.PARTIAL]: null},
        edgeMargin: 0,
        partialsBeforeFull: 10,
        temperatureCelsius: null,
    },
    run: null, /* {label, testId, params, kind, startedAt, drawn: [], settings} */
    pendingPreview: new Map(), /* slot → frame vừa giao, để cập nhật màn đang hiện khi có DRAWN */
    history: [],
};

/* ---- Tiện ích DOM ---- */

const $ = (id) => document.getElementById(id);

function element(tag, properties = {}, children = []) {
    const node = document.createElement(tag);
    for (const [key, value] of Object.entries(properties)) {
        if (key === 'class') { node.className = value; } else if (key === 'text') { node.textContent = value; } else if (key.startsWith('on')) { node.addEventListener(key.slice(2), value); } else { node.setAttribute(key, value); }
    }
    for (const child of [].concat(children)) { if (child) { node.append(child); } }
    return node;
}

function fillSelect(select, options, selected) {
    select.replaceChildren(...options.map(({value, label}) => element('option', {value: String(value), text: label})));
    if (selected !== undefined) { select.value = String(selected); }
}

function format(value, digits = 0) {
    return value === null || value === undefined || Number.isNaN(value) ? '–' : Number(value).toFixed(digits);
}

/* Bọc thao tác với máy: lỗi thì ghi log, không làm hỏng trang. */
async function guarded(action) {
    try {
        return await action();
    } catch (error) {
        log(`[web] ${error.message}`, 'error');
        toast(error.message, true);
        return undefined;
    }
}

/* ---- Log, thông báo, nút ---- */

const TOAST_MS = 4000;
let toastTimer = null;

function toast(text, isError = false) {
    const box = $('toast');
    box.textContent = text;
    box.className = `toast ${isError ? 'error' : ''}`;
    box.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { box.hidden = true; }, TOAST_MS);
}

function log(text, level = '') {
    const view = $('log');
    const line = element('div', {text, class: level || (text.startsWith('E (') ? 'error' : text.startsWith('W (') ? 'warning' : text.startsWith('[web]') ? 'web' : '')});
    view.append(line);
    while (view.childElementCount > LOG_LINE_LIMIT) { view.firstElementChild.remove(); }
    view.scrollTop = view.scrollHeight;
}

function showGesture(gesture) {
    const list = $('gesture-list');
    const name = gesture.modifier ? `${gesture.modifier}+${gesture.button}` : gesture.button;
    const className = gesture.button === 'A' ? 'button-a' : gesture.button === 'B' ? 'button-b' : 'button-ab';
    list.prepend(element('li', {class: className, text: `${(gesture.timeMs / 1000).toFixed(3)} s  ${name} ${gesture.type}`}));
    while (list.childElementCount > GESTURE_LIMIT) { list.lastElementChild.remove(); }
}

/* ---- Kết nối ---- */

function setConnectionState(state) {
    const badge = $('connection-state');
    const labels = {connected: 'Đã kết nối', disconnected: 'Chưa kết nối', reconnecting: 'Máy khởi động lại…'};
    badge.textContent = labels[state] ?? state;
    badge.className = `badge ${state === 'connected' ? 'connected' : ''}`;
    $('connect-button').textContent = state === 'connected' ? 'Ngắt' : 'Kết nối';
    if (state !== 'connected') { $('device-summary').textContent = ''; }
}

async function onConnected() {
    const info = await app.link.hello();
    app.info = info;
    /* Máy vừa khởi động: slot trong PSRAM đã trắng, profile về mặc định. */
    for (const slot of app.slots) { slot.uploaded = false; }
    app.settings.fullProfile = info.defaultFull;
    app.settings.partialProfile = info.defaultPartial;
    app.settings.tune = {[DISPLAY_SLOT.FULL]: null, [DISPLAY_SLOT.PARTIAL]: null};
    $('device-summary').textContent = `${info.width}×${info.height} · ${info.path} · ${info.firmware}`;
    document.querySelectorAll('input[name="path"]').forEach((radio) => { radio.checked = radio.value === info.path; });
    renderProfiles();
    renderPathControls();
    log(`[web] máy ${info.firmware}, đường ${info.path}, ${info.profileCount} profile`);
}

async function toggleConnection() {
    if (app.link.connected) {
        await app.link.disconnect();
        return;
    }
    await guarded(async () => {
        await app.link.connect();
        try {
            await onConnected();
        } catch (error) {
            await app.link.disconnect();
            throw new Error(`Máy không trả lời: cổng sai, chưa nạp firmware test, hoặc idf.py monitor đang giữ cổng (${error.message})`);
        }
        toast('Đã kết nối');
    });
}

/* Mở lại cổng đã được cấp quyền ở lần trước, không cần chọn lại. */
async function autoConnect() {
    const ports = (await navigator.serial?.getPorts?.()) ?? [];
    if (ports.length !== 1) { return; }
    await guarded(async () => {
        await app.link.openGranted(ports[0]);
        await onConnected();
        toast('Đã tự kết nối lại cổng cũ');
    });
}

/* ---- Chế độ ---- */

function profileName(index) { return app.info?.profiles[index]?.name ?? `#${index}`; }

function renderProfiles() {
    if (!app.info) { return; }
    const options = app.info.profiles.map((profile, index) => ({value: index, label: `${profile.name} (${profile.typicalMs} ms)`}));
    fillSelect($('full-profile'), options, app.settings.fullProfile);
    fillSelect($('partial-profile'), options, app.settings.partialProfile);
}

function renderPathControls() {
    const raw = app.info?.path !== 'pipeline';
    for (const id of ['draw-kind', 'old-source', 'old-slot', 'sequence-kind', 'read-temperature']) { $(id).disabled = !raw; }
    $('apply-options').disabled = raw;
}

async function applyPath() {
    const selected = document.querySelector('input[name="path"]:checked')?.value;
    if (!selected || !app.info || selected === app.info.path) { return; }
    await guarded(async () => {
        await app.link.command(Command.SET_PATH, new Writer().u8(PATH_CODES[selected]).result());
        await app.link.reconnect();
        await onConnected();
    });
}

async function applyProfiles() {
    await guarded(async () => {
        const full = Number($('full-profile').value);
        const partial = Number($('partial-profile').value);
        await app.link.command(Command.SELECT_PROFILE, new Writer().u8(DISPLAY_SLOT.FULL).u8(full).result());
        await app.link.command(Command.SELECT_PROFILE, new Writer().u8(DISPLAY_SLOT.PARTIAL).u8(partial).result());
        app.settings.fullProfile = full;
        app.settings.partialProfile = partial;
        app.settings.tune = {[DISPLAY_SLOT.FULL]: null, [DISPLAY_SLOT.PARTIAL]: null};
        log(`[web] profile full ${profileName(full)}, partial ${profileName(partial)}`);
    });
}

async function applyTune() {
    const slot = Number($('tune-slot').value);
    const tune = {
        frames: Number($('tune-frames').value),
        frameRateHz: Number($('tune-hz').value),
        holdFrames: Number($('tune-hold').value),
        scans: Number($('tune-scans').value),
        keepPowerOn: $('tune-power').checked,
    };
    await guarded(async () => {
        const payload = new Writer().u8(slot).u8(tune.frames).u16(tune.frameRateHz).u8(tune.holdFrames).u8(tune.scans).u8(tune.keepPowerOn ? 1 : 0);
        await app.link.command(Command.TUNE, payload.result());
        app.settings.tune[slot] = tune;
        log(`[web] tune ${slot === DISPLAY_SLOT.FULL ? 'full' : 'partial'}: ${JSON.stringify(tune)}`);
    });
}

async function applyOptions() {
    const edgeMargin = Number($('edge-margin').value);
    const partialsBeforeFull = Number($('partials-before-full').value);
    await guarded(async () => {
        const payload = new Writer().u8(app.settings.fullProfile).u8(app.settings.partialProfile).u16(partialsBeforeFull).u8(edgeMargin);
        await app.link.command(Command.SET_OPTIONS, payload.result());
        app.settings.edgeMargin = edgeMargin;
        app.settings.partialsBeforeFull = partialsBeforeFull;
        log(`[web] cài đặt: viền ${edgeMargin}, ngưỡng partial ${partialsBeforeFull}`);
    });
}

async function readTemperature() {
    await guarded(async () => {
        const reply = await app.link.send(Command.READ_TEMP);
        if (reply.error !== 0) { throw new Error(`Đọc nhiệt độ lỗi ${espErrorName(reply.error)}`); }
        app.settings.temperatureCelsius = reply.celsius;
        $('temperature').textContent = `${reply.celsius} °C`;
    });
}

/* ---- Slot ---- */

async function storeSlot(slot, frame, name) {
    app.slots[slot] = {frame, name, uploaded: false};
    if (app.link.connected) { await ensureUploaded(slot); }
    renderSlots();
}

async function ensureUploaded(slot) {
    const entry = app.slots[slot];
    if (!entry.frame) { entry.frame = createFrame(true); }
    if (entry.uploaded) { return; }
    await app.link.upload(slot, entry.frame);
    entry.uploaded = true;
}

function slotThumbnail(slot, onClick, selected) {
    const entry = app.slots[slot];
    const canvas = element('canvas');
    renderFrame(canvas, entry.frame, app.orientation, THUMBNAIL_SCALE);
    canvas.style.width = '100%';
    canvas.style.height = 'auto';
    return element('div', {class: `slot ${selected ? 'selected' : ''} ${entry.frame ? '' : 'empty'}`, title: entry.name, onclick: onClick}, [
        canvas,
        element('span', {class: 'slot-name', text: `${slot}${entry.name ? ' ' + entry.name : ''}`}),
    ]);
}

function usedSlots() {
    return app.slots.map((entry, slot) => ({entry, slot})).filter(({entry}) => entry.frame);
}

function renderSlots() {
    const slotOptions = app.slots.map((entry, slot) => ({value: slot, label: `${slot}${entry.name ? ' – ' + entry.name : entry.frame ? '' : ' (trống)'}`}));
    fillSelect($('target-slot'), slotOptions.slice(0, USER_SLOT_COUNT), Math.min(app.selectedSlot, USER_SLOT_COUNT - 1));
    fillSelect($('draw-slot'), slotOptions, app.selectedSlot);
    fillSelect($('old-slot'), slotOptions, $('old-slot').value || 0);

    $('slot-grid').replaceChildren(...usedSlots().map(({slot}) => slotThumbnail(slot, () => {
        app.selectedSlot = slot;
        renderSlots();
        renderImagePreview();
    }, slot === app.selectedSlot)));
    $('sequence-slots').replaceChildren(...usedSlots().map(({slot}) => slotThumbnail(slot, () => {
        app.sequence.push(slot);
        renderSequence();
    }, false)));
}

/* ---- Ảnh ---- */

function imageOptions() {
    return {
        fit: $('fit').value,
        brightness: Number($('brightness').value),
        contrast: Number($('contrast').value),
        gamma: Number($('gamma').value),
        invert: $('invert').checked,
    };
}

function processImage() {
    if (!app.image.source) {
        renderImagePreview();
        return;
    }
    const gray = imageToGray(app.image.source, app.orientation, imageOptions());
    app.image.frame = grayToFrame(gray, app.orientation, $('dither').value, Number($('threshold').value));
    renderImagePreview();
}

/* Chưa nạp ảnh thì xem slot đang chọn, để chọn window trên ảnh đã có. */
function renderImagePreview() {
    const frame = app.image.frame ?? app.slots[app.selectedSlot]?.frame ?? null;
    renderFrame($('image-preview'), frame, app.orientation, PREVIEW_SCALE, app.selectedWindow);
    const selected = app.selectedWindow;
    $('selected-window').textContent = selected ? `window x ${selected.x} y ${selected.y} ${selected.width}×${selected.height}` : 'window: cả màn';
}

function loadImage(url, name) {
    const image = new Image();
    image.onload = () => {
        app.image = {source: image, name, frame: null};
        processImage();
    };
    image.onerror = () => log(`[web] không mở được ảnh ${name}`, 'error');
    image.src = url;
}

function loadImageFile(file) {
    if (!file || !file.type.startsWith('image/')) { return; }
    loadImage(URL.createObjectURL(file), file.name);
}

async function loadSampleImages() {
    const names = await fetch('/api/images').then((response) => response.json()).catch(() => []);
    fillSelect($('sample-images'), [{value: '', label: names.length ? '—' : '(thư mục images/ trống)'}, ...names.map((name) => ({value: name, label: name}))]);
}

/* Kéo chuột trên ảnh xem trước để chọn window (toạ độ hướng nhìn → panel). */
function setupWindowSelection() {
    const canvas = $('image-preview');
    let start = null;
    const point = (event) => {
        const rect = canvas.getBoundingClientRect();
        const {width, height} = viewSize(app.orientation);
        return {
            x: Math.max(0, Math.min(width - 1, Math.floor(((event.clientX - rect.left) / rect.width) * width))),
            y: Math.max(0, Math.min(height - 1, Math.floor(((event.clientY - rect.top) / rect.height) * height))),
        };
    };
    canvas.addEventListener('pointerdown', (event) => {
        start = point(event);
        canvas.setPointerCapture(event.pointerId);
    });
    canvas.addEventListener('pointermove', (event) => {
        if (!start) { return; }
        const end = point(event);
        const rect = {x: Math.min(start.x, end.x), y: Math.min(start.y, end.y), width: Math.abs(end.x - start.x) + 1, height: Math.abs(end.y - start.y) + 1};
        app.selectedWindow = rect.width > 2 && rect.height > 2 ? viewRectToPanel(app.orientation, rect) : null;
        renderImagePreview();
    });
    canvas.addEventListener('pointerup', () => { start = null; });
}

async function storeImage() {
    const slot = Number($('target-slot').value);
    const frame = app.image.frame ?? createFrame(true);
    await guarded(async () => {
        await storeSlot(slot, frame, app.image.name || `ảnh ${slot}`);
        app.selectedSlot = slot;
        renderSlots();
        log(`[web] lưu vào slot ${slot}`);
    });
}

/* ---- Vẽ ---- */

function snapshotSettings() {
    const settings = app.settings;
    return {
        path: app.info?.path ?? null,
        firmware: app.info?.firmware ?? null,
        orientation: app.orientation,
        fullProfile: settings.tune[DISPLAY_SLOT.FULL] ? 'tune' : profileName(settings.fullProfile),
        partialProfile: settings.tune[DISPLAY_SLOT.PARTIAL] ? 'tune' : profileName(settings.partialProfile),
        tuneFull: settings.tune[DISPLAY_SLOT.FULL],
        tunePartial: settings.tune[DISPLAY_SLOT.PARTIAL],
        edgeMargin: settings.edgeMargin,
        partialsBeforeFull: settings.partialsBeforeFull,
    };
}

/* expected: số khung sẽ vẽ, để hiện tiến độ N/M; 0 = không biết trước. */
function startRun(label, details = {}, expected = 0) {
    app.run = {label, startedAt: new Date().toISOString(), drawn: [], settings: snapshotSettings(), expected, ...details};
    renderRun();
    renderProgress();
}

function renderProgress() {
    const run = app.run;
    const text = !run ? '' : run.expected ? `${run.label}: ${run.drawn.length}/${run.expected}` : `${run.label}: ${run.drawn.length} lần vẽ`;
    $('run-progress').textContent = text;
    if (run?.expected && run.drawn.length === run.expected) { toast(`${run.label} xong. Mở tab Chấm điểm để chấm.`); }
}

/* Gửi ảnh đang xem vào slot rồi vẽ ngay, theo kiểu vẽ và window đang chọn. */
async function sendAndDraw() {
    await storeImage();
    $('draw-slot').value = $('target-slot').value;
    await drawSlot();
}

/* Vẽ full cả màn trắng, dùng slot cuối của vùng ảnh người dùng. */
const CLEAR_SLOT = USER_SLOT_COUNT - 1;

async function clearScreen() {
    await guarded(async () => {
        await storeSlot(CLEAR_SLOT, createFrame(true), 'trắng');
        app.pendingPreview.set(CLEAR_SLOT, app.slots[CLEAR_SLOT].frame);
        if (app.info?.path === 'pipeline') {
            await app.link.command(Command.SUBMIT, new Writer().u8(CLEAR_SLOT).result());
            return;
        }
        const payload = new Writer().u8(CLEAR_SLOT).u8(Kind.FULL).u8(2).u8(0).u8(0).u8(0).u16(0).u16(0).u16(0).u16(0);
        await app.link.command(Command.DRAW, payload.result());
    });
}

async function rerun() {
    if (!app.lastRun) {
        toast('Chưa có bài hay chuỗi nào để chạy lại.', true);
        return;
    }
    await app.lastRun();
}

async function drawSlot() {
    const slot = Number($('draw-slot').value);
    const raw = app.info?.path !== 'pipeline';
    await guarded(async () => {
        await ensureUploaded(slot);
        if (!app.run || app.run.testId !== 'manual') { startRun('Vẽ tay', {testId: 'manual'}); }
        app.pendingPreview.set(slot, app.slots[slot].frame);
        if (!raw) {
            await app.link.command(Command.SUBMIT, new Writer().u8(slot).result());
            return;
        }
        const panelWindow = app.selectedWindow ?? {x: 0, y: 0, width: 0, height: 0};
        const kind = Number($('draw-kind').value);
        const oldSource = Number($('old-source').value);
        const oldSlot = Number($('old-slot').value);
        if (oldSource === 1) { await ensureUploaded(oldSlot); }
        app.settings.edgeMargin = Number($('edge-margin').value);
        const payload = new Writer().u8(slot).u8(kind).u8(oldSource).u8(oldSlot).u8(app.settings.edgeMargin).u8(0)
            .u16(panelWindow.x).u16(panelWindow.y).u16(panelWindow.width).u16(panelWindow.height);
        app.run.kind = KIND_NAMES[kind];
        await app.link.command(Command.DRAW, payload.result());
    });
}

/* ---- Màn đang hiện, gói DRAWN ---- */

function onDrawn(drawn) {
    const frame = app.pendingPreview.get(drawn.slot) ?? app.slots[drawn.slot]?.frame;
    if (drawn.mode !== 'không vẽ' && drawn.error === 0 && frame) { app.shown = {frame, window: drawn.window}; }
    if (drawn.temperatureValid) { app.settings.temperatureCelsius = drawn.temperatureCelsius; }
    renderFrame($('shown-preview'), app.shown.frame, app.orientation, PREVIEW_SCALE, app.shown.window);
    renderLastDrawn(drawn);
    if (app.run) {
        app.run.drawn.push(drawn);
        renderRun();
        renderProgress();
    }
    highlightSequence(drawn.slot);
}

function renderLastDrawn(drawn) {
    const items = [
        ['slot', drawn.slot],
        ['cách vẽ', drawn.mode],
        ['profile', profileName(drawn.profile)],
        ['window', `${drawn.window.x},${drawn.window.y} ${drawn.window.width}×${drawn.window.height}`],
        ['nhiệt độ', drawn.temperatureValid ? `${drawn.temperatureCelsius} °C` : '–'],
        ['lỗi', drawn.error === 0 ? 'không' : espErrorName(drawn.error)],
        ...SEGMENTS.map(({key, label}) => [label, `${format(drawn[key], 1)} ms`]),
    ];
    $('last-drawn').replaceChildren(...items.flatMap(([label, value]) => [
        element('dt', {text: label}),
        element('dd', {text: String(value), class: label === 'lỗi' && drawn.error !== 0 ? 'error' : ''}),
    ]));
}

/* ---- Chuỗi ---- */

function renderSequence() {
    $('sequence-list').replaceChildren(...app.sequence.map((slot, index) => element('li', {'data-slot': String(slot)}, [
        element('span', {text: String(slot)}),
        element('button', {text: '×', title: 'Bỏ khỏi chuỗi', onclick: () => { app.sequence.splice(index, 1); renderSequence(); }}),
    ])));
}

function highlightSequence(slot) {
    document.querySelectorAll('#sequence-list li').forEach((item) => item.classList.toggle('playing', Number(item.dataset.slot) === slot));
}

/* Gửi khung còn thiếu rồi gói PLAY. */
async function play(slots, intervalMs, repeat, kind, edgeMargin) {
    for (const slot of new Set(slots)) {
        await ensureUploaded(slot);
        app.pendingPreview.set(slot, app.slots[slot].frame);
    }
    const payload = new Writer().u16(Math.min(65535, intervalMs)).u8(repeat).u8(kind).u8(edgeMargin).u8(slots.length).array(slots);
    await app.link.command(Command.PLAY, payload.result());
}

async function playSequence() {
    if (app.sequence.length === 0) {
        log('[web] chuỗi trống: nhấp các slot để thêm', 'warning');
        return;
    }
    const intervalMs = Number($('sequence-interval').value);
    const repeat = Number($('sequence-repeat').value);
    const kind = Number($('sequence-kind').value);
    await guarded(async () => {
        app.lastRun = playSequence;
        startRun('Chuỗi tay', {testId: 'sequence', params: {slots: [...app.sequence], intervalMs, repeat}, kind: KIND_NAMES[kind]},
                 repeat > 0 ? app.sequence.length * repeat : 0);
        await play(app.sequence.slice(0, 64), intervalMs, repeat, kind, app.settings.edgeMargin);
    });
}

async function stopSequence() {
    await guarded(() => app.link.command(Command.STOP));
}

/* ---- Bài test ---- */

function selectedTest() { return TESTS.find((test) => test.id === app.selectedTestId); }

function renderTests() {
    $('test-list').replaceChildren(...TESTS.map((test) => element('li', {
        class: test.id === app.selectedTestId ? 'selected' : '',
        text: test.name,
        onclick: () => { app.selectedTestId = test.id; renderTests(); },
    })));
    const test = selectedTest();
    const inputs = test.params.map((param) => element('input', {
        type: 'number', id: `param-${param.key}`, min: String(param.min), max: String(param.max), step: String(param.step ?? 1), value: String(param.value),
    }));
    $('test-detail').replaceChildren(
        element('h2', {text: test.name}),
        element('p', {text: test.description}),
        element('div', {class: 'form-grid'}, test.params.flatMap((param, index) => [element('label', {for: `param-${param.key}`, text: param.label}), inputs[index]])),
        element('div', {class: 'row'}, [element('button', {class: 'primary', text: 'Chạy bài này', onclick: runSelectedTest})]),
        element('p', {class: 'muted', text: `Khung gửi vào slot ${TEST_FIRST_SLOT}–${TEST_FIRST_SLOT + TEST_SLOT_COUNT - 1}. Hướng nhìn theo tab Ảnh. Đường vẽ hiện tại: ${app.info?.path ?? 'chưa kết nối'}.`}),
    );
}

async function runSelectedTest() {
    const test = selectedTest();
    const values = Object.fromEntries(test.params.map((param) => {
        const value = Number($(`param-${param.key}`).value);
        return [param.key, Math.max(param.min, Math.min(param.max, Number.isFinite(value) ? value : param.value))];
    }));
    const plan = test.build(values, app.orientation);
    if (plan.frames.length > TEST_SLOT_COUNT) {
        log(`[web] bài ${test.name} cần ${plan.frames.length} khung, tối đa ${TEST_SLOT_COUNT}`, 'error');
        return;
    }
    await guarded(async () => {
        plan.frames.forEach((frame, index) => {
            app.slots[TEST_FIRST_SLOT + index] = {frame, name: `${test.id}#${index}`, uploaded: false};
        });
        renderSlots();
        const slots = plan.order.map((index) => TEST_FIRST_SLOT + index);
        /* Bài có tham số viền dùng giá trị của bài; bài khác dùng cài đặt ở tab Chế độ. */
        const edgeMargin = test.params.some((param) => param.key === 'edge') ? plan.edgeMargin : app.settings.edgeMargin;
        app.lastRun = runSelectedTest;
        startRun(test.name, {testId: test.id, params: values, kind: KIND_NAMES[plan.kind], edgeMargin}, plan.repeat === 1 ? slots.length : 0);
        app.run.settings.edgeMargin = edgeMargin;
        if (plan.note) { log(`[web] ${plan.note}`); }
        for (let start = 0; start < slots.length; start += 64) {
            if (start > 0) { log('[web] chuỗi dài hơn 64 khung: chạy phần sau khi phần trước xong', 'warning'); await waitForDraws(app.run, start); }
            await play(slots.slice(start, start + 64), plan.intervalMs, plan.repeat, plan.kind, edgeMargin);
        }
    });
}

/* Chờ tới khi lần chạy có đủ `count` gói DRAWN (chuỗi dài hơn 64 khung chia nhiều gói PLAY). */
async function waitForDraws(run, count) {
    while (app.run === run && run.drawn.length < count) { await delay(200); }
}

/* ---- Thống kê, chấm điểm ---- */

function statistics(drawn) {
    const drawnFrames = drawn.filter((item) => item.mode !== 'không vẽ' && item.error === 0);
    const result = {count: drawnFrames.length, full: drawnFrames.filter((item) => item.mode === 'full').length};
    for (const {key} of SEGMENTS) {
        const values = drawnFrames.map((item) => item[key]);
        result[key] = values.length
            ? {average: values.reduce((sum, value) => sum + value, 0) / values.length, minimum: Math.min(...values), maximum: Math.max(...values)}
            : null;
    }
    const partial = drawnFrames.filter((item) => item.mode === 'partial').map((item) => item.totalMs);
    result.partialAverageMs = partial.length ? partial.reduce((sum, value) => sum + value, 0) / partial.length : null;
    return result;
}

function renderRun() {
    const run = app.run;
    if (!run) { return; }
    const stats = statistics(run.drawn);
    const settings = run.settings;
    $('run-summary').textContent = `${run.label} · ${settings.path ?? '?'} · full ${settings.fullProfile}, partial ${settings.partialProfile}`
        + ` · viền ${settings.edgeMargin} · ${stats.count} lần vẽ (${stats.full} full)`
        + (app.settings.temperatureCelsius !== null ? ` · ${app.settings.temperatureCelsius} °C` : '');
    $('run-stats').replaceChildren(
        element('tr', {}, [element('th', {text: 'đoạn (ms)'}), element('th', {text: 'trung bình'}), element('th', {text: 'nhỏ nhất'}), element('th', {text: 'lớn nhất'})]),
        ...SEGMENTS.map(({key, label}) => element('tr', {}, [
            element('td', {text: label}),
            element('td', {text: format(stats[key]?.average, 1)}),
            element('td', {text: format(stats[key]?.minimum, 1)}),
            element('td', {text: format(stats[key]?.maximum, 1)}),
        ])),
    );
    renderChart(run.drawn);
}

/* Cột chồng spi + panel + after cho từng lần vẽ của lần chạy. */
function renderChart(drawn) {
    const canvas = $('run-chart');
    const ratio = globalThis.devicePixelRatio ?? 1;
    const width = canvas.clientWidth || 600;
    const height = canvas.clientHeight || 180;
    canvas.width = width * ratio;
    canvas.height = height * ratio;
    const context = canvas.getContext('2d');
    context.scale(ratio, ratio);
    context.clearRect(0, 0, width, height);
    const style = getComputedStyle(document.documentElement);
    const colors = [style.getPropertyValue('--brand-teal'), style.getPropertyValue('--accent'), style.getPropertyValue('--brand-bronze')];
    const parts = ['spiMs', 'panelMs', 'afterMs'];
    const items = drawn.filter((item) => item.mode !== 'không vẽ');
    const maximum = Math.max(1, ...items.map((item) => parts.reduce((sum, key) => sum + item[key], 0)));
    const left = 44;
    const bottom = height - 20;
    const barWidth = Math.max(2, Math.min(24, (width - left - 8) / Math.max(1, items.length) - 2));

    context.fillStyle = style.getPropertyValue('--text-muted');
    context.font = '11px Consolas, monospace';
    context.fillText(`${Math.round(maximum)} ms`, 2, 12);
    context.fillText('0', 2, bottom);
    items.forEach((item, index) => {
        let y = bottom;
        parts.forEach((key, part) => {
            const barHeight = (item[key] / maximum) * (bottom - 16);
            context.fillStyle = colors[part];
            context.fillRect(left + index * (barWidth + 2), y - barHeight, barWidth, barHeight);
            y -= barHeight;
        });
    });
    context.fillStyle = style.getPropertyValue('--text-muted');
    ['spi', 'panel', 'after'].forEach((label, index) => {
        context.fillStyle = colors[index];
        context.fillRect(left + index * 70, height - 12, 10, 10);
        context.fillStyle = style.getPropertyValue('--text');
        context.fillText(label, left + index * 70 + 14, height - 3);
    });
}

function renderScoreForm() {
    $('score-form').replaceChildren(...SCORES.flatMap(({key, label}) => {
        const options = element('div', {class: 'score-options'});
        for (const value of ['', 1, 2, 3, 4, 5]) {
            const input = element('input', {type: 'radio', name: `score-${key}`, value: String(value)});
            if (value === '') { input.checked = true; }
            const option = element('label', {class: value === '' ? 'checked' : ''}, [input, value === '' ? 'bỏ qua' : String(value)]);
            input.addEventListener('change', () => {
                options.querySelectorAll('label').forEach((item) => item.classList.toggle('checked', item.contains(input)));
            });
            options.append(option);
        }
        return [element('span', {text: label}), options];
    }));
}

function readScores() {
    const scores = {};
    for (const {key} of SCORES) {
        const value = document.querySelector(`input[name="score-${key}"]:checked`)?.value;
        if (value) { scores[key] = Number(value); }
    }
    return scores;
}

async function saveRating() {
    const run = app.run;
    if (!run) {
        $('rating-status').textContent = 'Chưa có lần chạy để chấm.';
        return;
    }
    const scores = readScores();
    if (Object.keys(scores).length === 0) {
        $('rating-status').textContent = 'Chọn ít nhất một điểm.';
        return;
    }
    const stats = statistics(run.drawn);
    const result = {
        test: run.testId,
        label: run.label,
        params: run.params ?? null,
        kind: run.kind ?? null,
        startedAt: run.startedAt,
        ...run.settings,
        temperatureCelsius: app.settings.temperatureCelsius,
        drawCount: stats.count,
        fullCount: stats.full,
        timing: Object.fromEntries(SEGMENTS.map(({key}) => [key, stats[key]])),
        partialAverageMs: stats.partialAverageMs,
        scores,
        note: $('rating-note').value.trim(),
    };
    const response = await fetch('/api/results', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(result)}).catch(() => null);
    if (!response?.ok) {
        $('rating-status').textContent = 'Không lưu được: bench_server.py có đang chạy không?';
        return;
    }
    const {file} = await response.json();
    $('rating-status').textContent = `Đã lưu vào results/${file}.`;
    $('rating-note').value = '';
    renderScoreForm();
    await loadHistory();
}

/* ---- Lịch sử ---- */

async function loadHistory() {
    app.history = await fetch('/api/results').then((response) => response.json()).catch(() => []);
    const tests = [...new Set(app.history.map((item) => item.label ?? item.test))];
    const profiles = [...new Set(app.history.flatMap((item) => [item.fullProfile, item.partialProfile]).filter(Boolean))];
    const keepTest = $('history-test').value;
    const keepProfile = $('history-profile').value;
    fillSelect($('history-test'), [{value: '', label: 'Tất cả'}, ...tests.map((name) => ({value: name, label: name}))], keepTest);
    fillSelect($('history-profile'), [{value: '', label: 'Tất cả'}, ...profiles.map((name) => ({value: name, label: name}))], keepProfile);
    renderHistory();
}

function renderHistory() {
    const testFilter = $('history-test').value;
    const profileFilter = $('history-profile').value;
    const rows = app.history
        .filter((item) => !testFilter || (item.label ?? item.test) === testFilter)
        .filter((item) => !profileFilter || item.fullProfile === profileFilter || item.partialProfile === profileFilter)
        .slice()
        .reverse();
    const header = ['Lúc', 'Bài', 'Đường', 'Full', 'Partial', 'Viền', '°C', 'Lần', 'Partial TB', 'Panel TB', ...SCORES.map((score) => score.short), 'Ghi chú'];
    $('history-table').replaceChildren(
        element('tr', {}, header.map((title) => element('th', {text: title}))),
        ...rows.map((item) => element('tr', {}, [
            element('td', {text: (item.saved_at ?? '').replace('T', ' ')}),
            element('td', {class: 'text', text: item.label ?? item.test}),
            element('td', {text: item.path ?? '–'}),
            element('td', {text: item.fullProfile ?? '–'}),
            element('td', {text: item.partialProfile ?? '–'}),
            element('td', {text: String(item.edgeMargin ?? '–')}),
            element('td', {text: item.temperatureCelsius ?? '–'}),
            element('td', {text: String(item.drawCount ?? '–')}),
            element('td', {text: format(item.partialAverageMs, 0)}),
            element('td', {text: format(item.timing?.panelMs?.average, 0)}),
            ...SCORES.map(({key}) => element('td', {text: item.scores?.[key] ?? '–'})),
            element('td', {class: 'text', text: item.note ?? ''}),
        ])),
    );
}

/* ---- Theme sáng/tối ---- */

const THEME_KEY = 'inkflash-bench-theme';

function applyTheme(theme) {
    document.documentElement.dataset.theme = theme;
    $('theme-button').textContent = theme === 'dark' ? 'Sáng' : 'Tối';
    try { localStorage.setItem(THEME_KEY, theme); } catch { /* trình duyệt chặn lưu: chỉ mất lựa chọn lần sau */ }
    /* Canvas không tự đổi màu theo CSS: vẽ lại. */
    renderImagePreview();
    renderSlots();
    renderFrame($('shown-preview'), app.shown.frame, app.orientation, PREVIEW_SCALE, app.shown.window);
    if (app.run) { renderRun(); }
}

function setupTheme() {
    let saved = null;
    try { saved = localStorage.getItem(THEME_KEY); } catch { /* không đọc được thì theo hệ điều hành */ }
    const prefersDark = globalThis.matchMedia?.('(prefers-color-scheme: dark)').matches;
    applyTheme(saved ?? (prefersDark ? 'dark' : 'light'));
    $('theme-button').addEventListener('click', () => applyTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark'));
}

/* ---- Tab ---- */

function setupTabs() {
    $('tabs').addEventListener('click', (event) => {
        const tab = event.target.dataset?.tab;
        if (!tab) { return; }
        document.querySelectorAll('#tabs button').forEach((button) => button.classList.toggle('active', button.dataset.tab === tab));
        document.querySelectorAll('.tab-panel').forEach((panel) => panel.classList.toggle('active', panel.id === `tab-${tab}`));
        if (tab === 'tests') { renderTests(); }
        if (tab === 'rating') { renderRun(); }
        if (tab === 'history') { loadHistory(); }
    });
}

/* ---- Khởi động ---- */

function setupControls() {
    $('connect-button').addEventListener('click', toggleConnection);
    $('apply-path').addEventListener('click', applyPath);
    $('full-profile').addEventListener('change', applyProfiles);
    $('partial-profile').addEventListener('change', applyProfiles);
    $('send-and-draw').addEventListener('click', sendAndDraw);
    $('quick-stop').addEventListener('click', stopSequence);
    $('quick-rerun').addEventListener('click', rerun);
    $('quick-clear').addEventListener('click', clearScreen);
    document.addEventListener('keydown', (event) => {
        if (event.target.closest('input, textarea, select')) { return; }
        if (event.key === 'Escape') { stopSequence(); }
        if (event.key === 'r' || event.key === 'R') { rerun(); }
    });
    $('apply-tune').addEventListener('click', applyTune);
    $('apply-options').addEventListener('click', applyOptions);
    $('read-temperature').addEventListener('click', readTemperature);
    $('edge-margin').addEventListener('change', () => { app.settings.edgeMargin = Number($('edge-margin').value); });

    fillSelect($('tune-hz'), [{value: 0, label: 'của panel'}, ...pllFrequencies().map((hz) => ({value: hz, label: `${hz} Hz`}))], 50);
    fillSelect($('dither'), Object.values(DitherMethod).map((method) => ({value: method, label: DITHER_NAMES[method]})), DitherMethod.FLOYD_STEINBERG);
    const kinds = Object.values(Kind).map((kind) => ({value: kind, label: KIND_NAMES[kind]}));
    fillSelect($('draw-kind'), kinds, Kind.AUTO);
    fillSelect($('sequence-kind'), kinds, Kind.AUTO);
    $('orientation').value = app.orientation;

    for (const id of ['fit', 'dither', 'threshold', 'brightness', 'contrast', 'gamma', 'invert']) { $(id).addEventListener('input', processImage); }
    $('orientation').addEventListener('change', () => {
        app.orientation = $('orientation').value;
        app.selectedWindow = null;
        processImage();
        renderSlots();
        renderFrame($('shown-preview'), app.shown.frame, app.orientation, PREVIEW_SCALE, app.shown.window);
    });
    $('image-file').addEventListener('change', (event) => loadImageFile(event.target.files[0]));
    $('sample-images').addEventListener('change', (event) => { if (event.target.value) { loadImage(`/images/${encodeURIComponent(event.target.value)}`, event.target.value); } });
    const dropZone = $('drop-zone');
    dropZone.addEventListener('dragover', (event) => { event.preventDefault(); dropZone.classList.add('over'); });
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('over'));
    dropZone.addEventListener('drop', (event) => {
        event.preventDefault();
        dropZone.classList.remove('over');
        loadImageFile(event.dataTransfer.files[0]);
    });
    setupWindowSelection();
    $('store-image').addEventListener('click', storeImage);
    $('draw-button').addEventListener('click', drawSlot);
    $('draw-slot').addEventListener('change', () => { app.selectedSlot = Number($('draw-slot').value); renderSlots(); });

    $('clear-sequence').addEventListener('click', () => { app.sequence = []; renderSequence(); });
    $('sequence-interval-range').addEventListener('input', () => { $('sequence-interval').value = $('sequence-interval-range').value; });
    $('sequence-interval').addEventListener('input', () => { $('sequence-interval-range').value = $('sequence-interval').value; });
    $('play-sequence').addEventListener('click', playSequence);
    $('stop-sequence').addEventListener('click', stopSequence);

    $('save-rating').addEventListener('click', saveRating);
    $('reload-history').addEventListener('click', loadHistory);
    $('history-test').addEventListener('change', renderHistory);
    $('history-profile').addEventListener('change', renderHistory);
    $('clear-log').addEventListener('click', () => $('log').replaceChildren());
}

function setupLink() {
    app.link.addEventListener('state', (event) => setConnectionState(event.detail));
    app.link.addEventListener('log', (event) => log(event.detail));
    app.link.addEventListener('drawn', (event) => onDrawn(event.detail));
    app.link.addEventListener('gesture', (event) => showGesture(event.detail));
}

function main() {
    setupTabs();
    setupControls();
    setupLink();
    renderScoreForm();
    renderSlots();
    renderSequence();
    renderTests();
    renderPathControls();
    processImage();
    setupTheme();
    loadSampleImages();
    autoConnect();
    if (!('serial' in navigator)) { log('[web] trình duyệt không có Web Serial: dùng Chrome hoặc Edge, mở qua http://localhost', 'error'); }
    if (FRAME_BYTES !== 12480) { log('[web] cỡ khung lệch firmware', 'error'); }
}

main();
