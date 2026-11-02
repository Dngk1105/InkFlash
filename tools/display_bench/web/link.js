/* Thiết kế: docs/design/display/bench.md */

/* Web Serial và khung gói với firmware test (bench_link.c). Gói: A5 5A | loại | seq | độ dài (2) | payload | CRC16. */

/* ---- Hằng số giao thức ---- */

export const Command = Object.freeze({
    HELLO: 0x01,
    SET_PATH: 0x02,
    UPLOAD: 0x03,
    DRAW: 0x04,
    SUBMIT: 0x05,
    PLAY: 0x06,
    STOP: 0x07,
    SELECT_PROFILE: 0x08,
    TUNE: 0x09,
    SET_OPTIONS: 0x0a,
    READ_TEMP: 0x0b,
});

const Event = Object.freeze({ACK: 0x80, INFO: 0x81, DRAWN: 0x82, GESTURE: 0x83, TEMP: 0x84, LOG: 0x85});

const UPLOAD_CHUNK_BYTES = 4096;
const REPLY_TIMEOUT_MS = 3000;
const RECONNECT_DELAY_MS = 2500;
const RECONNECT_ATTEMPTS = 10;

/* CRC-16/CCITT-FALSE: đa thức 0x1021, khởi đầu 0xFFFF; giống crc16() trong bench_link.c. */
function crc16(bytes, crc = 0xffff) {
    for (const byte of bytes) {
        crc ^= byte << 8;
        for (let bit = 0; bit < 8; bit++) { crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff; }
    }
    return crc;
}

/* ---- Đọc payload ---- */

class Reader {
    constructor(bytes) {
        this.view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        this.offset = 0;
    }
    u8() { return this.view.getUint8(this.offset++); }
    i8() { return this.view.getInt8(this.offset++); }
    u16() { const value = this.view.getUint16(this.offset, true); this.offset += 2; return value; }
    u32() { const value = this.view.getUint32(this.offset, true); this.offset += 4; return value; }
    i32() { const value = this.view.getInt32(this.offset, true); this.offset += 4; return value; }
    text() {
        const length = this.u8();
        const bytes = new Uint8Array(this.view.buffer, this.view.byteOffset + this.offset, length);
        this.offset += length;
        return new TextDecoder().decode(bytes);
    }
}

function parseInfo(payload) {
    const reader = new Reader(payload);
    const info = {
        path: reader.u8() === 1 ? 'pipeline' : 'raw',
        width: reader.u16(),
        height: reader.u16(),
        xAlign: reader.u8(),
        slotCount: reader.u8(),
        profileCount: reader.u8(),
        defaultFull: reader.u8(),
        defaultPartial: reader.u8(),
    };
    info.firmware = reader.text();
    info.profiles = [];
    for (let i = 0; i < info.profileCount; i++) { info.profiles.push({typicalMs: reader.u16(), name: reader.text()}); }
    return info;
}

const MODE_NAMES = ['không vẽ', 'partial', 'full'];

function parseDrawn(payload) {
    const reader = new Reader(payload);
    const drawn = {
        slot: reader.u8(),
        mode: MODE_NAMES[reader.u8()] ?? '?',
        profile: reader.u8(),
        temperatureCelsius: reader.i8(),
        temperatureValid: reader.u8() !== 0,
    };
    reader.u8();
    drawn.error = reader.i32();
    drawn.window = {x: reader.u16(), y: reader.u16(), width: reader.u16(), height: reader.u16()};
    drawn.inputMs = reader.u32();
    drawn.queueMs = reader.u32();
    drawn.diffMs = reader.u32();
    drawn.spiMs = reader.u32() / 1000;
    drawn.panelMs = reader.u32() / 1000;
    drawn.afterMs = reader.u32() / 1000;
    drawn.totalMs = drawn.inputMs + drawn.queueMs + drawn.diffMs + drawn.spiMs + drawn.panelMs + drawn.afterMs;
    drawn.receivedAt = Date.now();
    return drawn;
}

const GESTURE_TYPES = ['press', 'double press', 'hold', 'hold A+B ngắn', 'hold A+B dài'];
const BUTTON_NAMES = {1: 'A', 2: 'B', 3: 'A+B'};

function parseGesture(payload) {
    const reader = new Reader(payload);
    const modifier = reader.u8();
    const button = reader.u8();
    const type = reader.u8();
    reader.u8();
    return {
        modifier: BUTTON_NAMES[modifier] ?? '',
        button: BUTTON_NAMES[button] ?? String(button),
        type: GESTURE_TYPES[type] ?? String(type),
        timeMs: reader.u32(),
    };
}

/* ---- Ghi payload ---- */

export class Writer {
    constructor() { this.bytes = []; }
    u8(value) { this.bytes.push(value & 0xff); return this; }
    u16(value) { return this.u8(value).u8(value >> 8); }
    array(values) { for (const value of values) { this.u8(value); } return this; }
    result() { return new Uint8Array(this.bytes); }
}

/* ---- Liên kết ---- */

/*
 * Sự kiện phát ra: 'info', 'drawn', 'gesture', 'log', 'state' (detail: 'connected' | 'disconnected' | 'reconnecting').
 * send() chờ ACK (hoặc INFO, TEMP) cùng seq.
 */
export class BenchLink extends EventTarget {
    constructor() {
        super();
        this.port = null;
        this.reader = null;
        this.sequence = 0;
        this.pending = new Map(); /* seq → {resolve, reject, timer} */
        this.buffer = new Uint8Array(0);
        this.portFilter = null; /* vendor/product của cổng đã chọn, để mở lại sau khi máy khởi động lại */
    }

    get connected() { return this.port !== null; }

    async connect() {
        if (!('serial' in navigator)) { throw new Error('Trình duyệt không có Web Serial: dùng Chrome hoặc Edge, mở qua localhost'); }
        const port = await navigator.serial.requestPort();
        const portInfo = port.getInfo();
        this.portFilter = {usbVendorId: portInfo.usbVendorId, usbProductId: portInfo.usbProductId};
        await this.open(port);
    }

    /* Cổng đã được cấp quyền ở lần trước (navigator.serial.getPorts). */
    async openGranted(port) {
        const portInfo = port.getInfo();
        this.portFilter = {usbVendorId: portInfo.usbVendorId, usbProductId: portInfo.usbProductId};
        await this.open(port);
    }

    async open(port) {
        await port.open({baudRate: 115200, bufferSize: 65536});
        /* Hạ RTS trước rồi mới hạ DTR: RTS cao cùng DTR thấp là lệnh reset chip của USB-Serial-JTAG. */
        await port.setSignals({requestToSend: false});
        await port.setSignals({dataTerminalReady: false});
        this.port = port;
        this.buffer = new Uint8Array(0);
        this.emit('state', 'connected');
        this.readLoop(port);
    }

    async disconnect() {
        const port = this.port;
        this.port = null;
        if (this.reader) { await this.reader.cancel().catch(() => {}); }
        if (port) { await port.close().catch(() => {}); }
        this.rejectPending(new Error('Đã ngắt kết nối'));
        this.emit('state', 'disconnected');
    }

    /* Máy khởi động lại (đổi đường vẽ): cổng USB mất rồi hiện lại; mở lại cổng đã được cấp quyền. */
    async reconnect() {
        await this.disconnect();
        this.emit('state', 'reconnecting');
        for (let attempt = 0; attempt < RECONNECT_ATTEMPTS; attempt++) {
            await delay(RECONNECT_DELAY_MS);
            const ports = await navigator.serial.getPorts();
            const port = ports.find((candidate) => {
                const candidateInfo = candidate.getInfo();
                return candidateInfo.usbVendorId === this.portFilter?.usbVendorId && candidateInfo.usbProductId === this.portFilter?.usbProductId;
            });
            if (!port) { continue; }
            try {
                await this.open(port);
                return;
            } catch {
                /* cổng chưa sẵn sàng, thử lại */
            }
        }
        throw new Error('Không mở lại được cổng sau khi máy khởi động lại');
    }

    emit(type, detail) { this.dispatchEvent(new CustomEvent(type, {detail})); }

    rejectPending(error) {
        for (const {reject, timer} of this.pending.values()) {
            clearTimeout(timer);
            reject(error);
        }
        this.pending.clear();
    }

    /* ---- Gửi ---- */

    async send(type, payload = new Uint8Array(0), timeoutMs = REPLY_TIMEOUT_MS) {
        if (!this.port?.writable) { throw new Error('Chưa kết nối'); }
        this.sequence = (this.sequence + 1) & 0xff;
        const seq = this.sequence;
        const body = new Uint8Array(4 + payload.length);
        body.set([type, seq, payload.length & 0xff, payload.length >> 8]);
        body.set(payload, 4);
        const crc = crc16(body);
        const packet = new Uint8Array(2 + body.length + 2);
        packet.set([0xa5, 0x5a]);
        packet.set(body, 2);
        packet.set([crc & 0xff, crc >> 8], 2 + body.length);

        const reply = new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.pending.delete(seq);
                reject(new Error(`Máy không trả lời gói 0x${type.toString(16)}`));
            }, timeoutMs);
            this.pending.set(seq, {resolve, reject, timer});
        });
        const writer = this.port.writable.getWriter();
        try {
            await writer.write(packet);
        } finally {
            writer.releaseLock();
        }
        return reply;
    }

    /* Gửi xong thì kiểm mã lỗi trong ACK. */
    async command(type, payload, timeoutMs) {
        const reply = await this.send(type, payload, timeoutMs);
        if (reply.kind === 'ack' && reply.error !== 0) { throw new Error(`Máy báo lỗi ${espErrorName(reply.error)} cho gói 0x${type.toString(16)}`); }
        return reply;
    }

    async hello() { return (await this.command(Command.HELLO)).info; }

    async upload(slot, frame) {
        for (let offset = 0; offset < frame.length; offset += UPLOAD_CHUNK_BYTES) {
            const chunk = frame.subarray(offset, offset + UPLOAD_CHUNK_BYTES);
            const payload = new Uint8Array(4 + chunk.length);
            payload.set([slot, 0, offset & 0xff, offset >> 8]);
            payload.set(chunk, 4);
            await this.command(Command.UPLOAD, payload);
        }
    }

    /* ---- Đọc ---- */

    async readLoop(port) {
        while (port.readable && this.port === port) {
            this.reader = port.readable.getReader();
            try {
                for (;;) {
                    const {value, done} = await this.reader.read();
                    if (done) { break; }
                    this.receive(value);
                }
            } catch (error) {
                this.emit('log', `[web] đọc cổng lỗi: ${error.message}`);
            } finally {
                this.reader.releaseLock();
                this.reader = null;
            }
        }
        if (this.port === port) {
            this.port = null;
            this.rejectPending(new Error('Cổng đã đóng'));
            this.emit('state', 'disconnected');
        }
    }

    receive(bytes) {
        const joined = new Uint8Array(this.buffer.length + bytes.length);
        joined.set(this.buffer);
        joined.set(bytes, this.buffer.length);
        let start = 0;
        for (;;) {
            const sync = findSync(joined, start);
            if (sync < 0) {
                start = joined.length > 0 && joined[joined.length - 1] === 0xa5 ? joined.length - 1 : joined.length;
                break;
            }
            if (joined.length < sync + 6) { start = sync; break; }
            const length = joined[sync + 4] | (joined[sync + 5] << 8);
            const end = sync + 6 + length + 2;
            if (length > 8192) { start = sync + 2; continue; }
            if (joined.length < end) { start = sync; break; }
            const body = joined.subarray(sync + 2, sync + 6 + length);
            const crc = joined[end - 2] | (joined[end - 1] << 8);
            if (crc16(body) === crc) {
                this.dispatchPacket(body[0], body[1], body.slice(4));
                start = end;
            } else {
                start = sync + 2;
            }
        }
        this.buffer = joined.slice(start);
    }

    dispatchPacket(type, seq, payload) {
        switch (type) {
            case Event.ACK: this.resolve(seq, {kind: 'ack', error: new Reader(payload).i32()}); break;
            case Event.INFO: this.resolve(seq, {kind: 'info', error: 0, info: parseInfo(payload)}); break;
            case Event.TEMP: {
                const reader = new Reader(payload);
                const celsius = reader.i8();
                reader.u8(); reader.u16();
                this.resolve(seq, {kind: 'temperature', celsius, error: reader.i32()});
                break;
            }
            case Event.DRAWN: this.emit('drawn', parseDrawn(payload)); break;
            case Event.GESTURE: this.emit('gesture', parseGesture(payload)); break;
            case Event.LOG: this.emit('log', new TextDecoder().decode(payload)); break;
            default: this.emit('log', `[web] gói lạ 0x${type.toString(16)}`);
        }
    }

    resolve(seq, reply) {
        const pending = this.pending.get(seq);
        if (!pending) { return; }
        clearTimeout(pending.timer);
        this.pending.delete(seq);
        pending.resolve(reply);
    }
}

function findSync(bytes, from) {
    for (let i = from; i + 1 < bytes.length; i++) {
        if (bytes[i] === 0xa5 && bytes[i + 1] === 0x5a) { return i; }
    }
    return -1;
}

export function delay(ms) { return new Promise((resolve) => setTimeout(resolve, ms)); }

/* Tên mã lỗi esp_err_t hay gặp. */
const ESP_ERROR_NAMES = {
    [-1]: 'ESP_FAIL',
    0x101: 'ESP_ERR_NO_MEM',
    0x102: 'ESP_ERR_INVALID_ARG',
    0x103: 'ESP_ERR_INVALID_STATE',
    0x106: 'ESP_ERR_NOT_SUPPORTED',
    0x107: 'ESP_ERR_TIMEOUT',
};

export function espErrorName(code) { return ESP_ERROR_NAMES[code] ?? `0x${(code >>> 0).toString(16)}`; }
