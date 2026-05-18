def relabel_edges(input_file, output_file):
    # Read all edges
    edges = []
    vertices = set()
    
    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:  # Skip empty lines
                continue
            
            v0, v1 = map(int, line.split())
            edges.append((v0, v1))
            vertices.add(v0)
            vertices.add(v1)
    
    # Create mapping: old vertex ID -> new vertex ID (0, 1, 2, ...)
    old_to_new = {old_id: new_id for new_id, old_id in enumerate(sorted(vertices))}
    
    # Write relabeled edges
    with open(output_file, 'w') as f:
        for v0, v1 in edges:
            new_v0 = old_to_new[v0]
            new_v1 = old_to_new[v1]
            f.write(f"{new_v0} {new_v1}\n")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) != 3:
        print("Usage: python relabel_edges.py <input_file> <output_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    relabel_edges(input_file, output_file)