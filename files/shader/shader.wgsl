struct MyUniforms {
    viewprojectionmodelMatrix: mat4x4f,
    color: vec4f,
    cameraPos: vec3<f32>,
    time: f32,
};

struct vertexdata {
    face: u32,
    chunkx: i32,
    chunky: i32,
    chunkz: i32,
    widthheight: u32,
    normal: u32,
    color: u32,
};

struct VertexInput {
    @location(0) face: u32,
    @location(1) chunkpos: vec3<i32>,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2<f32>,
};

struct CalculationResult {
    pos: vec3f,
    normal: vec3f,
};

const normals = array<vec3f, 6>(
    vec3f(-1.0, 0.0, 0.0), // dir 0 (Left / -X)
    vec3f(1.0, 0.0, 0.0),  // dir 1 (Right / +X)
    vec3f(0.0, -1.0, 0.0), // dir 2 (Bottom / -Y)
    vec3f(0.0, 1.0, 0.0),  // dir 3 (Top / +Y)
    vec3f(0.0, 0.0, -1.0), // dir 4 (Back / -Z)
    vec3f(0.0, 0.0, 1.0),  // dir 5 (Front / +Z)
);

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
@group(0) @binding(1) var<storage> vertexbuffer: array<vertexdata>;

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var position: vec3f;

    var dta = vertexbuffer[instance_index];

    let xx = f32(dta.chunkx);
    let yy = f32(dta.chunky);
    let zz = f32(dta.chunkz);
    
    position = vec3<f32>(xx,yy,zz);

    let result = calculate_pos(dta, vertex_index, position);
    let pos = result.pos;

    let nx = plusminus(dta.normal);
    let ny = plusminus(dta.normal >> 10u);
    let nz = plusminus(dta.normal >> 20u);
   // let normal = (normaldta + result.normal) / vec3f(2);
    
    let normal = vec3f(nx,ny,nz);

    var out: VertexOutput;
    out.position = uMyUniforms.viewprojectionmodelMatrix * vec4f(pos, 1.0);

    let colorx = f32(dta.color & 255u) / 255.0;
    let colory = f32((dta.color >> 8u) & 255u) / 255.0;
    let colorz = f32((dta.color >> 16u) & 255u) / 255.0;

    out.color = vec4f(colorx, colory, colorz, 1.0);
  
    //out.normal = (uMyUniforms.modelMatrix * vec4f(normal, 0.0)).xyz;
    out.normal = vec4f(normal, 0.0).xyz;

   if (vertex_index == 0u) {
    out.uv = vec2f(0.0, 0.0);
} else if (vertex_index == 1u) {
    out.uv = vec2f(0.0, 2.0);
} else {
    out.uv = vec2f(2.0, 0.0);
}

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let normal = normalize(in.normal);
    let lightDirection = normalize(vec3f(0.5, 0.9, 0.1));
    let shading = max(dot(lightDirection, normal), 0.0);
    let color = vec4f(in.color.rgb * clamp(shading, 0.3, 1.0), in.color.a);

    return color;
}

fn calculate_pos(params: vertexdata, vertex_index: u32, position: vec3f) -> CalculationResult {
    let quad_index = vertex_index;

   
    var width = f32(params.widthheight & 63u);
    var height = f32((params.widthheight >> 6u) & 63u) ;
    let lod = f32(params.widthheight >> 12u);

    let l = lod;
    width = (width * l);
    height = (height * l);//versuch die chunks passend zum lod zu scalen
    let size: f32 = l;

     let packedPos = params.face;

    let x = (f32(packedPos & 63u)) * l; 
    let y = (f32((packedPos >> 6u) & 63u) ) * l;
    let z = (f32((packedPos >> 12u) & 63u) ) * l;
    let face_index = u32((packedPos >> 18u) & 7u);
    
    var pos = vec3f(0.0);
    let normal = normals[face_index];

  let quads = array<vec3f, 24>(
        // dir 0 (-X / Left)
        vec3f(0.0, 0.0, 0.0), vec3f(0.0, 0.0, width), vec3f(0.0, height, 0.0), vec3f(0.0, height, width),
        // dir 1 (+X / Right) - Um "size" auf der X-Achse nach rechts geschoben
        vec3f(size, 0.0, width), vec3f(size, 0.0, 0.0), vec3f(size, height, width), vec3f(size, height, 0.0),
        
        // dir 2 (-Y / Bottom)
        vec3f(0.0, 0.0, 0.0), vec3f(width, 0.0, 0.0), vec3f(0.0, 0.0, height), vec3f(width, 0.0, height),
        // dir 3 (+Y / Top) - Um "size" auf der Y-Achse nach oben geschoben
        vec3f(0.0, size, height), vec3f(width, size, height), vec3f(0.0, size, 0.0), vec3f(width, size, 0.0),
        
        // dir 4 (-Z / Back)
        vec3f(width, 0.0, 0.0), vec3f(0.0, 0.0, 0.0), vec3f(width, height, 0.0), vec3f(0.0, height, 0.0),
        // dir 5 (+Z / Front) - Um "size" auf der Z-Achse nach vorne geschoben
        vec3f(0.0, 0.0, size), vec3f(width, 0.0, size), vec3f(0.0, height, size), vec3f(width, height, size)
    );

    var quadpos = quads[face_index * 4u + quad_index];

    pos.x = quadpos.x + x + (position.x * 62);
    pos.y = quadpos.y + y + (position.y * 62);
    pos.z = quadpos.z + z + (position.z * 62);

    return CalculationResult(pos, normal);
}

fn plusminus(val: u32) -> f32 {
    let v = val & 0x3FFu;
    if (v >= 512u) {
        return f32(v) - 1024.0;
    }
    return f32(v);
}