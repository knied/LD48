function Pipeline(gl, attributes, uniforms,
                  vertexShaderSource,
                  fragmentShaderSource) {
    function createShader(type, source) {
        var shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        var success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
        if (success) {
            return shader;
        }
        console.log(gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
    }
    function createProgram(vertexShader, fragmentShader) {
        var program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);
        var success = gl.getProgramParameter(program, gl.LINK_STATUS);
        if (success) {
            return program;
        }
        console.log(gl.getProgramInfoLog(program));
        gl.deleteProgram(program);
    }
    const vertexShader = createShader(gl.VERTEX_SHADER, vertexShaderSource);
    const fragmentShader = createShader(gl.FRAGMENT_SHADER, fragmentShaderSource);
    this.program = createProgram(vertexShader, fragmentShader);
    gl.deleteShader(fragmentShader);
    gl.deleteShader(vertexShader);
    this.attribLocs = {};
    for (const i in attributes) {
        const attribute = attributes[i];
        const location = gl.getAttribLocation(this.program, attribute);
        if (location < 0) {
            throw "Missing attribute '" + attribute + "' in pipeline.";
        }
        this.attribLocs[attribute] = location;
    }
    this.uniformLocs = {};
    for (const i in uniforms) {
        const uniform = uniforms[i];
        const location = gl.getUniformLocation(this.program, uniform);
        if (location < 0) {
            throw "Missing uniform '" + attribute + "' in pipeline.";
        }
        this.uniformLocs[uniform] = location;
    }
    this.destroy = function() {
        gl.deleteProgram(this.program);
    }
}

const UNIFORM_VF1 = 1;
const UNIFORM_VF2 = 2;
const UNIFORM_VF3 = 3;
const UNIFORM_VF4 = 4;
const UNIFORM_VI1 = 5;
const UNIFORM_VI2 = 6;
const UNIFORM_VI3 = 7;
const UNIFORM_VI4 = 8;
const UNIFORM_MF2 = 9;
const UNIFORM_MF3 = 10;
const UNIFORM_MF4 = 11;
function UniformDef(type, offset) {
    this.type = type;
    this.offset = offset;
}
function UniformBinding(pipeline, size, uniformDefs) {
    this.size = size;
    this.slots = [];
    for (const uniform in uniformDefs) {
        const def = uniformDefs[uniform];
        const location = pipeline.uniformLocs[uniform];
        this.slots.push([def.type, def.offset, location]);
    }
    this.destroy = function() {}
}

function AttribDef(size, type, normalize,
                   stride, offset) {
    this.size = size;
    this.type = type;
    this.normalize = normalize;
    this.stride = stride;
    this.offset = offset;
}
function Mesh(gl, primitiveType, attribDefs) {
    this.primitiveType = primitiveType;
    this.attribDefs = attribDefs;
    this.count = 0;
    this.vb = gl.createBuffer();
    this.ib = gl.createBuffer();
    const ext = gl.getExtension('OES_vertex_array_object');
    this.set = function(vertexData, indexData) {
        this.count = indexData.length;
        ext.bindVertexArrayOES(null);
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vb);
        gl.bufferData(gl.ARRAY_BUFFER, vertexData, gl.STATIC_DRAW);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ib);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indexData, gl.STATIC_DRAW);
    }
    this.destroy = function() {
        gl.deleteBuffer(this.vb);
        gl.deleteBuffer(this.ib);
    }
}

function MeshBinding(gl, mesh, pipeline) {
    this.mesh = mesh;
    this.pipeline = pipeline;
    if (gl.createVertexArray !== undefined) {
        // WebGL 2
        this.vao = gl.createVertexArray();
        gl.bindVertexArray(this.vao);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, mesh.ib);
        gl.bindBuffer(gl.ARRAY_BUFFER, mesh.vb);
        for (const attribute in pipeline.attribLocs) {
            const location = pipeline.attribLocs[attribute];
            const def = mesh.attribDefs[attribute];
            if (!def) {
                throw "Missing attribute '" + attribute + "' in mesh.";
            }
            gl.enableVertexAttribArray(location);
            gl.vertexAttribPointer(location, def.size,
                                   def.type, def.normalize,
                                   def.stride, def.offset);
        }
        this.use = function() {
            gl.bindVertexArray(this.vao);
        }
        this.destroy = function() {
            gl.deleteVertexArray(this.vao);
        }
    } else {
        // WebGL 1 fallback
        const ext = gl.getExtension('OES_vertex_array_object');
        this.vao = ext.createVertexArrayOES();
        ext.bindVertexArrayOES(this.vao);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, mesh.ib);
        gl.bindBuffer(gl.ARRAY_BUFFER, mesh.vb);
        for (const attribute in pipeline.attribLocs) {
            const location = pipeline.attribLocs[attribute];
            const def = mesh.attribDefs[attribute];
            if (!def) {
                throw "Missing attribute '" + attribute + "' in mesh.";
            }
            gl.enableVertexAttribArray(location);
            gl.vertexAttribPointer(location, def.size,
                                   def.type, def.normalize,
                                   def.stride, def.offset);
        }
        this.use = function() {
            ext.bindVertexArrayOES(this.vao);
        }
        this.destroy = function() {
            ext.deleteVertexArrayOES(this.vao);
        }
    }
}

function applyUniforms(gl, binding, view) {
    for (const i in binding.slots) {
        const slot = binding.slots[i];
        const type = slot[0];
        const offset = slot[1];
        const location = slot[2];
        switch (type) {
        case UNIFORM_VF1:
            gl.uniform1fv(location, new Float32Array(
                view.buffer, view.byteOffset + offset, 1));
            break;
        case UNIFORM_VF2:
            gl.uniform2fv(location, new Float32Array(
                view.buffer, view.byteOffset + offset, 2));
            break;
        case UNIFORM_VF3:
            gl.uniform3fv(location, new Float32Array(
                view.buffer, view.byteOffset + offset, 3));
            break;
        case UNIFORM_VF4:
            gl.uniform4fv(location, new Float32Array(
                view.buffer, view.byteOffset + offset, 4));
            break;
        case UNIFORM_VI1:
            gl.uniform1iv(location, new Int32Array(
                view.buffer, view.byteOffset + offset, 1));
            break;
        case UNIFORM_VI2:
            gl.uniform2iv(location, new Int32Array(
                view.buffer, view.byteOffset + offset, 2));
            break;
        case UNIFORM_VI3:
            gl.uniform3iv(location, new Int32Array(
                view.buffer, view.byteOffset + offset, 3));
            break;
        case UNIFORM_VI4:
            gl.uniform4iv(location, new Int32Array(
                view.buffer, view.byteOffset + offset, 4));
            break;
        case UNIFORM_MF2:
            gl.uniformMatrix2fv(location, false, new Float32Array(
                view.buffer, view.byteOffset + offset, 4));
            break;
        case UNIFORM_MF3:
            gl.uniformMatrix3fv(location, false, new Float32Array(
                view.buffer, view.byteOffset + offset, 9));
            break;
        case UNIFORM_MF4:
            gl.uniformMatrix4fv(location, false, new Float32Array(
                view.buffer, view.byteOffset + offset, 16));
            break;
        }
    }
}

const COMMAND_END = 0;
const COMMAND_SET_VIEWPORT = 1;
const COMMAND_SET_CLEAR_COLOR = 2;
const COMMAND_CLEAR = 3;
const COMMAND_SET_PIPELINE = 4;
const COMMAND_SET_UNIFORMS = 5;
const COMMAND_SET_MESH = 6;
const COMMAND_DRAW = 7;
function executeCommands(gl, refHeap, view) {
    let mesh = null;
    let ptr = 0;
    let done = false;
    while (!done) {
        const cmd = view.getUint32(ptr, true); ptr += 4;
        switch (cmd) {
        case COMMAND_END: {
            done = true;
        } break;
        case COMMAND_SET_VIEWPORT: {
            //console.log('COMMAND_SET_VIEWPORT');
            const x = view.getInt32(ptr, true); ptr += 4;
            const y = view.getInt32(ptr, true); ptr += 4;
            const width = view.getUint32(ptr, true); ptr += 4;
            const height = view.getUint32(ptr, true); ptr += 4;
            gl.viewport(x, y, width, height);
        } break;
        case COMMAND_SET_CLEAR_COLOR: {
            //console.log('COMMAND_SET_CLEAR_COLOR');
            const r = view.getFloat32(ptr, true); ptr += 4;
            const g = view.getFloat32(ptr, true); ptr += 4;
            const b = view.getFloat32(ptr, true); ptr += 4;
            const a = view.getFloat32(ptr, true); ptr += 4;
            gl.clearColor(r, g, b, a);
        } break;
        case COMMAND_CLEAR: {
            //console.log('COMMAND_CLEAR');
            const flags = view.getUint32(ptr, true); ptr += 4;
            let mask = 0;
            mask = mask | (flags & 1 === 1 ? gl.COLOR_BUFFER_BIT : 0);
            mask = mask | (flags & 2 === 2 ? gl.DEPTH_BUFFER_BIT : 0);
            mask = mask | (flags & 4 === 4 ? gl.STENCIL_BUFFER_BIT : 0);
            gl.clear(mask);
        } break;
        case COMMAND_SET_PIPELINE: {
            //console.log('COMMAND_SET_PIPELINE');
            const p = view.getUint32(ptr, true); ptr += 4;
            const pipeline = refHeap.get(p);
            gl.useProgram(pipeline.program);
        } break;
        case COMMAND_SET_UNIFORMS: {
            //console.log('COMMAND_SET_UNIFORMS');
            const b = view.getUint32(ptr, true); ptr += 4;
            const binding = refHeap.get(b);
            applyUniforms(gl, binding, new DataView(view.buffer, view.byteOffset + ptr, binding.size));
            ptr += binding.size;
        } break;
        case COMMAND_SET_MESH: {
            //console.log('COMMAND_SET_MESH');
            const b = view.getUint32(ptr, true); ptr += 4;
            const binding = refHeap.get(b);
            binding.use();
            mesh = binding.mesh;
        } break;
        case COMMAND_DRAW: {
            //console.log('COMMAND_DRAW');
            gl.drawElements(mesh.primitiveType, mesh.count, gl.UNSIGNED_INT, 0);
        } break;
        default:
            throw "Unknown command."
        }
    }
}

function assert(condition, message) {
    if (!condition) {
        throw message || "Assertion failed";
    }
}

function RefHeap(size) {
    this.slots = new Array(size);
    this.firstFreeSlot = 0;
    this.put = function(obj) {
        assert(this.firstFreeSlot < this.slots.length, "RefHeap: Out of slots!");
        const slot = this.firstFreeSlot;
        this.slots[slot] = obj;
        this.firstFreeSlot++;
        while(this.firstFreeSlot < this.slots.length &&
              this.slots[this.firstFreeSlot] !== undefined) {
            this.firstFreeSlot++;
        }
        return slot;
    }
    this.del = function(id) {
        assert(id >= 0 && id < this.slots.length, "RefHeap: Invalid id");
        assert(this.slots[id] !== undefined, "RefHeap: Invalid del!");
        this.slots[id] = undefined;
        if (id < this.firstFreeSlot) {
            this.firstFreeSlot = id;
        }
    }
    this.get = function(id) {
        assert(id >= 0 && id < this.slots.length, "RefHeap: Invalid id");
        assert(this.slots[id] !== undefined, "RefHeap: Invalid get!");
        return this.slots[id];
    }
}

// Touch events are a bit more complicated, as they track multiple fingers and there is no movementX, movementY
/*canvas.addEventListener('touchmove', function(e) {
//console.log('touchmove', e);
const touch = e.touches[0];
clientX = touch.clientX;
clientY = touch.clientY;
});*/

/*canvas.requestPointerLock = canvas.requestPointerLock;
  canvas.requestPointerLock();*/

function MouseInputObserver(view) {
    this.view = view;
}
function MouseInputTracer(canvas, ) {
    let movementX = 0;
    let movementY = 0;
    let clientX = 0;
    let clientY = 0;
    let mousedownMain = 0;
    let pressedMain = 0;
    let mousedownSecond = 0;
    let pressedSecond = 0;
    const mousemoveFn = function(e) {
        //console.log('mousemove', e);
        movementX += e.movementX;
        movementY += e.movementY;
        clientX = e.clientX;
        clientY = e.clientY;
    };
    const mousedownFn = function(e) {
        //console.log('mousedown', e);
        canvas.requestPointerLock = canvas.requestPointerLock;
        canvas.requestPointerLock();
        if (e.button == 0) {
            mousedownMain++;
            pressedMain = 1;
        }
        if (e.button == 2) {
            mousedownSecond++;
            pressedSecond = 1;
        }
    };
    const mouseupFn = function(e) {
        //console.log('mousedown', e);
        if (e.button == 0) {
            pressedMain = 0;
        }
        if (e.button == 2) {
            pressedSecond = 0;
        }
    };
    let observers = [];
    this.addObserver = function(observer) {
        if (observers.length === 0) {
            console.log('First MouseInputObserver added');
            canvas.addEventListener('mousemove', mousemoveFn);
            canvas.addEventListener('mousedown', mousedownFn);
            canvas.addEventListener('mouseup', mouseupFn);
        }
        observers.push(observer);
    }
    this.removeObserver = function(observer) {
        const idx = observers.indexOf(observer);
        if (idx > -1) {
            observers.splice(idx, 1);
        } else {
            throw 'Trying to remove an unknown observer.'
        }
        if (observers.length == 0) {
            console.log('Last MouseInputObserver removed');
            canvas.removeEventListener('mousemove', mousemoveFn);
            canvas.removeEventListener('mousedown', mousedownFn);
            canvas.removeEventListener('mouseup', mouseupFn);
        }
    }
    this.swap = function() {
        for (const i in observers) {
            const observer = observers[i];
            const view = observer.view;
            view.setInt32(0, movementX, true);
            view.setInt32(4, movementY, true);
            view.setInt32(8, clientX, true);
            view.setInt32(12, clientY, true);
            view.setInt32(16, mousedownMain, true);
            view.setInt32(20, pressedMain, true);
            view.setInt32(24, mousedownSecond, true);
            view.setInt32(28, pressedSecond, true);
        }
        movementX = 0;
        movementY = 0;
        mousedownMain = 0;
        mousedownSecond = 0;
    };
}

function KeyboardInputObserver(view, code) {
    this.code = code;
    this.view = view;
}
function KeyboardInputTracer() {
    let tracers = {};
    const keydownFn = function(e) {
        //console.log('keydown', e);
        const tracer = tracers[e.code];
        if (tracer !== undefined) {
            tracer.keydown++;
            tracer.pressed = 1;
        }
    };
    const keyupFn = function(e) {
        //console.log('keyup', e);
        const tracer = tracers[e.code];
        if (tracer !== undefined) {
            tracer.keyup++;
            tracer.pressed = 0;
        }
    };
    this.addObserver = function(observer) {
        const code = observer.code;
        console.log(tracers.length);
        if (Object.keys(tracers).length === 0) {
            console.log('First KeyboardInputObserver added');
            document.addEventListener('keydown', keydownFn);
            document.addEventListener('keyup', keyupFn);
        }
        if (tracers[code] === undefined) {
            tracers[code] = { keydown: 0, pressed: 0, observers: []};
        }
        tracers[code].observers.push(observer);
    }
    this.removeObserver = function(observer) {
        const code = observer.code;
        if (tracers[code] === undefined) {
            throw 'Trying to remove an unknown observer.'
        }
        const idx = tracers[code].observers.indexOf(observer);
        if (idx > -1) {
            tracers[code].observers.splice(idx, 1);
        } else {
            throw 'Trying to remove an unknown observer.'
        }
        if (tracers[code].observers.length === 0) {
            delete tracers[code];
        }
        if (Object.keys(tracers).length === 0) {
            console.log('Last KeyboardInputObserver removed');
            document.removeEventListener('keydown', keydownFn);
            document.removeEventListener('keyup', keyupFn);
        }
    }
    this.swap = function() {
        for (const code in tracers) {
            const tracer = tracers[code];
            const keydown = tracer.keydown;
            const pressed = tracer.pressed;
            const observers = tracer.observers;
            for (const i in observers) {
                const observer = observers[i];
                const view = observer.view;
                view.setInt32(0, keydown, true);
                view.setInt32(4, pressed, true);
            }
            tracer.keydown = 0;
        }
    };
}

document.addEventListener("DOMContentLoaded", function(event) {
    const canvas = document.body.appendChild(document.createElement('canvas'));
    gl = canvas.getContext('webgl2', {antialias: false, powerPreference: 'high-performance'});
    if (!gl) {
        gl = canvas.getContext('webgl');
        if (!gl) {
            throw 'WebGL is not supported by your browser :(';
        }

        OES_element_index_uint = gl.getExtension('OES_element_index_uint');
        if (!OES_element_index_uint) {
            throw 'The WebGL extension OES_element_index_uint is not supported by your browser :(';
        }
        OES_vertex_array_object = gl.getExtension('OES_vertex_array_object');
        if (!OES_vertex_array_object) {
            throw 'The WebGL extension OES_vertex_array_object is not supported by your browser :(';
        }
    }
    const mouseInputTracer = new MouseInputTracer(canvas);
    const keyboardInputTracer = new KeyboardInputTracer();
    let memory = new WebAssembly.Memory({ initial: 8 });
    let heap = new Uint8Array(memory.buffer);
    let refHeap = new RefHeap(32);
    let wasm = null;
    
    let wasm_read_c_str = function(ptr) {
        let len = 0;
        while (heap[ptr + len] !== 0) {
            len += 1;
        }
        return new TextDecoder("utf-8")
            .decode(new Uint8Array(heap.buffer, ptr, len));
    };
    let wasm_read_buffer = function(ptr, len) {
        return new Uint8Array(heap.buffer, ptr, len);
    };
    let wasm_put_buffer = function(data) {
        const js_array = new Uint8Array(data);
        const length = js_array.length;
        const wasm_ptr = wasm.exports.allocate_buffer(length);
        const wasm_array = new Uint8Array(heap.buffer, wasm_ptr, length);
        wasm_array.set(js_array);
        return { ptr: wasm_ptr, length: length };
    }
    let wasm_del_buffer = function(buffer) {
        wasm.exports.free_buffer(buffer.ptr);
    }

    const input_module = {
        create_mouse_observer: function(buffer_ptr) {
            const view = new DataView(heap.buffer, buffer_ptr);
            const observer = new MouseInputObserver(view);
            console.log('create_mouse_observer');
            mouseInputTracer.addObserver(observer);
            return refHeap.put(observer);
        },
        destroy_mouse_observer: function(id) {
            const observer = refHeap.get(id);
            mouseInputTracer.removeObserver(observer);
            refHeap.del(id);
        },
        create_key_observer: function(buffer_ptr, code_ptr) {
            const view = new DataView(heap.buffer, buffer_ptr);
            const code = wasm_read_c_str(code_ptr);
            console.log('create_key_observer', code);
            const observer = new KeyboardInputObserver(view, code);
            keyboardInputTracer.addObserver(observer);
            return refHeap.put(observer);
        },
        destroy_key_observer: function(id) {
            const observer = refHeap.get(id);
            keyboardInputTracer.removeObserver(observer);
            refHeap.del(id);
        }
    };
    const wasi_module = {
        environ_get: function(environ, environ_buf) {
            console.log("environ_get");
            return 0;
        },
        environ_sizes_get: function(argc, argv_buf_size) {
            console.log("environ_sizes_get");
            return 0;
        },
        fd_close: function(fd) {
            console.log("fd_close",fd);
            return 0;
        },
        fd_fdstat_get: function(fd, stat) {
            console.log("fd_fdstat_get",fd);
            return 0;
        },
        fd_fdstat_set_flags: function(fd, flags) {
            console.log("fd_fdstat_set_flags",fd,flags);
            return 0;
        },
        fd_read: function(fd, iovs_ptr, iovs_len, nread) {
            console.log("fd_read",fd, iovs_ptr, iovs_len);
            return 0;
        },
        fd_renumber: function(fd, to) {
            console.log("fd_renumber",fd,to);
            return 0;
        },
        fd_seek: function(fd, offset, whence, newoffset) {
            console.log("fd_seek",fd,offset,whence);
            return 0;
        },
        fd_write: function(fd, iovs_ptr, iovs_len, nwritten) {
            //console.log("fd_write",fd,iovs_ptr.toString(16),iovs_len);
            var bytes = 0;
            var view = new DataView(heap.buffer, 0);
            for (var i = 0; i < iovs_len; i++) {
                const buf = view.getUint32(iovs_ptr + 8 * i, true);
                const buf_len = view.getUint32(iovs_ptr + 8 * i + 4, true);
                //console.log(buf, buf_len);
                var msg = new Uint8Array(heap.buffer, buf, buf_len);
                //console.log(msg);
                var string = new TextDecoder("utf-8").decode(msg);
                console.log(string);
                bytes += buf_len;
            }
            view.setUint32(nwritten, bytes, true);
            return 0;
        },
        path_open: function(fd, dirflags,
                            path_ptr, path_len,
                            oflags, fs_rights_base,
                            fs_rights_inherting,
                            fdflags, opened_fd) {
            console.log("path_open",fd, dirflags,
                        path_ptr, path_len,
                        oflags, fs_rights_base,
                        fs_rights_inherting,
                        fdflags);
        },
        proc_exit: function(rval) {
            console.log("proc_exit",rval);
        }
    };
    const websocket_module = {
        open: function(ptr, context) {
            let route = wasm_read_c_str(ptr);
            let socket = new WebSocket("wss://" + location.host + route);
            socket.binaryType = 'arraybuffer';
            let id = refHeap.put(socket);
            socket.onopen = function(event) {
                wasm.exports.websocket_onopen(context);
            };
            socket.onclose = function(event) {
                wasm.exports.websocket_onclose(context);
            };
            socket.onmessage = function(event) {
                let buffer = wasm_put_buffer(event.data);
                wasm.exports.websocket_onmessage(context, buffer.ptr, buffer.length);
                wasm_del_buffer(buffer);
            };
            socket.onerror = function(event) {
                let buffer = wasm_put_buffer(event.data);
                wasm.exports.websocket_onerror(context, buffer.ptr, buffer.length);
                wasm_del_buffer(buffer);
            };
            return id;
        },
        close: function(id) {
            let socket = refHeap.get(id);
            socket.close();
            refHeap.del(id);
        },
        send: function(id, buffer, size) {
            let socket = refHeap.get(id);
            socket.send(wasm_read_buffer(buffer, size));
        }
    };
    const gl_module = {
        create_pipeline: function(ctx, vs, fs, attv, attc, univ, unic) {
            console.log('create_pipeline');
            const gl = refHeap.get(ctx);
            const attributes = [];
            const uniforms = [];
            const view = new DataView(heap.buffer);
            for (let i = 0; i < attc; ++i) {
                const ptr = view.getUint32(attv + 4 * i, true);
                attributes.push(wasm_read_c_str(ptr));
            }
            for (let i = 0; i < unic; ++i) {
                const ptr = view.getUint32(univ + 4 * i, true);
                uniforms.push(wasm_read_c_str(ptr));
            }
            const pipeline = new Pipeline(
                gl, attributes, uniforms, wasm_read_c_str(vs), wasm_read_c_str(fs));
            console.log(pipeline);
            return refHeap.put(pipeline);
        },
        destroy_pipeline: function(p) {
            console.log('destroy_pipeline');
            const pipeline = refHeap.get(p);
            pipeline.destroy();
            refHeap.del(p);
        },
        create_mesh: function(ctx, pt, defv, defc) {
            console.log('create_mesh');
            const gl = refHeap.get(ctx);
            const primitiveType = pt == 1 ? gl.TRIANGLES : gl.LINES;
            const view = new DataView(heap.buffer);
            let attribDefs = {}
            for (let i = 0; i < defc; ++i) {
                const ptr = defv + i * 24;
                const name = wasm_read_c_str(view.getUint32(ptr, true));
                const stride = view.getUint32(ptr + 4, true);
                const offset = view.getUint32(ptr + 8, true);
                const size = view.getUint32(ptr + 12, true);
                const type = view.getUint32(ptr + 16, true) == 1 ? gl.FLOAT : gl.UNSIGNED_BYTE;
                const normalize = view.getUint32(ptr + 20, true);
                attribDefs[name] = new AttribDef(size, type, normalize, stride, offset);
            }
            const mesh = new Mesh(gl, primitiveType, attribDefs);
            console.log(mesh);
            return refHeap.put(mesh);
        },
        set_mesh: function(m, vd, vds, id, ids) {
            //console.log('set_mesh');
            const mesh = refHeap.get(m);
            const vertexData = new DataView(heap.buffer, vd, vds);
            const indexData = new Uint32Array(heap.buffer, id, ids);
            mesh.set(vertexData, indexData);
        },
        destroy_mesh: function(m) {
            console.log('destroy_mesh');
            const mesh = refHeap.get(m);
            mesh.destroy();
            refHeap.del(m);
        },
        create_mesh_binding: function(ctx, m, p) {
            console.log('create_mesh_binding');
            const gl = refHeap.get(ctx);
            const mesh = refHeap.get(m);
            const pipeline = refHeap.get(p);
            const binding = new MeshBinding(gl, mesh, pipeline);
            console.log(binding);
            return refHeap.put(binding);
        },
        destroy_mesh_binding: function(b) {
            console.log('destroy_mesh_binding');
            const binding = refHeap.get(b);
            binding.destroy();
            refHeap.del(b);
        },
        create_uniform_binding: function(ctx, p, size, defv, defc) {
            console.log('create_uniform_binding');
            //const gl = refHeap.get(ctx);
            const pipeline = refHeap.get(p);
            const view = new DataView(heap.buffer);
            let uniformDefs = {}
            for (let i = 0; i < defc; ++i) {
                const ptr = defv + i * 12;
                const name = wasm_read_c_str(view.getUint32(ptr, true));
                const type = view.getUint32(ptr + 4, true);
                const offset = view.getUint32(ptr + 8, true);
                uniformDefs[name] = new UniformDef(type, offset);
            }
            const binding = new UniformBinding(pipeline, size, uniformDefs);
            console.log(binding);
            return refHeap.put(binding);
        },
        destroy_uniform_binding: function(b) {
            console.log('destroy_uniform_binding');
            const binding = refHeap.get(b);
            binding.destroy();
            refHeap.del(b);
        },
        execute_command_buffer: function(ctx, buffer, size) {
            const gl = refHeap.get(ctx);
            const view = new DataView(heap.buffer, buffer, size);
            executeCommands(gl, refHeap, view);
        }
    };
    const importObject = {
        env: { memory },
        websocket: websocket_module,
        input: input_module,
        gl: gl_module,
        wasi_snapshot_preview1: wasi_module
    };
    console.log(importObject);
    fetch('game.wasm')
        .then(response => response.arrayBuffer())
        .then(bytes => WebAssembly.instantiate(bytes, importObject))
        .then(results => {
            wasm = results.instance;
            gl.enable(gl.DEPTH_TEST);
            const ctx = refHeap.put(gl);
            const udata = wasm.exports.init(ctx);
            let prevTime;
            let anim = {
                update: function(now) {
                    var width = gl.canvas.clientWidth/* * devicePixelRatio*/;
                    var height = gl.canvas.clientHeight/* * devicePixelRatio*/;
                    if (gl.canvas.width != width ||
                        gl.canvas.height != height) {
                        gl.canvas.width = width;
                        gl.canvas.height = height;
                    }
                    mouseInputTracer.swap();
                    keyboardInputTracer.swap();
                    const dt = prevTime !== undefined ? (now - prevTime) / 1000.0 : 0.0;
                    prevTime = now;
                    const result = wasm.exports.render(udata, dt,
                                                       gl.drawingBufferWidth,
                                                       gl.drawingBufferHeight);
                    if (result === 0) {
                        requestAnimationFrame(this.update);
                    } else {
                        wasm.exports.release(udata);
                        refHeap.del(ctx);
                    }
                }
            }
            anim.update = anim.update.bind(anim);
            requestAnimationFrame(anim.update);
        });
});
