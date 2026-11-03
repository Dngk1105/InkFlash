/* Thiết kế: docs/design/display/bench.md */

/* Điều khiển trang: kết nối, chế độ, ảnh, chuỗi, bài test, chấm điểm, lịch sử. Trạng thái gom trong `app`. */
import {
    ACTION_NAMES, Action, BenchLink, Command, INTENT_NAMES, PURPOSES, SOURCE_HAND, SubmitFlag, Writer, delay, espErrorName, writeOptions,
} from './link.js';
import {
    DITHER_NAMES, DitherMethod, FRAME_BYTES, createFrame, grayToFrame, imageToGray, renderFrame, viewRectToPanel, viewSize,
} from './frames.js';
import {OldSource, TESTS, TEST_FIRST_SLOT, TEST_SLOT_COUNT, WindowKind, defaultOrientation} from './tests.js';

/* ---- Hằng số ---- */

const SLOT_COUNT = 64;
const USER_SLOT_COUNT = TEST_FIRST_SLOT;
const PATH_CODES = {raw: 0, pipeline: 1};
const CLEAN_STRONG = 3; /* index của mục đích dọn mạnh trong PURPOSES */
const DEFAULT_CELSIUS = 25;
const LOG_LINE_LIMIT = 600;
const GESTURE_LIMIT = 30;
const PREVIEW_SCALE = 1;
const THUMBNAIL_SCALE = 0.25;

const SEGMENTS = [
    {key: 'inputMs', label: 'input'},
    {key: 'queueMs', label: 'queue'},
    {key: 'diffMs', label: 'diff'},
    {key: 'spiMs', label: 'spi'},
    {key: 'powerMs', label: 'bật nguồn'},
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
        raw: {profile: 0, windowKind: WindowKind.CHANGED, partialMode: true, ptScan: true, driveBorder: false},
        intent: 0,
        flags: 0,
        options: null,        /* cài đặt đang dùng của ink_display (từ INFO, sửa ở tab Chế độ) */
        defaultOptions: null, /* cài đặt lúc máy khởi động */
        temporaryProfile: null,
        temperatureCelsius: null,
    },
    run: null, /* {label, testId, params, startedAt, drawn: [], settings} */
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
    /* Máy vừa khởi động: slot trong PSRAM đã trắng, cài đặt về mặc định. */
    for (const slot of app.slots) { slot.uploaded = false; }
    app.settings.options = structuredClone(info.options);
    app.settings.defaultOptions = structuredClone(info.options);
    app.settings.temporaryProfile = null;
    app.settings.raw.profile = profileIndexForPurpose(0) ?? 0;
    $('device-summary').textContent = `${info.width}×${info.height} · ${info.path} · ${info.firmware}`;
    document.querySelectorAll('input[name="path"]').forEach((radio) => { radio.checked = radio.value === info.path; });
    renderProfiles();
    renderOptionsForm();
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

/* ---- Profile ---- */

function profileByIndex(index) { return app.info?.profiles[index] ?? null; }
function profileById(id) { return app.info?.profiles.find((profile) => profile.id === id) ?? null; }
function profileByName(name) { return app.info?.profiles.find((profile) => profile.name === name) ?? null; }
function profileName(index) { return profileByIndex(index)?.name ?? `#${index}`; }
function profileNameById(id) { return profileById(id)?.name ?? `id ${id}`; }

function profileLabel(profile) {
    const source = profile.source === SOURCE_HAND ? 'vẽ tay' : `TR${profile.source}`;
    return `${profile.name} · id ${profile.id} · ${source} · ${profile.drawMs} ms`;
}

/* Profile của dải chứa `celsius` trong preset, như profile_for() của ink_display. */
function bandProfileId(preset, celsius = DEFAULT_CELSIUS) {
    if (!preset?.bands.length) { return null; }
    return (preset.bands.find((band) => celsius <= band.maxCelsius) ?? preset.bands[preset.bands.length - 1]).profileId;
}

function profileIndexForPurpose(purpose) {
    const id = bandProfileId(app.settings.options?.presets[purpose], app.settings.temperatureCelsius ?? DEFAULT_CELSIUS);
    return profileById(id)?.index ?? null;
}

function renderProfiles() {
    if (!app.info) { return; }
    const options = app.info.profiles.map((profile) => ({value: profile.index, label: profileLabel(profile)}));
    fillSelect($('raw-profile'), options, app.settings.raw.profile);
}

function renderPathControls() {
    const raw = app.info?.path !== 'pipeline';
    for (const id of ['raw-profile', 'raw-window-kind', 'draw-window-kind', 'raw-partial-mode', 'raw-pt-scan', 'raw-drive-border', 'raw-power-off-ms', 'apply-raw-power',
        'power-off', 'old-source', 'old-slot', 'read-temperature', 'dump-otp']) { $(id).disabled = !raw; }
    for (const id of ['apply-options', 'reset-options', 'submit-intent', 'submit-defer', 'submit-sleep']) { $(id).disabled = raw; }
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

/* ---- Cách vẽ raw ---- */

function readRawForm(source) {
    /* Hai ô chọn window (tab Chế độ, thẻ Vẽ một slot) cùng một cài đặt: ô vừa đổi thắng. */
    const windowKind = source?.target?.id === 'draw-window-kind' ? $('draw-window-kind').value : $('raw-window-kind').value;
    $('raw-window-kind').value = windowKind;
    $('draw-window-kind').value = windowKind;
    app.settings.raw = {
        profile: Number($('raw-profile').value),
        windowKind: Number(windowKind),
        partialMode: $('raw-partial-mode').checked,
        ptScan: $('raw-pt-scan').checked,
        driveBorder: $('raw-drive-border').checked,
    };
}

function renderRawForm() {
    const raw = app.settings.raw;
    $('raw-profile').value = String(raw.profile);
    $('raw-window-kind').value = String(raw.windowKind);
    $('draw-window-kind').value = String(raw.windowKind);
    $('raw-partial-mode').checked = raw.partialMode;
    $('raw-pt-scan').checked = raw.ptScan;
    $('raw-drive-border').checked = raw.driveBorder;
}

/* Gói DRAW: slot, profile, nguồn mặt phẳng cũ, slot cũ, chế độ partial, PT_SCAN, kéo viền, kiểu window, x, y, rộng, cao. */
function drawPayload(slot, raw, oldSource, oldSlot, window) {
    const rect = window ?? {x: 0, y: 0, width: 0, height: 0};
    return new Writer().u8(slot).u8(raw.profile).u8(oldSource).u8(oldSlot)
        .u8(raw.partialMode ? 1 : 0).u8(raw.ptScan ? 1 : 0).u8(raw.driveBorder ? 1 : 0).u8(raw.windowKind)
        .u16(rect.x).u16(rect.y).u16(rect.width).u16(rect.height).result();
}

async function applyRawPower() {
    const ms = Math.max(0, Number($('raw-power-off-ms').value) || 0);
    await guarded(async () => {
        await app.link.command(Command.POWER_OFF, new Writer().u32(ms).result());
        log(`[web] raw: tự tắt nguồn sau ${ms} ms${ms === 0 ? ' (không tự tắt)' : ''}`);
    });
}

async function powerOffNow() {
    await guarded(() => app.link.command(Command.POWER_OFF));
}

async function readTemperature() {
    return guarded(async () => {
        const reply = await app.link.send(Command.READ_TEMP);
        if (reply.error !== 0) { throw new Error(`Đọc nhiệt độ lỗi ${espErrorName(reply.error)}`); }
        app.settings.temperatureCelsius = reply.celsius;
        $('temperature').textContent = `${reply.celsius} °C`;
        return reply.celsius;
    });
}

/* ---- Ý định, cài đặt pipeline ---- */

function readSubmitForm() {
    app.settings.intent = Number($('submit-intent').value);
    app.settings.flags = ($('submit-defer').checked ? SubmitFlag.DEFER_CLEANUP : 0) | ($('submit-sleep').checked ? SubmitFlag.BEFORE_SLEEP : 0);
}

const OPTION_FIELDS = [
    ['option-auto-ui', 'autoUiPercent'], ['option-debt-soft', 'debtSoft'], ['option-debt-page', 'debtPage'], ['option-debt-ceiling', 'debtCeiling'],
    ['option-debt-ceiling-max', 'debtCeilingMax'], ['option-edge-ms', 'edgeMs'], ['option-light-ms', 'lightMs'], ['option-strong-ms', 'strongMs'],
    ['option-power-off-ms', 'powerOffMs'],
];

/* Bảng preset: mỗi mục đích một hàng (profile hoặc theo dải, chế độ partial, PT_SCAN, kéo viền, trọng số). */
function renderOptionsForm() {
    const options = app.settings.options;
    if (!options || !app.info) { return; }
    const profileOptions = app.info.profiles.map((profile) => ({value: String(profile.id), label: `${profile.name} (${profile.drawMs} ms)`}));
    const header = element('tr', {}, ['Mục đích', 'Profile', 'Partial', 'PT_SCAN', 'Viền', 'Trọng số'].map((title) => element('th', {text: title})));
    const rows = PURPOSES.map((purpose, index) => {
        const preset = options.presets[index];
        const select = element('select', {id: `preset-profile-${index}`});
        const bandsLabel = preset.bands.length > 1 ? `theo dải nhiệt (${preset.bands.length} dải)` : null;
        fillSelect(select, [...(bandsLabel ? [{value: 'bands', label: bandsLabel}] : []), ...profileOptions],
                   bandsLabel ? 'bands' : String(preset.bands[0]?.profileId ?? ''));
        const check = (key, value) => {
            const input = element('input', {type: 'checkbox', id: `preset-${key}-${index}`});
            input.checked = value;
            return input;
        };
        const weight = element('input', {type: 'number', id: `preset-weight-${index}`, min: '0', max: '15', value: String(preset.debtWeight)});
        if (index > 2) { weight.disabled = true; }
        return element('tr', {}, [
            element('td', {class: 'text', text: purpose.name}),
            element('td', {}, [select]),
            element('td', {}, [check('partial', preset.partialMode)]),
            element('td', {}, [check('pt', preset.ptScan)]),
            element('td', {}, [check('border', preset.driveBorder)]),
            element('td', {}, [weight]),
        ]);
    });
    $('preset-table').replaceChildren(header, ...rows);
    for (const [id, key] of OPTION_FIELDS) { $(id).value = String(options[key]); }
}

function readOptionsForm() {
    const options = structuredClone(app.settings.options);
    options.presets.forEach((preset, index) => {
        const choice = $(`preset-profile-${index}`).value;
        if (choice !== 'bands') { preset.bands = [{maxCelsius: 127, profileId: Number(choice)}]; }
        preset.partialMode = $(`preset-partial-${index}`).checked;
        preset.ptScan = $(`preset-pt-${index}`).checked;
        preset.driveBorder = $(`preset-border-${index}`).checked;
        preset.debtWeight = Number($(`preset-weight-${index}`).value);
    });
    for (const [id, key] of OPTION_FIELDS) { options[key] = Math.max(0, Number($(id).value) || 0); }
    return options;
}

async function sendOptions(options) {
    await app.link.command(Command.SET_OPTIONS, writeOptions(new Writer(), options).result());
    app.settings.options = options;
    renderOptionsForm();
}

async function applyOptions() {
    await guarded(async () => {
        await sendOptions(readOptionsForm());
        log(`[web] cài đặt: ${optionsSummary(app.settings.options)}`);
    });
}

async function resetOptions() {
    if (!app.settings.defaultOptions) { return; }
    await guarded(() => sendOptions(structuredClone(app.settings.defaultOptions)));
}

function optionsSummary(options) {
    if (!options) { return ''; }
    const names = PURPOSES.slice(0, 3).map((purpose, index) => {
        const preset = options.presets[index];
        return `${purpose.name} ${preset.bands.length > 1 ? 'theo dải' : profileNameById(preset.bands[0]?.profileId)}`;
    });
    return `${names.join(', ')} · nợ ${options.debtSoft}/${options.debtPage}/${options.debtCeiling}–${options.debtCeilingMax}`
        + ` · dọn ${options.edgeMs}/${options.lightMs}/${options.strongMs} ms · tắt nguồn ${options.powerOffMs} ms`;
}

/* ---- Profile tạm, OTP ---- */

async function loadTemporaryProfile() {
    const file = $('profile-file').files[0];
    if (!file) {
        toast('Chọn file .inkw trước.', true);
        return;
    }
    await guarded(async () => {
        const bytes = new Uint8Array(await file.arrayBuffer());
        await app.link.command(Command.PROFILE_LOAD, bytes);
        app.settings.temporaryProfile = file.name;
        log(`[web] nạp profile tạm ${file.name}; có tác dụng từ lần vẽ sau`);
        app.info = await app.link.hello();
        renderProfiles();
        renderOptionsForm();
    });
}

async function dumpOtp() {
    await guarded(async () => {
        const chunks = [];
        const onOtp = (event) => chunks.push(event.detail);
        app.link.addEventListener('otp', onOtp);
        try {
            await app.link.command(Command.OTP_DUMP, new Uint8Array(0), 10000);
        } finally {
            app.link.removeEventListener('otp', onOtp);
        }
        const length = Math.max(0, ...chunks.map((chunk) => chunk.offset + chunk.bytes.length));
        const otp = new Uint8Array(length);
        for (const chunk of chunks) { otp.set(chunk.bytes, chunk.offset); }
        const lines = ['# OTP đọc bằng lệnh 0xA2 qua bench, ' + new Date().toISOString()];
        for (let offset = 0; offset < otp.length; offset += 16) {
            lines.push([...otp.subarray(offset, offset + 16)].map((byte) => byte.toString(16).toUpperCase().padStart(2, '0')).join(' '));
        }
        const link = element('a', {href: URL.createObjectURL(new Blob([lines.join('\n') + '\n'], {type: 'text/plain'})), download: 'otp_dump.hex'});
        link.click();
        log(`[web] đọc OTP ${otp.length} byte, đã tải otp_dump.hex`);
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
    const raw = app.settings.raw;
    return {
        path: app.info?.path ?? null,
        firmware: app.info?.firmware ?? null,
        orientation: app.orientation,
        rawProfile: profileName(raw.profile),
        raw: {...raw},
        intent: INTENT_NAMES[app.settings.intent],
        flags: app.settings.flags,
        options: app.info?.path === 'pipeline' ? structuredClone(app.settings.options) : null,
        optionsSummary: app.info?.path === 'pipeline' ? optionsSummary(app.settings.options) : null,
        temporaryProfile: app.settings.temporaryProfile,
    };
}

/* expected: số khung sẽ vẽ, để hiện tiến độ N/M; 0 = không biết trước. */
function startRun(label, details = {}, expected = 0) {
    app.run = {label, startedAt: new Date().toISOString(), drawn: [], settings: snapshotSettings(), expected, ...details};
    renderRun();
    renderProgress();
}

function framesDrawn(run) { return run.drawn.filter((item) => item.frame).length; }

function renderProgress() {
    const run = app.run;
    const frames = run ? framesDrawn(run) : 0;
    const extra = run ? run.drawn.length - frames : 0;
    const text = !run ? '' : (run.expected ? `${run.label}: ${frames}/${run.expected}` : `${run.label}: ${frames} khung`) + (extra ? ` + ${extra} lượt dọn/tắt nguồn` : '');
    $('run-progress').textContent = text;
    if (run?.expected && frames === run.expected && !run.announced) {
        run.announced = true;
        toast(`${run.label} xong. Mở tab Chấm điểm để chấm.`);
    }
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
            await app.link.command(Command.SUBMIT, new Writer().u8(CLEAR_SLOT).u8(3).u8(0).result());
            return;
        }
        /* dọn mạnh: profile của preset dọn mạnh, cả màn ngoài chế độ partial, mặt phẳng cũ đảo bit */
        const raw = {profile: profileIndexForPurpose(CLEAN_STRONG) ?? app.settings.raw.profile, windowKind: WindowKind.WHOLE, partialMode: false,
            ptScan: true, driveBorder: true};
        await app.link.command(Command.DRAW, drawPayload(CLEAR_SLOT, raw, OldSource.INVERTED, 0, null));
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
            readSubmitForm();
            await app.link.command(Command.SUBMIT, new Writer().u8(slot).u8(app.settings.intent).u8(app.settings.flags).result());
            return;
        }
        readRawForm();
        const oldSource = Number($('old-source').value);
        const oldSlot = Number($('old-slot').value);
        if (oldSource === OldSource.SLOT) { await ensureUploaded(oldSlot); }
        app.manualDrawPending = true;
        $('draw-summary').textContent = `${profileName(app.settings.raw.profile)} · ${$('draw-window-kind').selectedOptions[0].text} · ${$('old-source').selectedOptions[0].text}`;
        await app.link.command(Command.DRAW, drawPayload(slot, app.settings.raw, oldSource, oldSlot, app.selectedWindow));
    });
}

/* ---- Màn đang hiện, gói DRAWN ---- */

function onDrawn(drawn) {
    const frame = app.pendingPreview.get(drawn.slot) ?? app.slots[drawn.slot]?.frame;
    if (drawn.frame && drawn.action !== Action.NONE && drawn.error === 0 && frame) { app.shown = {frame, window: drawn.window}; }
    if (drawn.temperatureValid) { app.settings.temperatureCelsius = drawn.temperatureCelsius; }
    renderFrame($('shown-preview'), app.shown.frame, app.orientation, PREVIEW_SCALE, drawn.action === Action.POWER_OFF ? app.shown.window : drawn.window);
    renderLastDrawn(drawn);
    if (app.manualDrawPending && drawn.frame) {
        app.manualDrawPending = false;
        if (drawn.action === Action.NONE) {
            toast('Máy không vẽ: khung mới giống hệt mặt phẳng cũ trong window "Vùng đổi". Chọn window "Cả màn" hoặc mặt phẳng cũ "Khung mới đảo bit".', true);
        }
    }
    if (app.run) {
        app.run.drawn.push(drawn);
        renderRun();
        renderProgress();
    }
    if (drawn.frame) { highlightSequence(drawn.slot); }
    if (!drawn.frame) { log(`[web] ${drawn.actionName}${drawn.action === Action.POWER_OFF ? '' : ` ${profileNameById(drawn.profileId)}, panel ${format(drawn.panelMs, 1)} ms`}`); }
}

/* input chỉ có khi khung do nút bấm; queue chỉ có ở đường pipeline. */
function segmentText(drawn, key) {
    if (key === 'inputMs' && drawn.inputMs === 0) { return '– (không do nút)'; }
    if (key === 'queueMs' && app.info?.path !== 'pipeline') { return '– (raw)'; }
    return `${format(drawn[key], 1)} ms`;
}

function renderLastDrawn(drawn) {
    const items = [
        ['việc', drawn.frame ? `${drawn.actionName} (khung slot ${drawn.slot})` : drawn.actionName],
        ['mục đích', drawn.purposeName],
        ['profile', drawn.action === Action.POWER_OFF ? '–' : profileNameById(drawn.profileId)],
        ['window', `${drawn.window.x},${drawn.window.y} ${drawn.window.width}×${drawn.window.height}`],
        ['nhiệt độ', drawn.temperatureValid ? `${drawn.temperatureCelsius} °C` : '–'],
        ['nợ lớn nhất', `${drawn.maxDebt} / trần ${drawn.ceiling}`],
        ['điểm có cờ viền', String(drawn.flaggedCount)],
        ['lỗi', drawn.error === 0 ? 'không' : espErrorName(drawn.error)],
        ...SEGMENTS.map(({key, label}) => [label, segmentText(drawn, key)]),
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

/* Gửi khung còn thiếu rồi gói PLAY: ms giữa hai khung, số vòng, ý định, cờ, số slot, các slot. */
async function play(slots, intervalMs, repeat, intent, flags) {
    for (const slot of new Set(slots)) {
        await ensureUploaded(slot);
        app.pendingPreview.set(slot, app.slots[slot].frame);
    }
    const payload = new Writer().u16(Math.min(65535, intervalMs)).u8(repeat).u8(intent).u8(flags).u8(slots.length).array(slots);
    await app.link.command(Command.PLAY, payload.result());
}

async function playSequence() {
    if (app.sequence.length === 0) {
        log('[web] chuỗi trống: nhấp các slot để thêm', 'warning');
        return;
    }
    const intervalMs = Number($('sequence-interval').value);
    const repeat = Number($('sequence-repeat').value);
    readSubmitForm();
    await guarded(async () => {
        app.lastRun = playSequence;
        startRun('Chuỗi tay', {testId: 'sequence', params: {slots: [...app.sequence], intervalMs, repeat}}, repeat > 0 ? app.sequence.length * repeat : 0);
        await play(app.sequence.slice(0, 64), intervalMs, repeat, app.settings.intent, app.settings.flags);
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
        text: test.path ? `${test.name} (${test.path})` : test.name,
        onclick: () => { app.selectedTestId = test.id; renderTests(); },
    })));
    const test = selectedTest();
    const inputs = test.params.map((param) => {
        if (param.type === 'profile') {
            const select = element('select', {id: `param-${param.key}`});
            const profiles = app.info?.profiles ?? [];
            fillSelect(select, profiles.map((profile) => ({value: profile.index, label: profileLabel(profile)})), profileByName(param.value)?.index ?? 0);
            return select;
        }
        if (param.type === 'bool') {
            const input = element('input', {type: 'checkbox', id: `param-${param.key}`});
            input.checked = param.value;
            return input;
        }
        return element('input', {
            type: 'number', id: `param-${param.key}`, min: String(param.min), max: String(param.max), step: String(param.step ?? 1), value: String(param.value),
        });
    });
    $('test-detail').replaceChildren(
        element('h2', {text: test.name}),
        element('p', {text: test.description}),
        element('div', {class: 'form-grid'}, test.params.flatMap((param, index) => [element('label', {for: `param-${param.key}`, text: param.label}), inputs[index]])),
        element('div', {class: 'row'}, [element('button', {class: 'primary', text: 'Chạy bài này', onclick: runSelectedTest})]),
        element('p', {class: 'muted', text: `Khung gửi vào slot ${TEST_FIRST_SLOT}–${TEST_FIRST_SLOT + TEST_SLOT_COUNT - 1}. Hướng nhìn theo tab Ảnh. `
            + `Bài cần đường ${test.path ?? 'nào cũng được'}; đường hiện tại: ${app.info?.path ?? 'chưa kết nối'}.`}),
    );
}

function readTestValues(test) {
    return Object.fromEntries(test.params.map((param) => {
        const input = $(`param-${param.key}`);
        if (param.type === 'profile') { return [param.key, Number(input.value)]; }
        if (param.type === 'bool') { return [param.key, input.checked]; }
        const value = Number(input.value);
        return [param.key, Math.max(param.min, Math.min(param.max, Number.isFinite(value) ? value : param.value))];
    }));
}

/* Cách vẽ raw của bài: cài đặt ở tab Chế độ, ghi đè bằng phần raw của kế hoạch. */
function rawSettingsFor(plan) {
    readRawForm();
    return {...app.settings.raw, ...(plan.raw ?? {})};
}

/* Đường raw: gói DRAW cho khung đầu (đặt cách vẽ), chờ vẽ xong, rồi PLAY phần còn lại. */
async function runRawPlan(plan, slots) {
    const raw = rawSettingsFor(plan);
    app.run.settings.raw = raw;
    app.run.settings.rawProfile = profileName(raw.profile);
    await ensureUploaded(slots[0]);
    app.pendingPreview.set(slots[0], app.slots[slots[0]].frame);
    await app.link.command(Command.DRAW, drawPayload(slots[0], raw, plan.oldSource ?? OldSource.SHOWN, 0, plan.window ?? null));
    if (slots.length === 1) { return; }
    await waitForDraws(app.run, 1);
    await delay(plan.intervalMs);
    await playChunks(slots.slice(1), plan, 0, 0);
}

async function playChunks(slots, plan, intent, flags) {
    for (let start = 0; start < slots.length; start += 64) {
        if (start > 0) {
            log('[web] chuỗi dài hơn 64 khung: chạy phần sau khi phần trước xong', 'warning');
            await waitForDraws(app.run, framesDrawn(app.run) + 64);
        }
        await play(slots.slice(start, start + 64), plan.intervalMs, plan.repeat, intent, flags);
    }
}

async function runPipelinePlan(plan, slots) {
    if (plan.optionsPatch) {
        await sendOptions({...structuredClone(app.settings.options), ...plan.optionsPatch});
        app.run.settings.options = structuredClone(app.settings.options);
        app.run.settings.optionsSummary = optionsSummary(app.settings.options);
        log(`[web] bài đổi cài đặt: ${optionsSummary(app.settings.options)}`);
    }
    readSubmitForm();
    const intent = plan.intent ?? app.settings.intent;
    const flags = plan.flags ?? app.settings.flags;
    app.run.settings.intent = INTENT_NAMES[intent];
    await playChunks(slots, plan, intent, flags);
}

/* Bài đọc nhiệt độ: vẽ khung đầu bằng cách vẽ raw (REG = 1), rồi đọc cảm biến nhiều lần. */
async function runTemperaturePlan(plan, slots) {
    await runRawPlan({...plan, intervalMs: 0}, slots.slice(0, 1));
    await waitForDraws(app.run, 1);
    const readings = [];
    for (let i = 0; i < plan.readings; i++) {
        await delay(plan.intervalMs);
        const celsius = await readTemperature();
        if (celsius !== undefined) { readings.push(celsius); }
    }
    app.run.temperatures = readings;
    log(`[web] nhiệt độ đọc được: ${readings.join(', ')} °C`);
    renderRun();
}

/* Bài giữ nguồn: vẽ, giữ nguồn analog bật trong holdMs (đo dòng), rồi tắt; trả lại tự tắt 2 s. */
async function runHoldPowerPlan(plan, slots) {
    await app.link.command(Command.POWER_OFF, new Writer().u32(0).result());
    await runRawPlan({...plan, intervalMs: 0}, slots.slice(0, 1));
    await waitForDraws(app.run, 1);
    log(`[web] nguồn đang BẬT trong ${plan.holdMs / 1000} s: đo dòng bây giờ`);
    toast(`Nguồn bật: đo dòng trong ${plan.holdMs / 1000} s`);
    await delay(plan.holdMs);
    await app.link.command(Command.POWER_OFF);
    log('[web] nguồn đã TẮT: đo dòng lần nữa để so');
    toast('Nguồn đã tắt: đo dòng lần nữa');
    await delay(plan.holdMs);
    await app.link.command(Command.POWER_OFF, new Writer().u32(Number($('raw-power-off-ms').value) || 2000).result());
}

async function runSelectedTest() {
    const test = selectedTest();
    if (test.path && app.info?.path && test.path !== app.info.path) {
        toast(`Bài "${test.name}" chạy ở đường ${test.path}: đổi đường ở tab Chế độ trước.`, true);
        return;
    }
    const values = readTestValues(test);
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
        app.lastRun = runSelectedTest;
        startRun(test.name, {testId: test.id, params: values}, plan.repeat === 1 && !plan.special ? slots.length : 0);
        if (plan.note) { log(`[web] ${plan.note}`); }
        const raw = app.info?.path !== 'pipeline';
        if (plan.special === 'temperature') {
            await runTemperaturePlan(plan, slots);
        } else if (plan.special === 'hold_power') {
            await runHoldPowerPlan(plan, slots);
        } else if (raw) {
            await runRawPlan(plan, slots);
        } else {
            await runPipelinePlan(plan, slots);
        }
    });
}

/* Chờ tới khi lần chạy có đủ `count` gói DRAWN (chuỗi dài hơn 64 khung chia nhiều gói PLAY). */
async function waitForDraws(run, count) {
    while (app.run === run && framesDrawn(run) < count) { await delay(100); }
}

/* ---- Thống kê, chấm điểm ---- */

/* Thời gian theo đoạn, tách theo việc đã làm (vẽ khung, từng lượt dọn). */
function statistics(drawn) {
    const done = drawn.filter((item) => item.action !== Action.NONE && item.action !== Action.POWER_OFF && item.error === 0);
    const summarize = (items) => {
        const result = {count: items.length};
        for (const {key} of SEGMENTS) {
            const values = items.map((item) => item[key]);
            result[key] = values.length
                ? {average: values.reduce((sum, value) => sum + value, 0) / values.length, minimum: Math.min(...values), maximum: Math.max(...values)}
                : null;
        }
        return result;
    };
    const byAction = {};
    for (const action of new Set(done.map((item) => item.action))) {
        byAction[ACTION_NAMES[action]] = summarize(done.filter((item) => item.action === action));
    }
    const frames = done.filter((item) => item.frame);
    return {
        ...summarize(frames),
        byAction,
        cleanups: done.filter((item) => !item.frame).length,
        maxFlagged: Math.max(0, ...drawn.map((item) => item.flaggedCount ?? 0)),
        maxDebt: Math.max(0, ...drawn.map((item) => item.maxDebt ?? 0)),
        frameAverageMs: frames.length ? frames.reduce((sum, item) => sum + item.totalMs, 0) / frames.length : null,
    };
}

function renderRun() {
    const run = app.run;
    if (!run) { return; }
    const stats = statistics(run.drawn);
    const settings = run.settings;
    const how = settings.path === 'pipeline' ? `ý định ${settings.intent} · ${settings.optionsSummary}` : `raw ${settings.rawProfile}`;
    $('run-summary').textContent = `${run.label} · ${settings.path ?? '?'} · ${how} · ${stats.count} khung, ${stats.cleanups} lượt dọn`
        + ` · nợ lớn nhất ${stats.maxDebt} · cờ viền nhiều nhất ${stats.maxFlagged}`
        + (run.temperatures ? ` · nhiệt độ ${run.temperatures.join(', ')} °C` : app.settings.temperatureCelsius !== null ? ` · ${app.settings.temperatureCelsius} °C` : '');
    const rows = [['khung', stats], ...Object.entries(stats.byAction).filter(([name]) => name !== 'vẽ').map(([name, value]) => [name, value])];
    $('run-stats').replaceChildren(
        element('tr', {}, [element('th', {text: 'việc'}), element('th', {text: 'lần'}), element('th', {text: 'đoạn (ms)'}), element('th', {text: 'trung bình'}),
            element('th', {text: 'nhỏ nhất'}), element('th', {text: 'lớn nhất'})]),
        ...rows.flatMap(([name, value]) => SEGMENTS.filter(({key}) => name === 'khung' || !['inputMs', 'queueMs'].includes(key)).map(({key, label}, index) => element('tr', {}, [
            element('td', {class: 'text', text: index === 0 ? name : ''}),
            element('td', {text: index === 0 ? String(value.count) : ''}),
            element('td', {text: label}),
            element('td', {text: format(value[key]?.average, 1)}),
            element('td', {text: format(value[key]?.minimum, 1)}),
            element('td', {text: format(value[key]?.maximum, 1)}),
        ]))),
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
    const colors = [style.getPropertyValue('--brand-teal'), style.getPropertyValue('--brand-purple'), style.getPropertyValue('--accent'),
        style.getPropertyValue('--brand-bronze')];
    const parts = ['spiMs', 'powerMs', 'panelMs', 'afterMs'];
    const items = drawn.filter((item) => item.action !== Action.NONE && item.action !== Action.POWER_OFF);
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
    ['spi', 'bật nguồn', 'panel', 'after'].forEach((label, index) => {
        context.fillStyle = colors[index];
        context.fillRect(left + index * 90, height - 12, 10, 10);
        context.fillStyle = style.getPropertyValue('--text');
        context.fillText(label, left + index * 90 + 14, height - 3);
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
        startedAt: run.startedAt,
        ...run.settings,
        temperatureCelsius: app.settings.temperatureCelsius,
        temperatures: run.temperatures ?? null,
        drawCount: stats.count,
        cleanupCount: stats.cleanups,
        maxDebt: stats.maxDebt,
        maxFlagged: stats.maxFlagged,
        timing: Object.fromEntries(SEGMENTS.map(({key}) => [key, stats[key]])),
        timingByAction: stats.byAction,
        frameAverageMs: stats.frameAverageMs,
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

/* Profile của một dòng lịch sử: raw thì profile raw, pipeline thì profile TEXT/UI; dòng cũ thì full/partial. */
function historyProfiles(item) {
    if (item.rawProfile && item.path !== 'pipeline') { return [item.rawProfile]; }
    if (item.options) { return item.options.presets.slice(0, 2).map((preset) => profileNameById(preset.bands[0]?.profileId)); }
    return [item.fullProfile, item.partialProfile].filter(Boolean);
}

async function loadHistory() {
    app.history = await fetch('/api/results').then((response) => response.json()).catch(() => []);
    const tests = [...new Set(app.history.map((item) => item.label ?? item.test))];
    const profiles = [...new Set(app.history.flatMap(historyProfiles))];
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
        .filter((item) => !profileFilter || historyProfiles(item).includes(profileFilter))
        .slice()
        .reverse();
    const header = ['Lúc', 'Bài', 'Đường', 'Profile', 'Ý định', '°C', 'Khung', 'Dọn', 'Khung TB', 'Panel TB', ...SCORES.map((score) => score.short), 'Ghi chú'];
    $('history-table').replaceChildren(
        element('tr', {}, header.map((title) => element('th', {text: title}))),
        ...rows.map((item) => element('tr', {}, [
            element('td', {text: (item.saved_at ?? '').replace('T', ' ')}),
            element('td', {class: 'text', text: item.label ?? item.test}),
            element('td', {text: item.path ?? '–'}),
            element('td', {class: 'text', text: historyProfiles(item).join(', ') || '–'}),
            element('td', {text: item.path === 'pipeline' ? item.intent ?? '–' : '–'}),
            element('td', {text: item.temperatureCelsius ?? '–'}),
            element('td', {text: String(item.drawCount ?? '–')}),
            element('td', {text: String(item.cleanupCount ?? '–')}),
            element('td', {text: format(item.frameAverageMs ?? item.partialAverageMs, 0)}),
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
    $('send-and-draw').addEventListener('click', sendAndDraw);
    $('quick-stop').addEventListener('click', stopSequence);
    $('quick-rerun').addEventListener('click', rerun);
    $('quick-clear').addEventListener('click', clearScreen);
    document.addEventListener('keydown', (event) => {
        if (event.target.closest('input, textarea, select')) { return; }
        if (event.key === 'Escape') { stopSequence(); }
        if (event.key === 'r' || event.key === 'R') { rerun(); }
    });
    $('apply-options').addEventListener('click', applyOptions);
    $('reset-options').addEventListener('click', resetOptions);
    $('read-temperature').addEventListener('click', readTemperature);
    $('apply-raw-power').addEventListener('click', applyRawPower);
    $('power-off').addEventListener('click', powerOffNow);
    $('load-profile').addEventListener('click', loadTemporaryProfile);
    $('dump-otp').addEventListener('click', dumpOtp);
    for (const id of ['raw-profile', 'raw-window-kind', 'draw-window-kind', 'raw-partial-mode', 'raw-pt-scan', 'raw-drive-border']) { $(id).addEventListener('change', readRawForm); }
    for (const id of ['submit-intent', 'submit-defer', 'submit-sleep']) { $(id).addEventListener('change', readSubmitForm); }

    fillSelect($('dither'), Object.values(DitherMethod).map((method) => ({value: method, label: DITHER_NAMES[method]})), DitherMethod.FLOYD_STEINBERG);
    renderRawForm();
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
