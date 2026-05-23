import sys
from collections import deque, defaultdict

def bfs(graph, root):
    visited = set()
    frontierOld = set(root)
    visited.add(root)
    iterations = 0
    
    while frontierOld != set():
        frontierNew = set()
        for vert in frontierOld:
            for neighbor in graph[vert]:
                if neighbor not in visited:
                    visited.add(neighbor)
                    frontierNew.add(neighbor)
        frontierOld=frontierNew
        iterations += 1
    
    return iterations

def load_graph(filepath):
    graph = defaultdict(list)
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            
            parts = line.split()
            if len(parts) >= 2:
                u, v = parts[0], parts[1]
                graph[u].append(v)
                graph[v].append(u)  # Undirected graph
    
    return graph

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python bfs.py <filepath> <root_vertex>")
        sys.exit(1)
    
    filepath = sys.argv[1]
    root = sys.argv[2]
    
    graph = load_graph(filepath)
    
    if root not in graph:
        print(f"Error: Root vertex '{root}' not found in graph")
        sys.exit(1)
    
    iterations = bfs(graph, root)
    print(iterations)