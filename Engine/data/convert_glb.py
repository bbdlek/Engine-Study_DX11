import numpy as np
from pygltflib import GLTF2

def parse_glb_file(filename):
    gltf = GLTF2().load(filename)
    vertices = []
    tex_coords = []
    normals = []
    indices = []

    # Extract the mesh data
    for mesh in gltf.meshes:
        for primitive in mesh.primitives:
            attributes = primitive.attributes

            # Access vertex data
            position_accessor = gltf.accessors[attributes.POSITION]
            positions = gltf.get_accessor_data(position_accessor)
            vertices.extend(positions[i:i+3] for i in range(0, len(positions), 3))
                

            normal_accessor = gltf.accessors[attributes.NORMAL]
            normals_data = gltf.get_accessor_data(normal_accessor)
            normals.extend(normals_data[i:i+3] for i in range(0, len(normals_data), 3))

            texcoord_accessor = gltf.accessors[attributes.TEXCOORD_0]
            tex_coords_data = gltf.get_accessor_data(texcoord_accessor)
            tex_coords.extend(tex_coords_data[i:i+2] for i in range(0, len(tex_coords_data), 2))

            # Extract indices
            if primitive.indices is not None:
                index_accessor = gltf.accessors[primitive.indices]
                indices_data = gltf.get_accessor_data(index_accessor)
                indices.extend(indices_data)

    return vertices, tex_coords, normals, indices

def convert_to_custom_format(vertices, tex_coords, normals, indices):
    vertex_data = []

    # Convert indices to integers
    indices = [int(idx) for idx in indices]

    for i in range(0, len(indices), 3):
        for j in range(3):
            idx = indices[i + j]
            x, y, z = vertices[idx] if idx < len(vertices) else (0.0, 0.0, 0.0)
            tu, tv = tex_coords[idx] if idx < len(tex_coords) else (0.0, 0.0)
            nx, ny, nz = normals[idx] if idx < len(normals) else (0.0, 0.0, 0.0)
            vertex_data.append([x, y, z, tu, tv, nx, ny, nz])

    return vertex_data

def write_to_file(filename, vertex_data):
    with open(filename, 'w') as file:
        file.write(f"Vertex Count: {len(vertex_data)}\n\n")
        file.write("Data:\n\n")
        for vertex in vertex_data:
            file.write(f"{' '.join(map(str, vertex))}\n")

def main(glb_filename, output_filename):
    vertices, tex_coords, normals, indices = parse_glb_file(glb_filename)
    vertex_data = convert_to_custom_format(vertices, tex_coords, normals, indices)
    write_to_file(output_filename, vertex_data)

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 3:
        print("Usage: python convert_glb.py input.glb output.txt")
    else:
        glb_filename = sys.argv[1]
        output_filename = sys.argv[2]
        main(glb_filename, output_filename)
