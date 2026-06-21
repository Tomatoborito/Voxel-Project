struct MyUniforms {
    viewprojectionmodelMatrix: mat4x4<f32>,
    color: vec4<f32>,
    cameraPos: vec3<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> uniforms: MyUniforms;

struct ShaderData {
    face: i32,
    chunkx: i32,
    chunky: i32,
    chunkz: i32,
    widthheight: i32,
};
@group(0) @binding(1) var<storage, read_write> vertexData: array<ShaderData>;

struct DrawIndexedIndirect {
    vertexCount: u32,
    instanceCount: u32,
    baseIndex: u32,
    baseVertex: i32,
    baseInstance: u32,
};

struct pos {
    x: i32,
    y: i32,
    z: i32, 
    lod: i32,
    direction: i32,
};


    const directions = array<vec3<f32>, 6>(
    vec3<f32>(1.0, 0.0, 0.0),  // 0: x+
    vec3<f32>(-1.0, 0.0, 0.0), // 1: x-
    vec3<f32>(0.0, 1.0, 0.0),  // 2: y+
    vec3<f32>(0.0, -1.0, 0.0), // 3: y-
    vec3<f32>(0.0, 0.0, 1.0),  // 4: z+
    vec3<f32>(0.0, 0.0, -1.0)  // 5: z-
);

@group(0) @binding(2) var<storage, read_write> indirectBuffer: array<DrawIndexedIndirect>;
@group(0) @binding(3) var<storage, read_write> drawCount: atomic<u32>;
@group(0) @binding(4) var<storage, read_write> positionBuffer: array<pos>;

@compute @workgroup_size(64)
fn cs_main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    
    let index = global_id.x;

    if (index >= arrayLength(&indirectBuffer)) {
        return;
    }

    let chunk_size = 64.0 * f32(positionBuffer[index].lod);
    let pos_min = vec3<f32>(f32(62 * positionBuffer[index].x), 
                            f32(62 * positionBuffer[index].y), 
                            f32(62 * positionBuffer[index].z));
    let pos_max = pos_min + vec3<f32>(chunk_size);

    let vp = uniforms.viewprojectionmodelMatrix;
    var is_visible = true;

    // 2. Wir prüfen die 6 Ebenen des Frustums
    // Eine AABB ist außerhalb, wenn für eine Ebene ALLE Punkte der Box draußen sind.
    // Der Trick: Wir nehmen die Spalten der Matrix als Ebenen-Definitionen.
    
    // Wir extrahieren die Ebenen aus der VP-Matrix (Reihen-basiert für WebGPU/DirectX Style)
    let row4 = vec4<f32>(vp[0].w, vp[1].w, vp[2].w, vp[3].w); // W-Reihe
    let row1 = vec4<f32>(vp[0].x, vp[1].x, vp[2].x, vp[3].x); // X-Reihe
    let row2 = vec4<f32>(vp[0].y, vp[1].y, vp[2].y, vp[3].y); // Y-Reihe
    let row3 = vec4<f32>(vp[0].z, vp[1].z, vp[2].z, vp[3].z); // Z-Reihe

    // Erstelle die 6 Frustum-Ebenen (Normalen n + Distanz d)
    // Format: Plane = n.x, n.y, n.z, d
    var planes: array<vec4<f32>, 6>;
    planes[0] = row4 + row1; // Links
    planes[1] = row4 - row1; // Rechts
    planes[2] = row4 + row2; // Unten
    planes[3] = row4 - row2; // Oben
    planes[4] = row3;        // Near (WebGPU: 0 <= z <= w)
    planes[5] = row4 - row3; // Far

    for (var i = 0u; i < 6u; i = i + 1u) {
        let p = planes[i];
        
        // Finde den "p-vertex": Die Ecke der Box, die am weitesten 
        // in Richtung der Ebenennormale liegt.
        let target_pos = vec3<f32>(
            select(pos_min.x, pos_max.x, p.x >= 0.0),
            select(pos_min.y, pos_max.y, p.y >= 0.0),
            select(pos_min.z, pos_max.z, p.z >= 0.0)
        );

        // Wenn der "am weitesten innen liegende" Punkt draußen ist, 
        // ist die ganze Box draußen.
        if (dot(vec4<f32>(target_pos, 1.0), p) < 0.0) {
            is_visible = false;
            break;
        }
    }



        let cam = uniforms.cameraPos;
        let direction = u32(positionBuffer[index].direction);
        var is_backface = false;

        if (direction == 0u) { // X+ 
            is_backface = cam.x <= pos_min.x; 
        } else if (direction == 1u) { // X- 
            is_backface = cam.x >= pos_max.x;
        } else if (direction == 2u) { // Y+
            is_backface = cam.y <= pos_min.y;
        } else if (direction == 3u) { // Y-
            is_backface = cam.y >= pos_max.y;
        } else if (direction == 4u) { // Z+
            is_backface = cam.z <= pos_min.z;
        } else if (direction == 5u) { // Z-
            is_backface = cam.z >= pos_max.z;
        }

        if (is_backface) {
            is_visible = false;
        }


    

    if (is_visible) {
        indirectBuffer[index].vertexCount = 4;
        
    } else {
        indirectBuffer[index].vertexCount = 0;
    }
}